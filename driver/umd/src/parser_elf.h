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
 * @file  parser_elf.h
 * @brief AIPU User Mode Driver (UMD) ELF parser class header
 */

#ifndef _PARSER_ELF_H_
#define _PARSER_ELF_H_

#include <fstream>
#include "elfio/elfio.hpp"
#include "parser_base.h"
#include "graph_z5.h"

namespace aipudrv
{
struct ELFHeaderBottom {
    uint32_t elf_offset;
    uint32_t elf_size;
    uint32_t extra_data[8] = {0};
};

struct FeatureMapList
{
    uint32_t reserve0 = 0;
    uint32_t reserve1 = 0;
    uint32_t num_fm_descriptor = 0;
};

struct ElfSubGraphList
{
    uint32_t subgraphs_cnt;
};

struct ElfSubGraphDesc
{
    uint32_t id;
    uint32_t type;
    uint32_t text_offset;
    uint32_t fm_desc_offset;
    uint32_t rodata_offset;
    uint32_t rodata_size;
    uint32_t dcr_offset;
    uint32_t dcr_size;
    uint32_t printfifo_size;
    uint32_t profiler_buf_size;
    uint32_t reserve0;
    uint32_t reserve1;
    uint32_t reserve2;
    uint32_t reserve3;
    uint32_t precursor_cnt;
};

struct ElfPrecursorDesc
{
    uint32_t id;
};

enum ELFSection {
    ELFSectionRodata = 0,
    ELFSectionDesc,
    ELFSectionWeight,
    ELFSectionFMList,
    ELFSectionRemap,
    ELFSectionSubGraphs,
    ELFSectionCnt,
};

class ParserELF : public ParserBase
{
private:
    ELFHeaderBottom m_header;
    ELFIO::elfio m_elf;

private:
    ELFIO::section* m_text = nullptr;
    ELFIO::section* m_data = nullptr;
    ELFIO::section* m_note = nullptr;

    BinSection sections[ELFSectionCnt];
    const char* ELFSectionName[ELFSectionCnt] = {
        "rodata",
        "desc",
        "weight",
        "fmlist",
        "remap",
        "subgraphs",
    };
    uint32_t subgraph_cnt;

private:
    BinSection get_bin_note(const std::string& note_name);
    ELFIO::section* get_elf_section(const std::string &section_name);
    aipu_status_t parse_subgraph(char* start, uint32_t id, GraphZ5& gobj,
        uint64_t& sg_desc_size);

private:
    aipu_status_t parse_graph_header_bottom(std::ifstream& gbin);

public:
    virtual aipu_status_t parse_graph(std::ifstream& gbin, uint32_t size,
        Graph& gobj);

public:
    ParserELF(const ParserELF& parser) = delete;
    ParserELF& operator=(const ParserELF& parser) = delete;
    virtual ~ParserELF();
    ParserELF();
};
}

#endif /* _PARSER_ELF_H_ */