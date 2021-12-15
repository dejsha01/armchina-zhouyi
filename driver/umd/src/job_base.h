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
 * @file  job_base.h
 * @brief AIPU User Mode Driver (UMD) job base class header
 */

#ifndef _JOB_BASE_H_
#define _JOB_BASE_H_

#include <vector>
#include <pthread.h>
#include "standard_api.h"
#include "graph.h"
#include "device_base.h"
#include "memory_base.h"
#include "type.h"

namespace aipudrv
{
struct JobIOBuffer
{
    uint32_t  id;
    uint32_t  size;
    DEV_PA_64 pa;
    void init(uint32_t _id, uint32_t _size, DEV_PA_64 _pa)
    {
        id   = _id;
        size = _size;
        pa   = _pa;
    }
};

typedef enum {
    AIPU_JOB_STATUS_INIT  = 3,
    AIPU_JOB_STATUS_SCHED = 4,
    AIPU_JOB_STATUS_BIND  = 5,
} aipu_job_status_internal_t;

class JobBase
{
protected:
    JOB_ID            m_id;
    const GraphBase&  m_graph;
    DeviceBase*       m_dev;
    MemoryBase*       m_mem;
    uint32_t          m_remap_flag = 0;

protected:
    /* shared buffers */
    BufferDesc m_rodata;
    BufferDesc m_descriptor;
    std::vector<struct JobIOBuffer> m_inputs;
    std::vector<struct JobIOBuffer> m_outputs;
    std::vector<struct JobIOBuffer> m_inter_dumps;
    std::vector<struct JobIOBuffer> m_profiler;
    std::vector<struct JobIOBuffer> m_printf;
    std::vector<struct JobIOBuffer> m_layer_counter;

protected:
    bool m_dump_text = false;
    bool m_dump_weight = false;
    bool m_dump_rodata = false;
    bool m_dump_dcr = false;
    bool m_dump_input = false;
    bool m_dump_output = false;
    bool m_dump_tcb = false;
    bool m_dump_emu = false;
    std::string m_dump_dir = "./";
    std::string m_dump_prefix = "temp";

protected:
    uint32_t m_status = AIPU_JOB_STATUS_NO_STATUS;

private:
    DEV_PA_64 get_base_pa(int sec_type, BufferDesc& rodata,
        BufferDesc& descriptor);
    void create_io_buffers(std::vector<struct JobIOBuffer>& bufs,
        const std::vector<GraphIOTensorDesc>& desc,
        const std::vector<BufferDesc>& reuses);

protected:
    aipu_status_t setup_rodata(
        const std::vector<struct GraphParamMapLoadDesc>& param_map,
        const std::vector<BufferDesc>& reuse_buf,
        const std::vector<BufferDesc>& static_buf,
        BufferDesc rodata, BufferDesc dcr);
    virtual const Graph& get_graph()
    {
        return static_cast<const Graph&>(m_graph);
    }
    void setup_remap(BufferDesc& rodata, BufferDesc& descriptor);
    void create_io_buffers(const struct GraphIOTensors& io,
        const std::vector<BufferDesc>& reuses);
    void dump_buffer(DEV_PA_64 pa, const char* bin_va, uint32_t size, const char* name);
    void dump_job_shared_buffers();
    void dump_job_private_buffers(BufferDesc& rodata, BufferDesc& descriptor);
    void dump_job_private_buffers_after_run(BufferDesc& rodata, BufferDesc& descriptor);
    aipu_status_t validate_schedule_status();

public:
    virtual aipu_status_t init(const aipu_global_config_simulation_t* cfg) = 0;
    virtual aipu_status_t schedule() = 0;
    virtual aipu_status_t destroy() = 0;
    aipu_status_t load_tensor(uint32_t tensor, const void* data);
    aipu_status_t get_tensor(aipu_tensor_type_t type, uint32_t tensor, void* data);
    virtual aipu_status_t get_status(aipu_job_status_t* status);
    virtual aipu_status_t get_status_blocking(aipu_job_status_t* status, int32_t time_out);
    aipu_status_t config_mem_dump(uint64_t types, const aipu_job_config_dump_t* config);
    virtual aipu_status_t config_simulation(uint64_t types, const aipu_job_config_simulation_t* config)
    {
        return AIPU_STATUS_SUCCESS;
    };
    virtual aipu_status_t bind_core(uint32_t core_id) = 0;
    virtual aipu_status_t debugger_run()
    {
        return AIPU_STATUS_ERROR_OP_NOT_SUPPORTED;
    }

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
    JobBase(const GraphBase& graph, DeviceBase* dev);
    virtual ~JobBase();
    JobBase(const JobBase& job) = delete;
    JobBase& operator=(const JobBase& job) = delete;
};
}

#endif /* _JOB_BASE_H_ */