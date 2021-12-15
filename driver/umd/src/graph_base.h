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
 * @file  graph_base.h
 * @brief AIPU User Mode Driver (UMD) graph base module header
 */

#ifndef _GRAPH_BASE_H_
#define _GRAPH_BASE_H_

#include <fstream>
#include <map>
#include <pthread.h>
#include <assert.h>
#include "standard_api.h"
#include "device_base.h"
#include "memory_base.h"
#include "type.h"

namespace aipudrv
{
class JobBase;
class GraphBase
{
protected:
    GRAPH_ID m_id;
    uint32_t m_gversion = 0;
    uint32_t m_arch = 0;
    uint32_t m_hw_version = 0;
    uint32_t m_hw_config = 0;
    uint32_t m_hw_revision = 0;
    uint32_t m_asid_flag = 0;
    uint32_t m_remap_flag = 0;
    uint32_t m_sram_flag = 0;

protected:
    DeviceBase* m_dev;
    MemoryBase* m_mem;

protected:
    std::map<JOB_ID, JobBase*> m_jobs;
    pthread_rwlock_t m_lock;

protected:
    virtual JOB_ID create_job_id_inner();
    JOB_ID add_job(JobBase* job);
    aipu_status_t destroy_jobs();

public:
    virtual void print_parse_info() = 0;
    virtual aipu_status_t load(std::ifstream& gbin, uint32_t size, bool ver_check = true) = 0;
    virtual aipu_status_t unload() = 0;
    virtual aipu_status_t create_job(JOB_ID* id, const aipu_global_config_simulation_t* cfg) = 0;
    virtual aipu_status_t get_tensor_count(aipu_tensor_type_t type, uint32_t* cnt) = 0;
    virtual aipu_status_t get_tensor_descriptor(aipu_tensor_type_t type,
        uint32_t tensor, aipu_tensor_desc_t* desc) = 0;
    virtual DEV_PA_64 debugger_get_instr_base() = 0;

    JobBase* get_job(JOB_ID id)
    {
        JobBase* job = nullptr;
        pthread_rwlock_rdlock(&m_lock);
        job = (m_jobs.count(id) ? m_jobs[id]: nullptr);
        pthread_rwlock_unlock(&m_lock);
        return job;
    }
    aipu_status_t destroy_job(JOB_ID id);

public:
    /* Set functions */
    void set_gversion(uint32_t version)
    {
        m_gversion = version;
    }
    void set_arch(uint32_t arch)
    {
        m_arch = arch;
    }
    void set_hw_version(uint32_t hw_version)
    {
        m_hw_version = hw_version;
    }
    void set_hw_config(uint32_t hw_config)
    {
        m_hw_config = hw_config;
    }
    void set_hw_revision(uint32_t hw_revision)
    {
        m_hw_revision = hw_revision;
    }
    void set_asid_flag(uint32_t asid_flag)
    {
        m_asid_flag = asid_flag;
    }
    void set_sram_flag(uint32_t sram_flag)
    {
        m_sram_flag = sram_flag;
    }
    void set_remap_flag(uint32_t flag)
    {
        m_remap_flag = flag;
    }

    /* Get functions */
    uint32_t get_gversion()
    {
        return m_gversion;
    }
    uint32_t get_remap_flag()
    {
        return m_remap_flag;
    }
    uint32_t get_config()
    {
        return m_hw_config;
    }

public:
    GraphBase(GRAPH_ID id, DeviceBase* dev);
    virtual ~GraphBase();
    GraphBase(const GraphBase& base) = delete;
    GraphBase& operator=(const GraphBase& base) = delete;
};
}

#endif /* _GRAPH_BASE_H_ */
