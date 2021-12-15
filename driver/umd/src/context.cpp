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
 * @file  context.cpp
 * @brief AIPU User Mode Driver (UMD) context module implementation
 */

#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include "context.h"
#include "type.h"
#include "utils/log.h"
#include "utils/debug.h"
#include "utils/helper.h"
#include "device.h"
#include "graph_legacy.h"
#include "graph_z5.h"
#include "super_graph.h"
#include "job_base.h"
#include "parser_base.h"

aipudrv::MainContext::MainContext()
{
    m_dev = nullptr;
    pthread_rwlock_init(&m_glock, NULL);
    m_sim_cfg.z1_simulator = nullptr;
    m_sim_cfg.z2_simulator = nullptr;
    m_sim_cfg.z3_simulator = nullptr;
    m_sim_cfg.log_file_path = new char[1024];
    strcpy((char*)m_sim_cfg.log_file_path, "./");
    m_sim_cfg.log_level = 0;
    m_sim_cfg.verbose = false;
    m_sim_cfg.enable_avx = false;
    m_sim_cfg.enable_calloc = false;
}

aipudrv::MainContext::~MainContext()
{
    pthread_rwlock_destroy(&m_glock);
    if (m_sim_cfg.z1_simulator != nullptr)
    {
        delete[] m_sim_cfg.z1_simulator;
    }
    if (m_sim_cfg.z2_simulator != nullptr)
    {
        delete[] m_sim_cfg.z2_simulator;
    }
    if (m_sim_cfg.z3_simulator != nullptr)
    {
        delete[] m_sim_cfg.z3_simulator;
    }
    delete[] m_sim_cfg.log_file_path;
}

bool aipudrv::MainContext::is_deinit_ok()
{
    return true;
}

aipu_status_t aipudrv::MainContext::init()
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;

    /* arm64 platform init m_dev here; simulation init m_dev later */
    ret = get_device(&m_dev);
    return ret;
}

void aipudrv::MainContext::force_deinit()
{
    GraphTable::iterator iter;

    pthread_rwlock_wrlock(&m_glock);
    for (iter = m_graphs.begin(); iter != m_graphs.end(); iter++)
    {
        iter->second->unload();
    }
    m_graphs.clear();
    pthread_rwlock_unlock(&m_glock);
    if (m_dev != nullptr)
    {
        put_device(m_dev);
        m_dev = nullptr;
    }
}

aipu_status_t aipudrv::MainContext::deinit()
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;

    if (!is_deinit_ok())
    {
        ret = AIPU_STATUS_ERROR_DEINIT_FAIL;
        goto finish;
    }

    force_deinit();

finish:
    return ret;
}

aipu_status_t aipudrv::MainContext::get_status_msg(aipu_status_t status, const char** msg)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;

    if (nullptr == msg)
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    *msg = umd_status_string[status];

finish:
    return ret;
}

uint64_t aipudrv::MainContext::create_unique_graph_id_inner() const
{
    uint64_t id_candidate = (1UL << 32);
    while (m_graphs.count(id_candidate) == 1)
    {
        id_candidate++;
    }
    return id_candidate;
}

aipu_status_t aipudrv::MainContext::create_graph_object(std::ifstream& gbin, uint32_t size,
    uint64_t id, GraphBase** gobj)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    GraphBase* p_gobj = nullptr;
    uint32_t g_version = 0;

    if (nullptr == gobj)
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    g_version = ParserBase::get_graph_bin_version(gbin);
    ret = test_get_device(g_version, &m_dev, &m_sim_cfg);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        goto finish;
    }

    m_dram = m_dev->get_mem();

#if (defined ZHOUYI_V123)
    if (AIPU_LOADABLE_GRAPH_V0005 == g_version)
    {
        p_gobj = new GraphLegacy(id, m_dev);
    }
#endif
#if (defined ZHOUYI_V5)
    if (AIPU_LOADABLE_GRAPH_ELF_V0 == g_version)
    {
        p_gobj = new GraphZ5(id, m_dev);
    }
#endif

    if (nullptr == p_gobj)
    {
        ret = AIPU_STATUS_ERROR_GVERSION_UNSUPPORTED;
        goto finish;
    }

    ret = p_gobj->load(gbin, size, m_do_vcheck);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        destroy_graph_object(&p_gobj);
        goto finish;
    }

    /* success or return nullptr */
    *gobj = p_gobj;

finish:
    return ret;
}

aipudrv::GraphBase* aipudrv::MainContext::get_graph_object(GRAPH_ID id)
{
    GraphBase* p_gobj = nullptr;
    pthread_rwlock_rdlock(&m_glock);
    if (0 != m_graphs.count(id))
    {
        p_gobj = m_graphs[id];
    }
    pthread_rwlock_unlock(&m_glock);
    return p_gobj;
}

aipudrv::JobBase* aipudrv::MainContext::get_job_object(JOB_ID id)
{
    GraphBase* p_gobj = get_graph_object(job_id2graph_id(id));
    if (p_gobj == nullptr)
    {
        return nullptr;
    }
    return p_gobj->get_job(id);
}

aipu_status_t aipudrv::MainContext::destroy_graph_object(GraphBase** gobj)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;

    if ((nullptr == gobj) || (nullptr == (*gobj)))
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    ret = (*gobj)->unload();
    if (AIPU_STATUS_SUCCESS != ret)
    {
        goto finish;
    }

    /* success */
    delete *gobj;
    *gobj = nullptr;

finish:
    return ret;
}

aipu_status_t aipudrv::MainContext::load_graph(const char* graph_file, GRAPH_ID* id)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    GraphBase* gobj = nullptr;
    uint64_t _id = 0;
    std::ifstream gbin;
    int fsize = 0;

    if ((nullptr == graph_file) || (nullptr == id))
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    gbin.open(graph_file, std::ifstream::in | std::ifstream::binary);
    if (!gbin.is_open())
    {
        return AIPU_STATUS_ERROR_OPEN_FILE_FAIL;
    }

    gbin.seekg (0, gbin.end);
    fsize = gbin.tellg();
    gbin.seekg (0, gbin.beg);

    /* push nullptr into graphs to pin this graph ID */
    pthread_rwlock_wrlock(&m_glock);
    _id = create_unique_graph_id_inner();
    m_graphs[_id] = nullptr;
    pthread_rwlock_unlock(&m_glock);

    ret = create_graph_object(gbin, fsize, _id, &gobj);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        pthread_rwlock_wrlock(&m_glock);
        m_graphs.erase(_id);
        pthread_rwlock_unlock(&m_glock);
        goto finish;
    }

    /* success: update graphs[_id] */
    pthread_rwlock_wrlock(&m_glock);
    m_graphs[_id] = gobj;
    pthread_rwlock_unlock(&m_glock);
    *id = _id;

    /* TBD */
    return ret;

finish:
    gbin.close();
    return ret;
}

aipu_status_t aipudrv::MainContext::unload_graph(GRAPH_ID id)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    GraphBase* p_gobj = nullptr;

    p_gobj = get_graph_object(id);
    if (nullptr == p_gobj)
    {
        ret = AIPU_STATUS_ERROR_INVALID_GRAPH_ID;
        goto finish;
    }

    ret = destroy_graph_object(&p_gobj);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        goto finish;
    }

    /* p_gobj becomes NULL after destroy */

    /* success */
    pthread_rwlock_wrlock(&m_glock);
    m_graphs.erase(id);
    pthread_rwlock_unlock(&m_glock);

finish:
    return ret;
}

aipu_status_t aipudrv::MainContext::create_job(GRAPH_ID graph, JOB_ID* id)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    GraphBase* p_gobj = nullptr;

    if (nullptr == id)
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    p_gobj = get_graph_object(graph);
    if (nullptr == p_gobj)
    {
        ret = AIPU_STATUS_ERROR_INVALID_GRAPH_ID;
        goto finish;
    }

    ret = p_gobj->create_job(id, &m_sim_cfg);

finish:
    return ret;
}

aipu_status_t aipudrv::MainContext::get_simulation_instance(void** simulator, void** memory)
{
    return m_dev->get_simulation_instance(simulator, memory);
}

aipu_status_t aipudrv::MainContext::get_cluster_count(uint32_t* cnt)
{
    return m_dev->get_cluster_count(cnt);
}

aipu_status_t aipudrv::MainContext::get_core_count(uint32_t cluster, uint32_t* cnt)
{
    return m_dev->get_core_count(cluster, cnt);
}

aipu_status_t aipudrv::MainContext::debugger_get_job_info(uint64_t job, aipu_debugger_job_info_t* info)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    GraphBase* p_gobj = get_graph_object(get_graph_id(job));
    if (nullptr == p_gobj)
    {
        ret = AIPU_STATUS_ERROR_INVALID_JOB_ID;
        goto finish;
    }

    if (nullptr == info)
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    ret = get_simulation_instance(&info->simulation_aipu, &info->simulation_mem_engine);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        goto finish;
    }

    info->instr_base = p_gobj->debugger_get_instr_base();

finish:
    return ret;
}

aipu_status_t aipudrv::MainContext::config_simulation(uint64_t types, aipu_global_config_simulation_t* config)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;

    if (nullptr == config)
    {
        return AIPU_STATUS_ERROR_INVALID_JOB_ID;
    }

    if (config->z1_simulator != nullptr)
    {
        if (m_sim_cfg.z1_simulator == nullptr)
        {
            m_sim_cfg.z1_simulator = new char[1024];
        }
        strcpy((char*)m_sim_cfg.z1_simulator, config->z1_simulator);
    }
    if (config->z2_simulator != nullptr)
    {
        if (m_sim_cfg.z2_simulator == nullptr)
        {
            m_sim_cfg.z2_simulator = new char[1024];
        }
        strcpy((char*)m_sim_cfg.z2_simulator, config->z2_simulator);
    }
    if (config->z3_simulator != nullptr)
    {
        if (m_sim_cfg.z3_simulator == nullptr)
        {
            m_sim_cfg.z3_simulator = new char[1024];
        }
        strcpy((char*)m_sim_cfg.z3_simulator, config->z3_simulator);
    }
    if (config->log_file_path != nullptr)
    {
        strcpy((char*)m_sim_cfg.log_file_path, config->log_file_path);
    }
    m_sim_cfg.log_level = config->log_level;
    m_sim_cfg.verbose = config->verbose;
    m_sim_cfg.enable_avx = config->enable_avx;
    m_sim_cfg.enable_calloc = config->enable_calloc;
    return ret;
}

aipu_status_t aipudrv::MainContext::debugger_malloc(uint32_t size, void** va)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    BufferDesc buf;
    char* alloc_va = nullptr;
    uint32_t core_cnt = 0;

    if (nullptr == va)
    {
        return AIPU_STATUS_ERROR_NULL_PTR;
    }

    ret = m_dram->malloc(size, 1, &buf, "dbg");
    if (ret != AIPU_STATUS_SUCCESS)
    {
        return ret;
    }

    if (m_dram->pa_to_va(buf.pa, buf.size, &alloc_va) != 0)
    {
        return AIPU_STATUS_ERROR_BUF_ALLOC_FAIL;
    }

    /**
     * debugger for Jtag-only need data address 0/1 register (0x14/0x18) as
     * channel between server & client
     *
     * for all cores:
     * dreg0: buffer base address in device space
     * dreg1: magic number requested by debugger
     */
    m_dev->get_core_count(0, &core_cnt);
    for (uint32_t id = 0; id < core_cnt; id++)
    {
        m_dev->write_reg(id, 0x14, buf.pa);
        m_dev->write_reg(id, 0x18, 0x1248FFA5);
    }

    m_dbg_buffers[alloc_va] = buf;
    *va = alloc_va;
    return ret;
}

aipu_status_t aipudrv::MainContext::debugger_free(void* va)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;

    if (nullptr == va)
    {
        return AIPU_STATUS_ERROR_NULL_PTR;
    }

    if (m_dbg_buffers.count(va) == 0)
    {
        return AIPU_STATUS_ERROR_BUF_FREE_FAIL;
    }

    ret = m_dram->free(&m_dbg_buffers[va], "dbg");
    if (ret != AIPU_STATUS_SUCCESS)
    {
        return ret;
    }

    m_dbg_buffers.erase(va);
    return ret;
}
