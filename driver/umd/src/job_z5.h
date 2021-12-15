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
 * @file  job_z5.h
 * @brief AIPU User Mode Driver (UMD) job class header
 */

#ifndef _JOB_Z5_H_
#define _JOB_Z5_H_

#include <vector>
#include <pthread.h>
#include "graph_z5.h"
#include "kmd/tcb.h"
#include "job_base.h"

namespace aipudrv
{
struct TCB
{
    DEV_PA_64 pa;
    void init(DEV_PA_64 _pa)
    {
        pa = _pa;
    }
};

struct Task
{
    TCB        tcb;
    BufferDesc stack;
    BufferDesc dp_cc;
};

struct SubGraphTask
{
    std::vector<BufferDesc> reuses;
    std::vector<BufferDesc> weights;
    std::vector<Task>       tasks;
    void reset()
    {
        reuses.clear();
        weights.clear();
        tasks.clear();
    }
};

class JobZ5: public JobBase
{
private:
    uint32_t    m_tot_tcb_cnt = 0;
    uint32_t    m_sg_cnt = 0;
    uint32_t    m_task_per_sg = 0;

private:
    BufferDesc m_tcbs;
    TCB        m_init_tcb;
    std::vector<SubGraphTask>       m_sg_job;

private:
    const aipu_global_config_simulation_t* m_cfg;

private:
    const GraphZ5& get_graph()
    {
        return static_cast<const GraphZ5&>(m_graph);
    }

private:
    aipu_status_t setup_rodata_sg(uint32_t sg_id);
    aipu_status_t setup_tcb_task(uint32_t sg_id, uint32_t task_id);
    aipu_status_t setup_tcb_sg(uint32_t sg_id);
    void          set_job_params(uint32_t sg_cnt, uint32_t task_per_sg, uint32_t remap);
    aipu_status_t alloc_load_job_buffers();
    aipu_status_t free_job_buffers();
    aipu_status_t setup_tcbs();
    void free_sg_buffers(const SubGraphTask& sg);
    void dump_z5_specific_buffers();
    aipu_status_t dump_for_emulation();

public:
    aipu_status_t init(const aipu_global_config_simulation_t* cfg);
    aipu_status_t schedule();
    aipu_status_t destroy();
    aipu_status_t bind_core(uint32_t core_id)
    {
        return AIPU_STATUS_ERROR_OP_NOT_SUPPORTED;
    }

public:
    /* Set functions */

    /* Get functions */
    JOB_ID get_id()
    {
        return m_id;
    }

public:
    JobZ5(const GraphBase& graph, DeviceBase* dev);
    ~JobZ5();
    JobZ5(const JobZ5& job) = delete;
    JobZ5& operator=(const JobZ5& job) = delete;
};
}

#endif /* _JOB_Z5_H_ */