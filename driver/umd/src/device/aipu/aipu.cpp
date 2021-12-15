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
 * @file  aipu.cpp
 * @brief AIPU User Mode Driver (UMD) aipu module header
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <assert.h>
#include "aipu.h"
#include "ukmemory.h"

aipudrv::Aipu* aipudrv::Aipu::m_aipu = nullptr;

aipudrv::Aipu::Aipu()
{
    m_dev_type = DEV_TYPE_AIPU;
}

aipudrv::Aipu::~Aipu()
{
    deinit();
}

aipu_ll_status_t aipudrv::Aipu::init()
{
    aipu_ll_status_t ret = AIPU_LL_STATUS_SUCCESS;
    int kret = 0;
    aipu_cap cap;
    aipu_core_cap *core_caps = NULL;

    m_fd = open("/dev/aipu", O_RDWR | O_SYNC);
    if (m_fd <= 0)
    {
        m_fd = 0;
        return AIPU_LL_STATUS_ERROR_OPEN_FAIL;
    }

    kret = ioctl(m_fd, AIPU_IOCTL_QUERY_CAP, &cap);
    if (kret || (0 == cap.core_cnt))
    {
        ret = AIPU_LL_STATUS_ERROR_IOCTL_QUERY_CAP_FAIL;
        goto fail;
    }

    if (cap.is_homogeneous)
    {
        m_core_caps.push_back(cap.core_cap);
    }
    else
    {
        core_caps = new aipu_core_cap[cap.core_cnt];
        kret = ioctl(m_fd, AIPU_IOCTL_QUERY_CORE_CAP, core_caps);
        if (kret)
        {
            delete[] core_caps;
            ret = AIPU_LL_STATUS_ERROR_IOCTL_QUERY_CORE_CAP_FAIL;
            goto fail;
        }
        for (uint32_t i = 0; i < cap.core_cnt; i++)
        {
            m_core_caps.push_back(core_caps[i]);
        }
        delete[] core_caps;
    }

    m_dram = UKMemory::get_memory(m_fd);

    /* success */
    m_core_cnt = cap.core_cnt;
    goto finish;

fail:
    close(m_fd);

finish:
    return ret;
}

void aipudrv::Aipu::deinit()
{
    if (m_dram != nullptr)
    {
        delete m_dram;
        m_dram = nullptr;
    }

    if (m_fd > 0)
    {
        close(m_fd);
        m_fd = 0;
    }
}

bool aipudrv::Aipu::has_target(uint32_t arch, uint32_t version, uint32_t config, uint32_t rev)
{
    for (uint32_t i = 0; i < m_core_caps.size(); i++)
    {
        if ((arch == m_core_caps[i].arch) &&
            (version == m_core_caps[i].version) &&
            (config == m_core_caps[i].config))
        {
            return true;
        }
    }
    return false;
}

aipu_ll_status_t aipudrv::Aipu::read_reg(uint32_t core_id, uint32_t offset, uint32_t* value)
{
    int kret = 0;
    aipu_io_req ioreq;

    if (nullptr == value)
    {
        return AIPU_LL_STATUS_ERROR_NULL_PTR;
    }

    ioreq.core_id = core_id;
    ioreq.rw = aipu_io_req::AIPU_IO_READ;
    ioreq.offset = offset;
    kret = ioctl(m_fd, AIPU_IOCTL_REQ_IO, &ioreq);
    if (kret)
    {
        return AIPU_LL_STATUS_ERROR_IOCTL_REQ_IO_FAIL;
    }

    /* success */
    *value = ioreq.value;
    return AIPU_LL_STATUS_SUCCESS;
}

aipu_ll_status_t aipudrv::Aipu::write_reg(uint32_t core_id, uint32_t offset, uint32_t value)
{
    int kret = 0;
    aipu_io_req ioreq;

    ioreq.core_id = core_id;
    ioreq.rw = aipu_io_req::AIPU_IO_WRITE;
    ioreq.offset = offset;
    ioreq.value = value;
    kret = ioctl(m_fd, AIPU_IOCTL_REQ_IO, &ioreq);
    if (kret)
    {
        return AIPU_LL_STATUS_ERROR_IOCTL_REQ_IO_FAIL;
    }

    return AIPU_LL_STATUS_SUCCESS;
}

aipu_status_t aipudrv::Aipu::schedule(const JobDesc& job)
{
    int kret = 0;

    kret = ioctl(m_fd, AIPU_IOCTL_SCHEDULE_JOB, &job.kdesc);
    if (kret)
    {
        return AIPU_STATUS_ERROR_INVALID_OP;
    }

    return AIPU_STATUS_SUCCESS;
}

aipu_ll_status_t aipudrv::Aipu::get_status(std::vector<aipu_job_status_desc>& jobs_status,
    uint32_t max_cnt, bool of_this_thread)
{
    aipu_ll_status_t ret = AIPU_LL_STATUS_SUCCESS;
    int kret = 0;
    aipu_job_status_query status_query;

    assert(max_cnt > 0);

    status_query.of_this_thread = of_this_thread;
    status_query.max_cnt = max_cnt;
    status_query.status = new aipu_job_status_desc[max_cnt];
    kret = ioctl(m_fd, AIPU_IOCTL_QUERY_STATUS, &status_query);
    if (kret)
    {
        ret = AIPU_LL_STATUS_ERROR_IOCTL_QUERY_STATUS_FAIL;
        goto clean;
    }

    for (uint32_t i = 0; i < status_query.poll_cnt; i++)
    {
        jobs_status.push_back(status_query.status[i]);
    }

clean:
    delete[] status_query.status;
    return ret;
}

aipu_ll_status_t aipudrv::Aipu::get_status(std::vector<aipu_job_status_desc>& jobs_status,
    uint32_t max_cnt)
{
    return poll_status(jobs_status, max_cnt, 0, true);
}

aipu_ll_status_t aipudrv::Aipu::poll_status(std::vector<aipu_job_status_desc>& jobs_status,
    uint32_t max_cnt, int32_t time_out, bool of_this_thread)
{
    aipu_ll_status_t ret = AIPU_LL_STATUS_SUCCESS;
    int kret = 0;
    struct pollfd poll_list;
    poll_list.fd = m_fd;
    poll_list.events = POLLIN | POLLPRI;

    assert(max_cnt > 0);

    kret = poll(&poll_list, 1, time_out);
    if (kret < 0)
    {
        return AIPU_LL_STATUS_ERROR_POLL_FAIL;
    }

    if ((poll_list.revents & POLLIN) == POLLIN)
    {
        ret = get_status(jobs_status, max_cnt, of_this_thread);
    }

    return ret;
}