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
 * @file  context.h
 * @brief AIPU User Mode Driver (UMD) context module header
 */

#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include <map>
#include <fstream>
#include <pthread.h>
#include "standard_api.h"
#include "graph_base.h"
#include "device_base.h"
#include "memory_base.h"

namespace aipudrv
{
typedef std::map<uint32_t, GraphBase*> GraphTable;

class MainContext
{
private:
    DeviceBase* m_dev = nullptr;
    MemoryBase* m_dram = nullptr;
    GraphTable  m_graphs;
    pthread_rwlock_t m_glock;
    bool m_do_vcheck = true;
    std::map<void*, BufferDesc> m_dbg_buffers;

private:
    static char umd_status_string[][1024];

private:
    aipu_global_config_simulation_t m_sim_cfg;

private:
    uint64_t create_unique_graph_id_inner() const;
    aipu_status_t create_graph_object(std::ifstream& gbin, uint32_t size, uint64_t id, GraphBase** gobj);
    aipu_status_t destroy_graph_object(GraphBase** gobj);

private:
    bool is_deinit_ok();

public:
    /* z5 core APIs */
    aipu_status_t init();
    void force_deinit();
    aipu_status_t deinit();
    GraphBase*    get_graph_object(GRAPH_ID id);
    JobBase*      get_job_object(JOB_ID id);
    aipu_status_t get_status_msg(aipu_status_t status, const char** msg);
    aipu_status_t load_graph(const char* graph_file, GRAPH_ID* id);
    aipu_status_t unload_graph(GRAPH_ID id);
    aipu_status_t get_simulation_instance(void** simulator, void** memory);
    aipu_status_t create_job(GRAPH_ID graph, JOB_ID* id);
    aipu_status_t get_cluster_count(uint32_t* cnt);
    aipu_status_t get_core_count(uint32_t cluster, uint32_t* cnt);
    aipu_status_t debugger_get_job_info(JOB_ID job, aipu_debugger_job_info_t* info);
    aipu_status_t config_simulation(uint64_t types, aipu_global_config_simulation_t* config);
    void disable_version_check()
    {
        m_do_vcheck = false;
    }
    void enable_version_check()
    {
        m_do_vcheck = true;
    }
    aipu_status_t debugger_malloc(uint32_t size, void** va);
    aipu_status_t debugger_free(void* va);

public:
    DeviceBase* get_dev()
    {
        return m_dev;
    };

public:
    MainContext(const MainContext& ctx) = delete;
    MainContext& operator=(const MainContext& ctx) = delete;
    ~MainContext();
    MainContext();
};
}

struct ctx_handle {
    uint32_t handle;
};

#endif /* _CONTEXT_H_ */