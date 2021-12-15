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
 * @file  z5_simulator.h
 * @brief AIPU User Mode Driver (UMD) zhouyi z5 simulator module header
 */

#ifndef _Z5_SIMULATOR_H_
#define _Z5_SIMULATOR_H_

#include <map>
#include <pthread.h>
#include "standard_api.h"
#include "device_base.h"
#include "umemory.h"
#include "simulator/aipu.h"
#include "simulator/config.h"
#include "kmd/tcb.h"
#include "utils/debug.h"

namespace aipudrv
{
class CmdPool
{
private:
    MemoryBase* m_dram = nullptr;
    DEV_PA_64 m_tcb_head = 0;
    DEV_PA_64 m_tcb_tail = 0;

public:
    void update_tcb(DEV_PA_64 head, DEV_PA_64 tail)
    {
        tcb_t prev;
        m_dram->read(m_tcb_tail, &prev, sizeof(prev));
        prev.next = get_low_32(head);
        m_dram->write(m_tcb_tail, &prev, sizeof(prev));
        m_tcb_tail = tail;
    }

public:
    CmdPool(MemoryBase* mem, DEV_PA_64 head, DEV_PA_64 tail)
    {
        m_dram = mem;
        m_tcb_head = head;
        m_tcb_tail = tail;
    }
};

class Z5Simulator : public DeviceBase
{
private:
    sim_aipu::config_t m_config;
    sim_aipu::Aipu*    m_aipu = nullptr;
    uint32_t m_code = 0;
    uint32_t m_log_level;
    bool m_verbose;
    std::vector<CmdPool> m_cmd_pools;
    pthread_rwlock_t m_lock;

private:
    bool cmd_pool_created()
    {
        return m_cmd_pools.size();
    }

public:
    bool has_target(uint32_t arch, uint32_t version, uint32_t config, uint32_t rev);
    aipu_status_t schedule(const JobDesc& job);
    aipu_status_t get_simulation_instance(void** simulator, void** memory)
    {
        if (m_aipu != nullptr)
        {
            *(sim_aipu::Aipu**)simulator = m_aipu;
            *(sim_aipu::IMemEngine**)memory = static_cast<UMemory*>(m_dram);
            return AIPU_STATUS_SUCCESS;
        }
        return AIPU_STATUS_ERROR_INVALID_OP;
    }
    aipu_ll_status_t get_status(std::vector<aipu_job_status_desc>& jobs_status,
        uint32_t max_cnt);
    aipu_ll_status_t poll_status(std::vector<aipu_job_status_desc>& jobs_status,
        uint32_t max_cnt, int32_t time_out, bool of_this_thread);
    int get_config_code()
    {
        return m_config.code;
    }

public:
    static Z5Simulator* get_z5_simulator(const aipu_global_config_simulation_t* cfg)
    {
        if (nullptr == m_sim)
        {
            m_sim = new Z5Simulator(cfg);
        }
        m_sim->inc_ref_cnt();
        return m_sim;
    }
    virtual ~Z5Simulator();
    Z5Simulator(const Z5Simulator& sim) = delete;
    Z5Simulator& operator=(const Z5Simulator& sim) = delete;

private:
    Z5Simulator(const aipu_global_config_simulation_t* cfg);
    static Z5Simulator* m_sim;
};

inline sim_aipu::config_t z5_sim_create_config(int code, uint32_t log_level = 0, bool verbose = false,
    bool enable_avx = false)
{
    sim_aipu::config_t config;
    config.code = code;
    config.enable_avx = enable_avx;
    config.log.filepath = "./sim.log";
    config.log.level = log_level;
    config.log.verbose = verbose;
    return config;
}

inline aipu_status_t get_code(uint32_t config, uint32_t* code)
{
    if (config == 1308)
    {
        *code = sim_aipu::config_t::Z5_1308;
    }
    else if (config == 1204)
    {
        *code = sim_aipu::config_t::Z5_1204;
    }
    else if (config == 1308)
    {
        *code = sim_aipu::config_t::Z5_1308_MP4;
    }
    else if (config == 901)
    {
        *code = sim_aipu::config_t::Z5_0901;
    }
    else
    {
        return AIPU_STATUS_ERROR_TARGET_NOT_FOUND;
    }
    return AIPU_STATUS_SUCCESS;
}
}

#endif /* _Z5_SIMULATOR_H_ */
