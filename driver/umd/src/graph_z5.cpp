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
 * @file  graph_z5.cpp
 * @brief AIPU User Mode Driver (UMD) z5 graph module implementation
 */

#include <cstring>
#include "graph_z5.h"
#include "job_z5.h"
#include "parser_elf.h"
#include "utils/helper.h"
#include "utils/log.h"

aipudrv::GraphZ5::GraphZ5(GRAPH_ID id, DeviceBase* dev): Graph(id, dev)
{
    m_parser = new ParserELF();
}

aipudrv::GraphZ5::~GraphZ5()
{
    unload();
    delete m_parser;
}

void aipudrv::GraphZ5::print_parse_info()
{
    LOG(LOG_DEFAULT, "=====================Graph Parse Results====================");
    LOG(LOG_DEFAULT, "Target device: z%u-%u", m_hw_version, m_hw_config);
    LOG(LOG_DEFAULT, "--Text:      size 0x%lx", m_btext.size);
    LOG(LOG_DEFAULT, "--Rodata:    size 0x%lx", m_brodata.size);
    LOG(LOG_DEFAULT, "--DCR:       size 0x%lx", m_bdesc.size);
    LOG(LOG_DEFAULT, "--Weight:    size 0x%lx", m_bweight.size);
    LOG(LOG_DEFAULT, "--Data (CC): size 0x%lx", m_bdata.size);
    LOG(LOG_DEFAULT, "--Remap:     cnt  0x%lx", m_remap.size());
    LOG(LOG_DEFAULT, "--Subgraph:  cnt  0x%lx", m_subgraphs.size());
    for (uint32_t i = 0; i < m_subgraphs.size(); i++)
    {
        LOG(LOG_DEFAULT, "[subgraph #%d]\n", m_subgraphs[i].id);
        LOG(LOG_DEFAULT, "--Text:       offset 0x%lx, size 0x%lx", m_subgraphs[i].text.offset, m_subgraphs[i].text.size);
        LOG(LOG_DEFAULT, "--Rodata:     offset 0x%lx, size 0x%lx", m_subgraphs[i].rodata.offset, m_subgraphs[i].rodata.size);
        LOG(LOG_DEFAULT, "--DCR:        offset 0x%lx, size 0x%lx", m_subgraphs[i].dcr.offset, m_subgraphs[i].dcr.size);
        LOG(LOG_DEFAULT, "--printf:     size 0x%x", m_subgraphs[i].printfifo_size);
        LOG(LOG_DEFAULT, "--profiler:   size 0x%x", m_subgraphs[i].profiler_buf_size);
        LOG(LOG_DEFAULT, "--precursors: size 0x%lx", m_subgraphs[i].precursors.size());
        LOG(LOG_DEFAULT, "--stack:      size 0x%x, align 0x%x", m_subgraphs[i].stack_size, m_subgraphs[i].stack_align_in_page);
        LOG(LOG_DEFAULT, "--static:     cnt 0x%lx", m_subgraphs[i].static_sections.size());
        for (uint32_t j = 0; j < m_subgraphs[i].static_sections.size(); j++)
        {
            LOG(LOG_DEFAULT, "----static section [%d]: size 0x%x, align 0x%x",
                j, m_subgraphs[i].static_sections[j].size, m_subgraphs[i].static_sections[j].align_in_page);
            for (uint32_t k = 0; k < m_subgraphs[i].static_sections[j].sub_sections.size(); k++)
            {
                LOG(LOG_DEFAULT, "------subsection [%d]: offset 0x%x",
                    k, m_subgraphs[i].static_sections[j].sub_sections[k].offset_in_section);
            }
        }
        LOG(LOG_DEFAULT, "--reuse:      cnt 0x%lx", m_subgraphs[i].reuse_sections.size());
        for (uint32_t j = 0; j < m_subgraphs[i].reuse_sections.size(); j++)
        {
            LOG(LOG_DEFAULT, "----reuse section [%d]: size 0x%x, align 0x%x",
                j, m_subgraphs[i].reuse_sections[j].size, m_subgraphs[i].reuse_sections[j].align_in_page);
            for (uint32_t k = 0; k < m_subgraphs[i].reuse_sections[j].sub_sections.size(); k++)
            {
                LOG(LOG_DEFAULT, "------subsection [%d]: offset 0x%x",
                    k, m_subgraphs[i].reuse_sections[j].sub_sections[k].offset_in_section);
            }
        }
    }
    LOG(LOG_DEFAULT, "============================================================");
}

aipu_status_t aipudrv::GraphZ5::create_job(JOB_ID* id, const aipu_global_config_simulation_t* cfg)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    JobZ5* job = new JobZ5(*this, m_dev);

    ret = job->init(cfg);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    *id = add_job(job);
    return ret;
}

aipu_status_t aipudrv::GraphZ5::get_tensor_count(aipu_tensor_type_t type, uint32_t* cnt)
{
    if (nullptr == cnt)
    {
        return AIPU_STATUS_ERROR_NULL_PTR;
    }

    if (type == AIPU_TENSOR_TYPE_INPUT)
    {
        *cnt = (uint32_t)m_subgraphs[0].io.inputs.size();
    }
    else if (type == AIPU_TENSOR_TYPE_OUTPUT)
    {
        *cnt = (uint32_t)m_subgraphs[0].io.outputs.size();
    }
    else if (type == AIPU_TENSOR_TYPE_PRINTF)
    {
        *cnt = (uint32_t)m_subgraphs[0].io.printf.size();
    }
    else if (type == AIPU_TENSOR_TYPE_PROFILER)
    {
        *cnt = (uint32_t)m_subgraphs[0].io.profiler.size();
    }

    return AIPU_STATUS_SUCCESS;
}

aipu_status_t aipudrv::GraphZ5::get_tensor_descriptor(aipu_tensor_type_t type, uint32_t tensor, aipu_tensor_desc_t* desc)
{
    uint32_t cnt = 0;
    GraphIOTensorDesc io;

    get_tensor_count(type, &cnt);
    if (tensor >= cnt)
    {
        return AIPU_STATUS_ERROR_INVALID_TENSOR_ID;
    }

    if (nullptr == desc)
    {
        return AIPU_STATUS_ERROR_NULL_PTR;
    }

    if (type == AIPU_TENSOR_TYPE_INPUT)
    {
        io = m_subgraphs[0].io.inputs[tensor];
    }
    else if (type == AIPU_TENSOR_TYPE_OUTPUT)
    {
        io = m_subgraphs[0].io.outputs[tensor];
    }
    else if (type == AIPU_TENSOR_TYPE_PRINTF)
    {
        io = m_subgraphs[0].io.printf[tensor];
    }
    else if (type == AIPU_TENSOR_TYPE_PROFILER)
    {
        io = m_subgraphs[0].io.profiler[tensor];
    }
    else
    {
        return AIPU_STATUS_ERROR_INVALID_TENSOR_ID;
    }

    desc->id = tensor;
    desc->size = io.size;
    desc->scale = io.scale;
    desc->zero_point = io.zero_point;
    desc->data_type = io.data_type;

    return AIPU_STATUS_SUCCESS;
}
