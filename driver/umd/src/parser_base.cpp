/**********************************************************************************
 * This file is CONFIDENTIAL and any use by you is subject to the terms of the
 * agreement between you and Arm China or the terms of the agreement between you
 * and the party authorised by Arm China to disclose this file to you.
 * The confidential and proprietary information contained in this file
 * may only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm China.
 *
 *        (C) Copyright 2020 Arm Technology (China) Co. Ltd.
 *                    All rights reserved.
 *
 * This entire notice must be reproduced on all copies of this file and copies of
 * this file may only be made by a person if such person is permitted to do so
 * under the terms of a subsisting license agreement from Arm China.
 *
 *********************************************************************************/

/**
 * @file  parser_base.cpp
 * @brief AIPU User Mode Driver (UMD) parser module implementation
 */

#include <cstring>
#include <cstdint>
#include "parser_base.h"
#include "graph.h"
#include "utils/helper.h"
#include "utils/log.h"

#define PRINT_GRAPH_HDR_PARSING_INFO 0

aipudrv::ParserBase::ParserBase()
{
}

aipudrv::ParserBase::~ParserBase()
{
}

void aipudrv::ParserBase::print_graph_header_top(const BinHeaderTop& top)
{
#if (defined PRINT_GRAPH_HDR_PARSING_INFO) && (PRINT_GRAPH_HDR_PARSING_INFO == 1)
    LOG(LOG_DEFAULT, "===========================AIPU Bin Header (Top)===========================");
    LOG(LOG_DEFAULT, "Graph magic: %s", top.magic);
    LOG(LOG_DEFAULT, "Target device: 0x%x (arch %u, version %u, configuration %u, revision %u)",
        top.device,
        AIPU_ARCH(top.device),
        AIPU_VERSION(top.device),
        AIPU_CONFIG(top.device),
        AIPU_REVISION(top.device));
    LOG(LOG_DEFAULT, "Graph version: %u", GRAPH_VERSION(top.version));
    LOG(LOG_DEFAULT, "Building tool version: 0x%x (major %u, minor %u, build number %u)",
        top.build_version,
        BUILD_MAJOR(top.build_version),
        BUILD_MINOR(top.build_version),
        BUILD_NUMBER(top.build_version));
    LOG(LOG_DEFAULT, "Graph header size: %u", top.header_size);
    LOG(LOG_DEFAULT, "Graph file size: %u", top.file_size);
    LOG(LOG_DEFAULT, "Graph type: 0x%x", top.type);
    LOG(LOG_DEFAULT, "Graph flag: 0x%x (ASID %u, ASID_EN %u, REMAP_EN %u, SRAM_EN %u)",
        top.flag,
        GET_ASID_FLAG(top.flag),
        IS_ASID_ENABLED(top.flag),
        GET_REMAP_FLAG(top.flag),
        GET_SRAM_FLAG(top.flag));
    LOG(LOG_DEFAULT, "===========================================================================");
#endif
}

aipu_status_t aipudrv::ParserBase::sort_io_tensor(std::vector<GraphIOTensorDesc>& tensors) const
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    std::vector<GraphIOTensorDesc> tensors_tmp = tensors;

    for (uint32_t i = 0; i < tensors_tmp.size(); i++)
    {
        if (tensors_tmp[i].id >= tensors_tmp.size())
        {
            ret = AIPU_STATUS_ERROR_INVALID_GBIN;
            break;
        }
        if (tensors_tmp[i].id != i)
        {
            tensors[tensors_tmp[i].id] = tensors_tmp[i];
        }
    }

    return ret;
}

template<typename sub_section_desc_v3_t>
aipu_status_t aipudrv::ParserBase::fill_io_tensor_desc_inner(uint32_t reuse_sec_iter,
    uint32_t sub_sec_iter, const sub_section_desc_v3_t& sub_section_load, struct GraphIOTensors& desc) const
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    GraphIOTensorDesc io_desc;

    io_desc.size = sub_section_load.size;
    io_desc.id = sub_section_load.id;
    io_desc.ref_section_iter = reuse_sec_iter;
    io_desc.offset_in_section = sub_section_load.offset_in_section_exec;
    io_desc.scale = sub_section_load.scale;
    io_desc.zero_point = sub_section_load.zero_point;
    io_desc.data_type = (aipu_data_type_t)sub_section_load.data_type;

    if (SECTION_TYPE_INPUT == sub_section_load.type)
    {
        desc.inputs.push_back(io_desc);
    }
    else if (SECTION_TYPE_OUTPUT == sub_section_load.type)
    {
        desc.outputs.push_back(io_desc);
    }
    else if (SECTION_TYPE_INTER_DUMP == sub_section_load.type)
    {
        desc.inter_dumps.push_back(io_desc);
    }
    else if (SECTION_TYPE_PROF_DATA == sub_section_load.type)
    {
        desc.profiler.push_back(io_desc);
    }
    else if (SECTION_TYPE_PLOG_DATA == sub_section_load.type)
    {
        desc.printf.push_back(io_desc);
    }
    else if (SECTION_TYPE_LAYER_COUNTER == sub_section_load.type)
    {
        desc.layer_counter.push_back(io_desc);
    }

    return ret;
}

aipu_status_t aipudrv::ParserBase::parse_bss_section(char* bss, uint32_t size, uint32_t id,
    Graph& gobj, char** next) const
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    char* desc_load_addr = nullptr;
    BSSHeader             bss_header;
    BSSStaticSectionDesc  static_desc_load;
    BSSReuseSectionDesc   reuse_desc_load;
    SubSectionDesc        sub_desc_load;
    GraphSectionDesc      section_ir;
    GraphParamMapLoadDesc param;
    GraphIOTensors        io;

    void* load_lb = bss;
    void* load_ub = (void*)((unsigned long)bss + sizeof(BSSHeader) + size);
    if (0 == size)
    {
        load_ub = (void*)UINT64_MAX;
    }

    memcpy(&bss_header, bss, sizeof(BSSHeader));
    if ((bss_header.stack_size == 0) || (bss_header.stack_align_bytes == 0) ||
        (bss_header.reuse_section_desc_cnt == 0))
    {
        ret = AIPU_STATUS_ERROR_INVALID_GBIN;
        goto finish;
    }

    /* set stack section descriptions */
    gobj.set_stack(id, bss_header.stack_size, ALIGN_ADDR(bss_header.stack_align_bytes));

    /* static sections (weight/bias) in bss */
    desc_load_addr = (char*)(bss + sizeof(BSSHeader));
    for (uint32_t static_sec_iter = 0; static_sec_iter < bss_header.static_section_desc_cnt; static_sec_iter++)
    {
        section_ir.init();
        if (umd_is_valid_ptr(load_lb, load_ub, desc_load_addr, sizeof(BSSStaticSectionDesc)))
        {
            memcpy(&static_desc_load, desc_load_addr, sizeof(BSSStaticSectionDesc));
        }
        else
        {
            goto overflow;
        }

        /* update sub-section-desc desc. */
        desc_load_addr = (char*)(desc_load_addr + sizeof(BSSStaticSectionDesc));
        for (uint32_t sub_sec_iter = 0; sub_sec_iter < static_desc_load.sub_section_cnt; sub_sec_iter++)
        {
            GraphSubSectionDesc sub_desc_ir;
            if (umd_is_valid_ptr(load_lb, load_ub, desc_load_addr, sizeof(SubSectionDesc)))
            {
                memcpy(&sub_desc_load, desc_load_addr, sizeof(SubSectionDesc));
            }
            else
            {
                goto overflow;
            }

            /* get subsection desc. */
            sub_desc_ir.offset_in_section = sub_desc_load.offset_in_section_exec;
            section_ir.sub_sections.push_back(sub_desc_ir);

            /* update parameter map element */
            desc_load_addr = (char*)(desc_load_addr + sizeof(SubSectionDesc));
            for (uint32_t ro_entry_iter = 0; ro_entry_iter < sub_desc_load.offset_in_ro_cnt; ro_entry_iter++)
            {
                uint32_t offset_in_ro = 0;
                if (umd_is_valid_ptr(load_lb, load_ub, desc_load_addr, sizeof(uint32_t)))
                {
                    offset_in_ro = *(uint32_t*)desc_load_addr;
                }
                else
                {
                    goto overflow;
                }
                param.init(offset_in_ro, PARAM_MAP_LOAD_TYPE_STATIC, static_sec_iter, sub_sec_iter,
                    sub_desc_load.offset_in_section_exec, sub_desc_load.addr_mask);
                gobj.add_param(id, param);
                desc_load_addr = (char*)(desc_load_addr + sizeof(uint32_t));
            }
        }

        /* update section descriptor */
        section_ir.size = static_desc_load.size;
        section_ir.align_in_page = ALIGN_ADDR(static_desc_load.align_bytes);
        section_ir.offset = static_desc_load.offset_in_file;
        section_ir.load_src = (char*)((unsigned long)gobj.get_bweight_base() + static_desc_load.offset_in_file);
        gobj.add_static_section(id, section_ir);
    }

    /* reuse sections (input/output/intermediate) in bss */
    for (uint32_t reuse_sec_iter = 0; reuse_sec_iter < bss_header.reuse_section_desc_cnt; reuse_sec_iter++)
    {
        section_ir.init();
        if (umd_is_valid_ptr(load_lb, load_ub, desc_load_addr, sizeof(BSSReuseSectionDesc)))
        {
            memcpy(&reuse_desc_load, desc_load_addr, sizeof(BSSReuseSectionDesc));
        }
        else
        {
            goto overflow;
        }

        desc_load_addr = (char*)(desc_load_addr + sizeof(BSSReuseSectionDesc));
        for (uint32_t sub_sec_iter = 0; sub_sec_iter < reuse_desc_load.sub_section_cnt; sub_sec_iter++)
        {
            GraphSubSectionDesc sub_desc_ir;
            if (umd_is_valid_ptr(load_lb, load_ub, desc_load_addr, sizeof(SubSectionDesc)))
            {
                memcpy(&sub_desc_load, desc_load_addr, sizeof(SubSectionDesc));
            }
            else
            {
                goto overflow;
            }

            /* get io tensor info if this sub-section represents io */
            if ((SECTION_TYPE_INPUT == sub_desc_load.type) ||
                (SECTION_TYPE_OUTPUT == sub_desc_load.type) ||
                (SECTION_TYPE_INTER_DUMP == sub_desc_load.type) ||
                (SECTION_TYPE_PROF_DATA == sub_desc_load.type) ||
                (SECTION_TYPE_PLOG_DATA == sub_desc_load.type) ||
                (SECTION_TYPE_LAYER_COUNTER == sub_desc_load.type))
            {
                fill_io_tensor_desc_inner<SubSectionDesc>(reuse_sec_iter,
                    sub_sec_iter, sub_desc_load, io);
            }

            /* get subsection desc. */
            sub_desc_ir.offset_in_section = sub_desc_load.offset_in_section_exec;
            section_ir.sub_sections.push_back(sub_desc_ir);

            /* update parameter map element */
            desc_load_addr = (char*)(desc_load_addr + sizeof(SubSectionDesc));
            for (uint32_t ro_entry_iter = 0; ro_entry_iter < sub_desc_load.offset_in_ro_cnt; ro_entry_iter++)
            {
                uint32_t offset_in_ro = 0;
                if (umd_is_valid_ptr(load_lb, load_ub, desc_load_addr, sizeof(uint32_t)))
                {
                    offset_in_ro = *(uint32_t*)desc_load_addr;
                }
                else
                {
                    goto overflow;
                }

                param.init(offset_in_ro, PARAM_MAP_LOAD_TYPE_REUSE, reuse_sec_iter, sub_sec_iter,
                    sub_desc_load.offset_in_section_exec, sub_desc_load.addr_mask);
                gobj.add_param(id, param);
                desc_load_addr = (char*)(desc_load_addr + sizeof(uint32_t));
            }
        }

        /* update section descriptor */
        section_ir.load_src = nullptr;
        section_ir.align_in_page = ALIGN_ADDR(reuse_desc_load.align_bytes);
        section_ir.size = reuse_desc_load.size;
        gobj.add_reuse_section(id, section_ir);
    }

    /* sort IO tensors by tensor ID */
    ret = sort_io_tensor(io.inputs);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        goto finish;
    }
    ret = sort_io_tensor(io.outputs);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        goto finish;
    }
    gobj.set_io_tensors(id, io);

    /* success */
    *next = desc_load_addr;
    goto finish;

overflow:
    LOG(LOG_DEBUG, "[UMD ERROR] Input graph binary contains invalid offset value(s)!");
    ret = AIPU_STATUS_ERROR_INVALID_GBIN;

finish:
    return ret;
}

aipu_status_t aipudrv::ParserBase::parse_remap_section(char* remap, Graph& gobj)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    char* desc_load_addr = remap;
    RemapSectionDesc remap_desc;

    if (remap != nullptr)
    {
        memcpy(&remap_desc, desc_load_addr, sizeof(RemapSectionDesc));
        if (remap_desc.entry_cnt == 0)
        {
            return ret;
        }
        desc_load_addr = (char*)(desc_load_addr + sizeof(RemapSectionDesc));

        for (uint32_t i = 0; i < remap_desc.entry_cnt; i++)
        {
            RemapEntry entry;
            memcpy(&entry, desc_load_addr, sizeof(RemapEntry));
            gobj.add_remap(entry);
            desc_load_addr = (char*)(desc_load_addr + sizeof(RemapEntry));
        }
    }
    return ret;
}

aipu_status_t aipudrv::ParserBase::parse_graph_header_top(std::ifstream& gbin, uint32_t size, Graph& gobj)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    BinHeaderTop header;
    uint32_t remap_flag = 0;

    gbin.read((char*)&header, BIN_HDR_TOP_SIZE);
    if (gbin.gcount() != BIN_HDR_TOP_SIZE)
    {
        return AIPU_STATUS_ERROR_INVALID_GBIN;
    }

    if (strcmp(header.magic, MAGIC) != 0)
    {
        ret = AIPU_STATUS_ERROR_UNKNOWN_BIN;
        goto finish;
    }

    if ((AIPU_LOADABLE_GRAPH_V0005 != GRAPH_VERSION(header.version)) &&
        (AIPU_LOADABLE_GRAPH_ELF_V0 != GRAPH_VERSION(header.version)))
    {
        ret = AIPU_STATUS_ERROR_GVERSION_UNSUPPORTED;
        goto finish;
    }

    gobj.set_gversion(GRAPH_VERSION(header.version));
    gobj.set_arch(AIPU_ARCH(header.device));
    gobj.set_hw_version(AIPU_VERSION(header.device));
    gobj.set_hw_config(AIPU_CONFIG(header.device));
    gobj.set_hw_revision(AIPU_REVISION(header.device));
    gobj.set_asid_flag(GET_ASID_FLAG(header.flag));
    gobj.set_sram_flag(GET_SRAM_FLAG(header.flag));

    remap_flag = GET_REMAP_FLAG(header.flag);
    gobj.set_remap_flag(remap_flag);

    print_graph_header_top(header);

finish:
    return ret;
}

uint32_t aipudrv::ParserBase::get_graph_bin_version(std::ifstream& gbin)
{
    BinHeaderTop header;
    uint32_t g_version = 0;

    gbin.read((char*)&header, BIN_HDR_TOP_SIZE);
    gbin.seekg(0, gbin.beg);
    if (gbin.gcount() != BIN_HDR_TOP_SIZE)
    {
        goto finish;
    }

    if (strcmp(header.magic, MAGIC) != 0)
    {
        goto finish;
    }

    g_version = GRAPH_VERSION(header.version);

finish:
    return g_version;
}
