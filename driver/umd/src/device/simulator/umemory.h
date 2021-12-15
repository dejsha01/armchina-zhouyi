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
 * @file  umemory.h
 * @brief AIPU User Mode Driver (UMD) userspace memory module header
 */

#ifndef _UMEMORY_H_
#define _UMEMORY_H_

#include "memory_base.h"
#include "simulator/mem_engine_base.h"

namespace aipudrv
{
class UMemory: public MemoryBase, public sim_aipu::IMemEngine
{
private:
    uint64_t m_base = 0;
    uint64_t m_size = 512 * MB_SIZE;
    uint64_t m_bit_cnt;
    bool*    m_bitmap;

private:
    uint32_t get_next_alinged_page_no(uint32_t start, uint32_t align);
    auto     get_allocated_buffer(uint64_t addr) const;

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
    virtual size_t size() const
    {
        return m_size;
    };
    virtual bool invalid(uint64_t addr) const
    {
        return (addr < m_base) || (addr >= (m_base + m_size));
    };

public:
    static UMemory* get_memory()
    {
        if (nullptr == m_mem)
        {
            m_mem = new UMemory();
        }
        return m_mem;
    }
    virtual ~UMemory();
    UMemory(const UMemory& mem) = delete;
    UMemory& operator=(const UMemory& mem) = delete;

private:
    UMemory();
    static UMemory* m_mem;
};
}

#endif /* _UMEMORY_H_ */
