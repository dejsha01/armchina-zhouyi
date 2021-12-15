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
 * @file  umemory.cpp
 * @brief AIPU User Mode Driver (UMD) userspace memory module implementation
 */

#include <unistd.h>
#include <cstring>
#include <assert.h>
#include "umemory.h"
#include "utils/log.h"
#include "utils/helper.h"

aipudrv::UMemory* aipudrv::UMemory::m_mem = nullptr;

aipudrv::UMemory::UMemory(): MemoryBase(), sim_aipu::IMemEngine()
{
    assert((m_base % PAGE_SIZE) == 0);
    assert((m_size % PAGE_SIZE) == 0);

    m_bit_cnt = m_size / PAGE_SIZE;
    m_bitmap = new bool[m_bit_cnt];
    memset(m_bitmap, 1, m_bit_cnt);
}

aipudrv::UMemory::~UMemory()
{
    auto bm_iter = m_allocated.begin();
    delete[] m_bitmap;
    for (; bm_iter != m_allocated.end(); bm_iter++)
    {
        delete[] bm_iter->second.va;
    }
}

uint32_t aipudrv::UMemory::get_next_alinged_page_no(uint32_t start, uint32_t align)
{
    uint32_t no = start;

    while (no < m_bit_cnt)
    {
        uint64_t pa = m_base + no * PAGE_SIZE;
        if ((pa % (align * PAGE_SIZE)) == 0)
        {
            return no;
        }
        no++;
    }
    return m_bit_cnt;
}

aipu_status_t aipudrv::UMemory::malloc(uint32_t size, uint32_t align, BufferDesc* desc, const char* str)
{
    aipu_status_t ret = AIPU_STATUS_ERROR_BUF_ALLOC_FAIL;
    Buffer buf;
    uint64_t malloc_size, malloc_page = 0;
    uint64_t i = 0;

    if (0 == size)
    {
        return AIPU_STATUS_ERROR_INVALID_SIZE;
    }

    if (nullptr == desc)
    {
        return AIPU_STATUS_ERROR_NULL_PTR;
    }

    if (0 == align)
    {
        align = 1;
    }

    malloc_page = get_page_cnt(size);
    malloc_size = malloc_page * PAGE_SIZE;

    if (malloc_page > m_bit_cnt)
    {
        return AIPU_STATUS_ERROR_BUF_ALLOC_FAIL;
    }

    pthread_rwlock_wrlock(&m_lock);
    i = get_next_alinged_page_no(0, align);
    while ((i + malloc_page) < m_bit_cnt)
    {
        uint64_t j = i;
        for (; j < (i + malloc_page); j++)
        {
            if (m_bitmap[j] == 0)
            {
                i = get_next_alinged_page_no(j + 1, align);
                break;
            }
        }

        if (j == i + malloc_page)
        {
            desc->init(m_base + i * 4096, malloc_size, size);
            buf.desc = *desc;
            buf.va = new char[malloc_size];
            memset(buf.va, 0, malloc_size);
            m_allocated[desc->pa] = buf;
            for (uint32_t j = 0; j < malloc_page; j++)
            {
                m_bitmap[i + j] = 0;
            }
            ret = AIPU_STATUS_SUCCESS;
            break;
        }
    }
    pthread_rwlock_unlock(&m_lock);

#if RTDEBUG_TRACKING_MEM_OPERATION
    if (ret == AIPU_STATUS_SUCCESS)
    {
        add_tracking(desc->pa, desc->size, MemOperationAlloc, str, false, 0);
    }
#endif

    return ret;
}

aipu_status_t aipudrv::UMemory::free(const BufferDesc* desc, const char* str)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    uint64_t b_start, b_end;
    auto iter = m_allocated.begin();

    if (nullptr == desc)
    {
        return AIPU_STATUS_ERROR_NULL_PTR;
    }

    if (0 == desc->size)
    {
        return AIPU_STATUS_ERROR_INVALID_SIZE;
    }

    pthread_rwlock_wrlock(&m_lock);
    iter = m_allocated.find(desc->pa);
    if (iter == m_allocated.end())
    {
        ret = AIPU_STATUS_ERROR_BUF_FREE_FAIL;
        goto unlock;
    }

    b_start = iter->second.desc.pa/4096;
    b_end = b_start + iter->second.desc.size/4096;
    for (uint64_t i = b_start; i < b_end; i++)
    {
        m_bitmap[i] = 1;
    }
    delete[] iter->second.va;
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
