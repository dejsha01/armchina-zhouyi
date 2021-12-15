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
 * @file  device_base.h
 * @brief AIPU User Mode Driver (UMD) device module header
 */

#ifndef _DEVICE_BASE_H_
#define _DEVICE_BASE_H_

#include "kmd/armchina_aipu.h"
#include "memory_base.h"
#include "type.h"

typedef enum {
    AIPU_LL_STATUS_SUCCESS,
    AIPU_LL_STATUS_ERROR_OPERATION_UNSUPPORTED,
    AIPU_LL_STATUS_ERROR_NULL_PTR,
    AIPU_LL_STATUS_ERROR_OPEN_FAIL,
    AIPU_LL_STATUS_ERROR_IOCTL_QUERY_CAP_FAIL,
    AIPU_LL_STATUS_ERROR_IOCTL_QUERY_CORE_CAP_FAIL,
    AIPU_LL_STATUS_ERROR_IOCTL_REQ_IO_FAIL,
    AIPU_LL_STATUS_ERROR_POLL_FAIL,
    AIPU_LL_STATUS_ERROR_IOCTL_QUERY_STATUS_FAIL,
} aipu_ll_status_t;

inline aipu_status_t convert_ll_status(aipu_ll_status_t status)
{
    switch(status)
    {
        case AIPU_LL_STATUS_SUCCESS:
            return AIPU_STATUS_SUCCESS;

        case AIPU_LL_STATUS_ERROR_OPERATION_UNSUPPORTED:
            return AIPU_STATUS_ERROR_OP_NOT_SUPPORTED;

        case AIPU_LL_STATUS_ERROR_NULL_PTR:
            return AIPU_STATUS_ERROR_NULL_PTR;

        case AIPU_LL_STATUS_ERROR_OPEN_FAIL:
            return AIPU_STATUS_ERROR_OPEN_DEV_FAIL;

        case AIPU_LL_STATUS_ERROR_IOCTL_QUERY_CAP_FAIL:
        case AIPU_LL_STATUS_ERROR_IOCTL_QUERY_CORE_CAP_FAIL:
        case AIPU_LL_STATUS_ERROR_IOCTL_REQ_IO_FAIL:
        case AIPU_LL_STATUS_ERROR_POLL_FAIL:
        case AIPU_LL_STATUS_ERROR_IOCTL_QUERY_STATUS_FAIL:
            return AIPU_STATUS_ERROR_DEV_ABNORMAL;

        default:
            return AIPU_STATUS_SUCCESS;
    }
}

namespace aipudrv
{
struct JobDesc
{
    /* job descriptor for KMD, part of the members shared with x86 simulation */
    struct aipu_job_desc kdesc;

    /* shared */
    uint32_t aipu_revision;

    /* z5 only */
    DEV_PA_64 tcb_head;
    DEV_PA_64 tcb_tail;

    /* z1/2/3 only */
    DEV_PA_64 instruction_base_pa;

    /* z1/2/3 simulation only */
    uint32_t text_size;
    DEV_PA_64 weight_pa;
    uint32_t weight_size;
    uint32_t rodata_size;
    DEV_PA_64 dcr_pa;
    uint32_t dcr_size;
    uint32_t stack_size;
    std::vector<BufferDesc> reuses;
    std::vector<BufferDesc> outputs;
    std::string output_dir;
    std::string simulator;
    uint32_t log_level;
};

enum DeviceType
{
    DEV_TYPE_NONE             = 0,
    DEV_TYPE_SIMULATOR_LEGACY = 1,
    DEV_TYPE_SIMULATOR_Z5     = 2,
    DEV_TYPE_AIPU             = 3,
};

class DeviceBase
{
protected:
    DeviceType  m_dev_type = DEV_TYPE_NONE;
    MemoryBase* m_dram = nullptr;
    uint32_t m_cluster_cnt = 1;
    uint32_t m_core_cnt = 1;
    uint32_t m_ref_cnt = 0;

public:
    virtual bool has_target(uint32_t arch, uint32_t version, uint32_t config, uint32_t rev) = 0;
    virtual uint32_t tec_cnt_per_core(uint32_t config)
    {
        return config % 100;
    };
    virtual aipu_status_t schedule(const JobDesc& job) = 0;
    virtual aipu_status_t get_simulation_instance(void** simulator, void** memory)
    {
        return AIPU_STATUS_ERROR_INVALID_OP;
    }
    MemoryBase* get_mem()
    {
        return m_dram;
    }
    virtual aipu_ll_status_t read_reg(uint32_t core_id, uint32_t offset, uint32_t* value)
    {
        return AIPU_LL_STATUS_ERROR_OPERATION_UNSUPPORTED;
    }
    virtual aipu_ll_status_t write_reg(uint32_t core_id, uint32_t offset, uint32_t value)
    {
        return AIPU_LL_STATUS_ERROR_OPERATION_UNSUPPORTED;
    }
    /* non-blocking */
    virtual aipu_ll_status_t get_status(std::vector<aipu_job_status_desc>& jobs_status,
        uint32_t max_cnt)
    {
        return AIPU_LL_STATUS_SUCCESS;
    }
    /* blocking with timeout */
    virtual aipu_ll_status_t poll_status(std::vector<aipu_job_status_desc>& jobs_status,
        uint32_t max_cnt, int32_t time_out, bool of_this_thread)
    {
        return AIPU_LL_STATUS_SUCCESS;
    }
    int dec_ref_cnt()
    {
        return --m_ref_cnt;
    }
    int inc_ref_cnt()
    {
        return ++m_ref_cnt;
    }
    virtual int get_config_code()
    {
        return 0;
    }

public:
    aipu_status_t get_cluster_count(uint32_t* cnt)
    {
        if (cnt != nullptr)
        {
            if (m_cluster_cnt != 0)
            {
                *cnt = m_cluster_cnt;
                return AIPU_STATUS_SUCCESS;
            }
            return AIPU_STATUS_ERROR_INVALID_OP;
        }
        return AIPU_STATUS_ERROR_NULL_PTR;
    };
    aipu_status_t get_core_count(uint32_t cluster, uint32_t* cnt)
    {
        if (cluster >= m_cluster_cnt)
        {
            return AIPU_STATUS_ERROR_INVALID_CLUSTER_ID;
        }

        /* homogeneous */
        if (cnt != nullptr)
        {
            if (m_core_cnt != 0)
            {
                *cnt = m_core_cnt;
                return AIPU_STATUS_SUCCESS;
            }
            return AIPU_STATUS_ERROR_INVALID_OP;
        }
        return AIPU_STATUS_ERROR_NULL_PTR;
    };
    DeviceType get_dev_type()
    {
        return m_dev_type;
    }

public:
    DeviceBase(){};
    virtual ~DeviceBase(){};
    DeviceBase(const DeviceBase& dev) = delete;
    DeviceBase& operator=(const DeviceBase& dev) = delete;
};
}

#endif /* _DEVICE_BASE_H_ */
