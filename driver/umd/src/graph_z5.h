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
 * @file  graph_z5.h
 * @brief AIPU User Mode Driver (UMD) z5 graph module header
 */

#ifndef _GRAPH_Z5_H_
#define _GRAPH_Z5_H_

#include <map>
#include <vector>
#include <deque>
#include "graph.h"

namespace aipudrv
{
struct BinSubGraphSection {
    char* va;
    uint64_t offset;
    uint64_t size;
    void load(char* _va, uint64_t _offset, uint64_t _size)
    {
        va = _va;
        offset = _offset;
        size = _size;
    }
};

struct Subgraph {
    uint32_t id;
    struct BinSubGraphSection text;
    struct BinSubGraphSection rodata;
    struct BinSubGraphSection dcr;
    uint32_t printfifo_size;
    uint32_t profiler_buf_size;
    std::vector<uint32_t> precursors;
    uint32_t stack_size;
    uint32_t stack_align_in_page;
    std::vector<struct GraphParamMapLoadDesc> param_map;
    std::vector<struct GraphSectionDesc> static_sections;
    std::vector<struct GraphSectionDesc> reuse_sections;
    struct GraphIOTensors io;
};

class GraphZ5: public Graph
{
private:
    std::vector<struct Subgraph> m_subgraphs;

public:
    void print_parse_info();
    aipu_status_t create_job(JOB_ID* id, const aipu_global_config_simulation_t* cfg);
    aipu_status_t get_tensor_count(aipu_tensor_type_t type, uint32_t* cnt);
    aipu_status_t get_tensor_descriptor(aipu_tensor_type_t type, uint32_t tensor, aipu_tensor_desc_t* desc);

public:
    void set_subgraph(struct Subgraph sg)
    {
        m_subgraphs.push_back(sg);
    }
    void set_stack(uint32_t sg_id, uint32_t size, uint32_t align)
    {
        if (sg_id < (uint32_t)m_subgraphs.size())
        {
            m_subgraphs[sg_id].stack_size = size;
            m_subgraphs[sg_id].stack_align_in_page = align;
        }
    }
    void add_param(uint32_t sg_id, struct GraphParamMapLoadDesc param)
    {
        if (sg_id < (uint32_t)m_subgraphs.size())
        {
            m_subgraphs[sg_id].param_map.push_back(param);
        }
    }
    void add_static_section(uint32_t sg_id, struct GraphSectionDesc section)
    {
        if (sg_id < (uint32_t)m_subgraphs.size())
        {
            m_subgraphs[sg_id].static_sections.push_back(section);
        }
    }
    void add_reuse_section(uint32_t sg_id, struct GraphSectionDesc section)
    {
        if (sg_id < (uint32_t)m_subgraphs.size())
        {
            m_subgraphs[sg_id].reuse_sections.push_back(section);
        }
    }
    void set_io_tensors(uint32_t sg_id, struct GraphIOTensors io)
    {
        if (sg_id < (uint32_t)m_subgraphs.size())
        {
            m_subgraphs[sg_id].io = io;
        }
    }

public:
    GraphZ5(GRAPH_ID id, DeviceBase* dev);
    ~GraphZ5();
    GraphZ5(const GraphZ5& graph) = delete;
    GraphZ5& operator=(const GraphZ5& graph) = delete;

    friend class JobZ5;
};
}

#endif /* _GRAPH_Z5_H_ */