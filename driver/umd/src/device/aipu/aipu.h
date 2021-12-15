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
 * @file  aipu.h
 * @brief AIPU User Mode Driver (UMD) aipu module header
 */

#ifndef _AIPU_H_
#define _AIPU_H_

#include <vector>

#include "device_base.h"
#include "type.h"

namespace aipudrv
{
class Aipu : public DeviceBase
{
protected:
    int m_fd = 0;
    std::vector<aipu_core_cap> m_core_caps;

private:
    aipu_ll_status_t init();
    void deinit();

public:
    virtual bool has_target(uint32_t arch, uint32_t version, uint32_t config, uint32_t rev);
    virtual aipu_status_t schedule(const JobDesc& job);
    virtual aipu_ll_status_t read_reg(uint32_t core_id, uint32_t offset, uint32_t* value);
    virtual aipu_ll_status_t write_reg(uint32_t core_id, uint32_t offset, uint32_t value);
    aipu_ll_status_t get_status(std::vector<aipu_job_status_desc>& jobs_status,
        uint32_t max_cnt, bool of_this_thread);
    virtual aipu_ll_status_t get_status(std::vector<aipu_job_status_desc>& jobs_status,
        uint32_t max_cnt);
    virtual aipu_ll_status_t poll_status(std::vector<aipu_job_status_desc>& jobs_status,
        uint32_t max_cnt, int32_t time_out, bool of_this_thread);

public:
    static aipu_status_t get_aipu(DeviceBase** dev)
    {
        aipu_status_t ret = AIPU_STATUS_SUCCESS;

        if (nullptr == dev)
        {
            return AIPU_STATUS_ERROR_NULL_PTR;
        }

        if (m_aipu == nullptr)
        {
            m_aipu = new Aipu();
            ret = convert_ll_status(m_aipu->init());
            if (ret != AIPU_STATUS_SUCCESS)
            {
                delete m_aipu;
                m_aipu = nullptr;
                return ret;
            }
        }

        m_aipu->inc_ref_cnt();
        *dev = m_aipu;
        return AIPU_STATUS_SUCCESS;
    }
    virtual ~Aipu();
    Aipu(const Aipu& dev) = delete;
    Aipu& operator=(const Aipu& dev) = delete;

private:
    Aipu();
    static Aipu* m_aipu;
};
}

#endif /* _AIPU_H_ */
