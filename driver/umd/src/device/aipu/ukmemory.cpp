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
 * @file  ukmemory.cpp
 * @brief AIPU User Mode Driver (UMD) user & kernel space memory module implementation
 */

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <cstring>
#include <assert.h>
#include "kmd/armchina_aipu.h"
#include "ukmemory.h"
#include "utils/log.h"
#include "utils/helper.h"

aipudrv::UKMemory* aipudrv::UKMemory::m_mem = nullptr;

aipudrv::UKMemory::UKMemory(int fd): MemoryBase()
{
    m_fd = fd;
}

aipudrv::UKMemory::~UKMemory()
{
    auto bm_iter = m_allocated.begin();
    for (; bm_iter != m_allocated.end(); bm_iter++)
    {
        free(&bm_iter->second.desc, nullptr);
    }
    m_allocated.clear();
}

aipu_status_t aipudrv::UKMemory::malloc(uint32_t size, uint32_t align, BufferDesc* desc, const char* str)
{
    int kret = 0;
    Buffer buf;
    aipu_buf_request buf_req;
    buf_req.bytes = size;
    buf_req.align_in_page = (align == 0) ? 1: align;
    char* ptr = nullptr;

    assert(desc != nullptr);

    if (0 == size)
    {
        return AIPU_STATUS_ERROR_INVALID_SIZE;
    }

    kret = ioctl(m_fd, AIPU_IOCTL_REQ_BUF, &buf_req);
    if (kret != 0)
    {
        return AIPU_STATUS_ERROR_BUF_ALLOC_FAIL;
    }

    ptr = (char*)mmap(NULL, buf_req.desc.bytes, PROT_READ | PROT_WRITE, MAP_SHARED,
        m_fd, buf_req.desc.dev_offset);
    if (ptr == MAP_FAILED)
    {
        ioctl(m_fd, AIPU_IOCTL_FREE_BUF, &buf_req.desc);
        return AIPU_STATUS_ERROR_BUF_ALLOC_FAIL;
    }

    /* success */
    desc->init(buf_req.desc.pa, buf_req.desc.bytes, size, buf_req.desc.dev_offset);
    buf.init(ptr, *desc);
    pthread_rwlock_wrlock(&m_lock);
    m_allocated[buf_req.desc.pa] = buf;
    pthread_rwlock_unlock(&m_lock);

#if RTDEBUG_TRACKING_MEM_OPERATION
    add_tracking(buf_req.desc.pa, size, MemOperationAlloc, str, false, 0);
#endif

    return AIPU_STATUS_SUCCESS;
}

aipu_status_t aipudrv::UKMemory::free(const BufferDesc* desc, const char* str)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    int kret = 0;
    aipu_buf_desc kdesc;
    auto iter = m_allocated.begin();

    assert(desc != nullptr);

    pthread_rwlock_wrlock(&m_lock);
    iter = m_allocated.find(desc->pa);
    if ((iter == m_allocated.end()) ||
        (iter->second.desc.size != desc->size))
    {
        ret = AIPU_STATUS_ERROR_BUF_FREE_FAIL;
        goto unlock;
    }

    kdesc.pa = desc->pa;
    kdesc.bytes = desc->size;
    munmap(iter->second.va, kdesc.bytes);
    kret = ioctl(m_fd, AIPU_IOCTL_FREE_BUF, &kdesc);
    if (kret != 0)
    {
        ret = AIPU_STATUS_ERROR_BUF_FREE_FAIL;
        goto unlock;
    }

    m_allocated.erase(desc->pa);

unlock:
    pthread_rwlock_unlock(&m_lock);

#if RTDEBUG_TRACKING_MEM_OPERATION
    if (ret == AIPU_STATUS_SUCCESS)
    {
        add_tracking(desc->pa, desc->size, MemOperationFree, str, false, 0);
    }
#endif

    return ret;
}
