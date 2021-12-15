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
 * @file  parser_elf.cpp
 * @brief AIPU User Mode Driver (UMD) ELF parser module implementation
 */

#include <cstring>
#include "parser_elf.h"

aipudrv::ParserELF::ParserELF(): ParserBase()
{
}

aipudrv::ParserELF::~ParserELF()
{
}

aipu_status_t aipudrv::ParserELF::parse_graph_header_bottom(std::ifstream& gbin)
{
    gbin.read((char*)&m_header, sizeof(ELFHeaderBottom));
    if (gbin.gcount() != sizeof(ELFHeaderBottom))
    {
        return AIPU_STATUS_ERROR_INVALID_GBIN;
    }
    return AIPU_STATUS_SUCCESS;
}

aipudrv::BinSection aipudrv::ParserELF::get_bin_note(const std::string& note_name)
{
    aipudrv::BinSection ro = {nullptr, 0};

    if (m_note)
    {
        ELFIO::note_section_accessor notes(m_elf, m_note);
        ELFIO::Elf_Word no_notes = notes.get_notes_num();
        for (ELFIO::Elf_Word j = 0; j < no_notes; ++j)
        {
            ELFIO::Elf_Word type;
            std::string name;
            void *desc;
            ELFIO::Elf_Word size;

            if (notes.get_note(j, type, name, desc, size))
            {
                if (name == note_name)
                {
                    ro.va = (char *)desc;
                    ro.size = size;
                    break;
                }
            }
        }
    }
    return ro;
}

ELFIO::section* aipudrv::ParserELF::get_elf_section(const std::string& section_name)
{
    ELFIO::Elf_Half no = m_elf.sections.size();

    for (ELFIO::Elf_Half i = 0; i < no; ++i)
    {
        ELFIO::section *sec = m_elf.sections[i];

        if (section_name == sec->get_name())
        {
            return sec;
        }
    }
    return nullptr;
}

aipu_status_t aipudrv::ParserELF::parse_subgraph(char* start, uint32_t id, GraphZ5& gobj,
        uint64_t& sg_desc_size)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    Subgraph sg;
    ElfSubGraphDesc gbin_sg_desc;
    FeatureMapList  fm_list;
    char* bss = nullptr;
    char* next = nullptr;

    if (nullptr == start)
    {
        return AIPU_STATUS_ERROR_NULL_PTR;
    }

    memcpy(&gbin_sg_desc, start, sizeof(gbin_sg_desc));
    sg_desc_size = sizeof(gbin_sg_desc) +
                   sizeof(struct ElfPrecursorDesc) * gbin_sg_desc.precursor_cnt;

    sg.id = id;
    sg.text.load(nullptr, gbin_sg_desc.text_offset, 0);
    sg.rodata.load(nullptr, gbin_sg_desc.rodata_offset, gbin_sg_desc.rodata_size);
    sg.dcr.load(nullptr, gbin_sg_desc.dcr_offset, gbin_sg_desc.dcr_size);
    sg.printfifo_size    = gbin_sg_desc.printfifo_size;
    sg.profiler_buf_size = gbin_sg_desc.profiler_buf_size;

    start += sizeof(gbin_sg_desc);
    for(uint32_t i = 0; i < gbin_sg_desc.precursor_cnt; i++)
    {
        struct ElfPrecursorDesc pre;
        memcpy(&pre, start, sizeof(pre));
        sg.precursors.push_back(pre.id);
        start += sizeof(pre);
    }
    gobj.set_subgraph(sg);

    bss = (char*)sections[ELFSectionFMList].va + gbin_sg_desc.fm_desc_offset;
    memcpy(&fm_list, bss, sizeof(fm_list));
    bss += sizeof(fm_list);
    for (uint32_t i = 0; i < fm_list.num_fm_descriptor; i++)
    {
        ret = parse_bss_section(bss, 0, id, gobj, &next);
        if (AIPU_STATUS_SUCCESS != ret)
        {
            return ret;
        }
        bss = next;
    }

    return ret;
}

aipu_status_t aipudrv::ParserELF::parse_graph(std::ifstream& gbin, uint32_t size, Graph& gobj)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    struct ElfSubGraphList sg_desc_header;
    char* start = nullptr;

    if (size < (BIN_HDR_TOP_SIZE + sizeof(ELFHeaderBottom)))
    {
        return AIPU_STATUS_ERROR_INVALID_GBIN;
    }

    gbin.seekg(0, gbin.beg);
    ret = parse_graph_header_top(gbin, size, gobj);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    ret = parse_graph_header_bottom(gbin);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    /* real ELF parsing */
    if (!m_elf.load(gbin))
    {
        ret = AIPU_STATUS_ERROR_INVALID_GBIN;
        goto finish;
    }

    /* .text section parse */
    m_text = get_elf_section(".text");
    if (nullptr == m_text)
    {
        ret = AIPU_STATUS_ERROR_INVALID_GBIN;
        goto finish;
    }
    gobj.set_graph_text(m_text->get_data(), m_text->get_size());

    /* .data section parse */
    m_data = get_elf_section(".data");
    if (nullptr != m_data)
    {
        gobj.set_graph_dp(m_data->get_data(), m_data->get_size());
    }

    /* .note section parse */
    m_note = get_elf_section(".note.aipu");
    if (nullptr == m_note)
    {
        ret = AIPU_STATUS_ERROR_INVALID_GBIN;
        goto finish;
    }

    for (uint32_t i = 0; i < ELFSectionCnt; i++)
    {
        sections[i] = get_bin_note(ELFSectionName[i]);
    }
    gobj.set_graph_rodata(sections[ELFSectionRodata]);
    if (sections[ELFSectionDesc].size != 0)
    {
        gobj.set_graph_desc(sections[ELFSectionDesc]);
    }
    if (sections[ELFSectionWeight].size != 0)
    {
        gobj.set_graph_weight(sections[ELFSectionWeight]);
    }

    start = (char*)sections[ELFSectionSubGraphs].va;
    memcpy(&sg_desc_header, start, sizeof(sg_desc_header));
    if (0 == sg_desc_header.subgraphs_cnt)
    {
        ret = AIPU_STATUS_ERROR_INVALID_GBIN;
        goto finish;
    }

    start += sizeof(sg_desc_header);
    for (uint32_t i = 0; i < sg_desc_header.subgraphs_cnt; i++)
    {
        uint64_t sg_desc_size = 0;
        ret = parse_subgraph(start, i, static_cast<GraphZ5&>(gobj), sg_desc_size);
        if (ret)
        {
            goto finish;
        }
        start += sg_desc_size;
    }

    start = (char*)sections[ELFSectionRemap].va;
    ret = parse_remap_section(start, gobj);

finish:
    return ret;
}
