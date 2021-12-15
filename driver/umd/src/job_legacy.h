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
 * @file  job_legacy.h
 * @brief AIPU User Mode Driver (UMD) legacy job class header
 */

#ifndef _JOB_LEGACY_H_
#define _JOB_LEGACY_H_

#include <vector>
#include <pthread.h>
#include "standard_api.h"
#include "graph_legacy.h"
#include "job_base.h"
#include "type.h"

namespace aipudrv
{
class JobLegacy: public JobBase
{
private:
    DEV_PA_64 m_spc = 0;
    DEV_PA_64 m_intr_pc = 0;
    BufferDesc m_stack;
    std::vector<BufferDesc> m_reuses;
    std::vector<BufferDesc> m_weights; /* Do NOT free me in this class */
    std::string m_z1_sim;
    std::string m_z2_sim;
    std::string m_z3_sim;
    uint32_t m_log_level = 0;
    std::string m_data_dir;
    bool m_is_defer_run = false;
    bool m_do_trigger = false;
    uint32_t m_bind_core_id = 0;

private:
    const GraphLegacy& get_graph()
    {
        return static_cast<const GraphLegacy&>(m_graph);
    }
    aipu_status_t free_job_buffers();
    aipu_status_t setup_rodata_legacy();

public:
    virtual aipu_status_t init(const aipu_global_config_simulation_t* cfg);
    virtual aipu_status_t schedule();
    virtual aipu_status_t destroy();
    aipu_status_t config_simulation(uint64_t types, const aipu_job_config_simulation_t* config);
    aipu_status_t bind_core(uint32_t core_id);
    aipu_status_t debugger_run();

public:
    /* Set functions */
    void set_id(JOB_ID id)
    {
        m_id = id;
    }
    /* Get functions */
    JOB_ID get_id()
    {
        return m_id;
    }

public:
    JobLegacy(const GraphBase& graph, DeviceBase* dev);
    virtual ~JobLegacy();
    JobLegacy(const JobLegacy& job) = delete;
    JobLegacy& operator=(const JobLegacy& job) = delete;
};
}

#endif /* _JOB_LEGACY_H_ */