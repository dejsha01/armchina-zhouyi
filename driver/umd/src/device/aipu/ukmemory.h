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
 * @file  ukmemory.h
 * @brief AIPU User Mode Driver (UMD) user & kernel space memory module header
 */

#ifndef _UKMEMORY_H_
#define _UKMEMORY_H_

#include <map>
#include <pthread.h>
#include "memory_base.h"

namespace aipudrv
{
class UKMemory: public MemoryBase
{
private:
    int m_fd = 0;

public:
    virtual aipu_status_t malloc(uint32_t size, uint32_t align, BufferDesc* desc, const char* str = nullptr);
    virtual aipu_status_t free(const BufferDesc* desc, const char* str = nullptr);
    virtual int read(uint64_t addr, void *dest, size_t size) const
    {
        return mem_read(addr, dest, size);
    };
    virtual int write(uint64_t addr, const void *src, size_t size)
    {
        return mem_write(addr, src, size);
    };
    virtual int bzero(uint64_t addr, size_t size)
    {
        return mem_bzero(addr, size);
    };

public:
    static UKMemory* get_memory(int fd)
    {
        if (nullptr == m_mem)
        {
            m_mem = new UKMemory(fd);
        }
        return m_mem;
    }
    virtual ~UKMemory();
    UKMemory(const UKMemory& mem) = delete;
    UKMemory& operator=(const UKMemory& mem) = delete;

private:
    UKMemory(int fd);
    static UKMemory* m_mem;
};
}

#endif /* _UKMEMORY_H_ */
