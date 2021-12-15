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
 * @file  parser_legacy.cpp
 * @brief AIPU User Mode Driver (UMD) legacy parser module implementation
 */

#include <cstring>
#include "parser_legacy.h"
#include "utils/helper.h"
#include "utils/log.h"

aipudrv::ParserLegacy::ParserLegacy()
{
}

aipudrv::ParserLegacy::~ParserLegacy()
{
    for (uint32_t i = 0; i < m_sections.size(); i++)
    {
        delete[] m_sections[i].va;
    }
}

aipu_status_t aipudrv::ParserLegacy::parse_graph_header_bottom(std::ifstream& gbin, Graph& gobj)
{
    LegacyHeaderBottom header;
    LegacySectionDesc desc;

    gbin.read((char*)&header, sizeof(LegacyHeaderBottom));
    if (gbin.gcount() != sizeof(LegacyHeaderBottom))
    {
        return AIPU_STATUS_ERROR_INVALID_GBIN;
    }

    gobj.set_enrty(header.entry);

    /* Do NOT modify the initialization sequence */
    desc.init(header.rodata_offset, header.rodata_size);
    m_section_descs.push_back(desc);
    desc.init(header.dcr_offset, header.dcr_size);
    m_section_descs.push_back(desc);
    desc.init(header.text_offset, header.text_size);
    m_section_descs.push_back(desc);
    desc.init(header.data_offset, header.data_size);
    m_section_descs.push_back(desc);
    gbin.seekg(0, gbin.end);
    desc.init(header.bss_offset, (uint32_t)gbin.tellg() - header.bss_offset);
    m_section_descs.push_back(desc);

    return AIPU_STATUS_SUCCESS;
}

aipu_status_t aipudrv::ParserLegacy::parse_graph(std::ifstream& gbin, uint32_t size, Graph& gobj)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    BinSection section;
    char* remap = nullptr;

    if (size < (BIN_HDR_TOP_SIZE + sizeof(LegacyHeaderBottom)))
    {
        return AIPU_STATUS_ERROR_INVALID_GBIN;
    }

    gbin.seekg(0, gbin.beg);
    ret = parse_graph_header_top(gbin, size, gobj);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    ret = parse_graph_header_bottom(gbin, gobj);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    for (uint32_t i = 0; i < SECTION_TYPE_MAX; i++)
    {
        gbin.seekg(m_section_descs[i].offset, gbin.beg);
        section.size = m_section_descs[i].size;
        section.va = new char[section.size];
        gbin.read((char*)section.va, section.size);
        if (gbin.gcount() != (int)section.size)
        {
            return AIPU_STATUS_ERROR_INVALID_GBIN;
        }
        m_sections.push_back(section);
    }

    gobj.set_graph_rodata(m_sections[SECTION_TYPE_RODATA]);
    gobj.set_graph_desc(m_sections[SECTION_TYPE_DESCRIPTOR]);
    gobj.set_graph_weight(m_sections[SECTION_TYPE_WEIGHT]);
    gobj.set_graph_text(m_sections[SECTION_TYPE_TEXT].va, m_sections[SECTION_TYPE_TEXT].size);

    ret = parse_bss_section((char*)m_sections[SECTION_TYPE_BSS].va,
        m_sections[SECTION_TYPE_BSS].size, 0, gobj, &remap);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        goto finish;
    }

    ret = parse_remap_section(remap, gobj);

finish:
    return ret;
}