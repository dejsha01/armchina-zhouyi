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
 * @file  z5_simulator.cpp
 * @brief AIPU User Mode Driver (UMD) zhouyi z5 simulator module implementation
 */

#include <cstring>
#include <unistd.h>
#include <assert.h>
#include "z5_simulator.h"

aipudrv::Z5Simulator* aipudrv::Z5Simulator::m_sim = nullptr;

aipudrv::Z5Simulator::Z5Simulator(const aipu_global_config_simulation_t* cfg)
{
    m_dev_type = DEV_TYPE_SIMULATOR_Z5;
    m_dram = UMemory::get_memory();
    if (nullptr == cfg)
    {
        m_log_level = RTDEBUG_SIMULATOR_LOG_LEVEL;
        m_verbose = false;
    }
    else
    {
        m_log_level = cfg->log_level;
        m_verbose = cfg->verbose;
    }
    pthread_rwlock_init(&m_lock, NULL);
}

aipudrv::Z5Simulator::~Z5Simulator()
{
    if (m_aipu != nullptr)
    {
        if (cmd_pool_created())
        {
            m_aipu->write_register(TSM_CMD_SCHED_CTRL, DESTROY_CMD_POOL);
        }
        delete m_aipu;
    }
    pthread_rwlock_destroy(&m_lock);
    delete m_dram;
}

bool aipudrv::Z5Simulator::has_target(uint32_t arch, uint32_t version, uint32_t config, uint32_t rev)
{
    uint32_t code = 0;
    bool ret = false;

    if ((arch != 0) || (version != 5) || (rev != 0))
    {
        return false;
    }

    if (get_code(config, &code))
    {
        return false;
    }

    pthread_rwlock_wrlock(&m_lock);
    if (m_aipu != nullptr)
    {
        ret = (code == m_code);
        goto unlock;
    }

    m_config = z5_sim_create_config(code, m_log_level, m_verbose);
    m_aipu = new sim_aipu::Aipu(m_config, static_cast<UMemory&>(*m_dram));
    if (m_aipu == nullptr)
    {
        ret = false;
        goto unlock;
    }

    m_code = code;
    m_aipu->read_register(CLUSTER0_CONFIG, m_core_cnt);
    m_core_cnt = (m_core_cnt >> 8) & 0xF;
    ret = true;

unlock:
    pthread_rwlock_unlock(&m_lock);
    return ret;
}

aipu_status_t aipudrv::Z5Simulator::schedule(const JobDesc& job)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;

    assert(m_aipu != nullptr);

    pthread_rwlock_wrlock(&m_lock);
    if (cmd_pool_created() == false)
    {
        CmdPool pool(m_dram, job.tcb_head, job.tcb_tail);
        m_cmd_pools.push_back(pool);
        m_aipu->write_register(TSM_CMD_SCHED_ADDR_HI, get_high_32(job.tcb_head));
        m_aipu->write_register(TSM_CMD_SCHED_ADDR_LO, get_low_32(job.tcb_head));
        m_aipu->write_register(TSM_CMD_SCHED_CTRL, CREATE_CMD_POOL);
    }
    else
    {
        m_cmd_pools[0].update_tcb(job.tcb_head, job.tcb_tail);
    }
    pthread_rwlock_unlock(&m_lock);

    LOG(LOG_INFO, "triggering simulator...");
    m_aipu->write_register(TSM_CMD_SCHED_CTRL, DISPATCH_CMD_POOL);

    return ret;
}

aipu_ll_status_t aipudrv::Z5Simulator::get_status(std::vector<aipu_job_status_desc>& jobs_status,
    uint32_t max_cnt)
{
    uint32_t value = 0;
    aipu_job_status_desc desc;

    m_aipu->read_register(CMD_POOL0_STATUS, value);
    if (value & CMD_POOL0_IDLE)
    {
        desc.state = AIPU_JOB_STATE_DONE;
        jobs_status.push_back(desc);
    }
    return AIPU_LL_STATUS_SUCCESS;
}

aipu_ll_status_t aipudrv::Z5Simulator::poll_status(std::vector<aipu_job_status_desc>& jobs_status,
    uint32_t max_cnt, int32_t time_out, bool of_this_thread)
{
    uint32_t value = 0;
    aipu_job_status_desc desc;

    while (m_aipu->read_register(CMD_POOL0_STATUS, value) > 0)
    {
        if (value & CMD_POOL0_IDLE)
        {
            LOG(LOG_INFO, "simulation done.");
            break;
        }
        sleep(1);
        LOG(LOG_INFO, "wait for simulation execution...");
    }

    desc.state = AIPU_JOB_STATE_DONE;
    jobs_status.push_back(desc);
    return AIPU_LL_STATUS_SUCCESS;
}