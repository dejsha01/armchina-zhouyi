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
 * @file  memory_base.h
 * @brief AIPU User Mode Driver (UMD) memory base module header
 */

#ifndef _MEMORY_BASE_H_
#define _MEMORY_BASE_H_

#include <map>
#include <pthread.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include "standard_api.h"
#include "type.h"
#include "utils/log.h"

namespace aipudrv
{

#define PAGE_SIZE (4 * 1024)
#define MB_SIZE   (1 * 1024 * 1024)

struct BufferDesc
{
    DEV_PA_64 pa;       /**< device physical base address */
    uint64_t  size;     /**< buffer size */
    uint64_t  req_size; /**< requested size (<= buffer size) */
    uint64_t  dev_offset;
    void init(DEV_PA_64 _pa, uint64_t _size, uint64_t _req_size, uint64_t _offset = 0)
    {
        pa = _pa;
        size = _size;
        req_size = _req_size;
        dev_offset = _offset;
    }
    void reset()
    {
        pa = 0;
        size = 0;
        req_size = 0;
        dev_offset = 0;
    }
};

struct Buffer
{
    char* va;
    BufferDesc desc;
    void init(char* _va, BufferDesc _desc)
    {
        va = _va;
        desc = _desc;
    }
};

enum MemOperation
{
    MemOperationAlloc,
    MemOperationFree,
    MemOperationRead,
    MemOperationWrite,
    MemOperationBzero,
    MemOperationDump,
    MemOperationReload,
    MemOperationCnt,
};

struct MemTracking
{
    DEV_PA_64    pa;
    uint64_t     size;
    MemOperation op;
    std::string  log;
    bool         is_32_op;
    uint32_t     data;
    void init(DEV_PA_64 _pa, uint64_t _size, MemOperation _op,
        const char* str)
    {
        pa = _pa;
        size = _size;
        op = _op;
        is_32_op = false;
        log = std::string(str);
    }
    void init(DEV_PA_64 _pa, uint64_t _size, MemOperation _op,
        std::string str)
    {
        pa = _pa;
        size = _size;
        op = _op;
        is_32_op = false;
        log = str;
    }
};

class MemoryBase
{
private:
    const char* MemOperationStr[MemOperationCnt] = {
        "alloc",
        "free",
        "read",
        "write",
        "bzero",
        "dump",
        "reload",
    };

private:
    mutable std::ofstream mem_dump;
    mutable std::vector<MemTracking> m_tracking;
    mutable uint32_t m_tracking_idx = 0;
    mutable pthread_rwlock_t m_tlock;
    mutable uint32_t start = 0;
    mutable uint32_t end = 0;
    uint32_t m_enable_mem_dump = RTDEBUG_TRACKING_MEM_OPERATION;
    std::string m_file_name = "mem_info.log";

protected:
    std::map<DEV_PA_64, Buffer> m_allocated;
    mutable pthread_rwlock_t m_lock;

private:
    std::string get_tracking_log(DEV_PA_64 pa) const;
    auto get_allocated_buffer(uint64_t addr) const;

protected:
    uint64_t get_page_cnt(uint64_t bytes) const
    {
        return ceil((double)bytes/PAGE_SIZE);
    }

    uint64_t get_page_no(DEV_PA_64 pa) const
    {
        return floor((double)pa/PAGE_SIZE);
    }

    void add_tracking(DEV_PA_64 pa, uint64_t size, MemOperation op,
        const char* str, bool is_32_op, uint32_t data) const;
    int mem_read(uint64_t addr, void *dest, size_t size) const;
    int mem_write(uint64_t addr, const void *src, size_t size);
    int mem_bzero(uint64_t addr, size_t size);

public:
    void dump_tracking_log_start() const;
    void dump_tracking_log_end() const;
    void write_line(const char* log) const;

public:
    /* Interfaces */
    int pa_to_va(uint64_t addr, uint64_t size, char** va) const;
    virtual aipu_status_t malloc(uint32_t size, uint32_t align, BufferDesc* buf, const char* str = nullptr) = 0;
    virtual aipu_status_t free(const BufferDesc* buf, const char* str = nullptr) = 0;
    virtual int read(uint64_t addr, void *dest, size_t size) const = 0;
    virtual int write(uint64_t addr, const void *src, size_t size) = 0;
    virtual int bzero(uint64_t addr, size_t size) = 0;
    virtual aipu_status_t dump_file(DEV_PA_64 src, const char* name, uint32_t size);
    virtual aipu_status_t load_file(DEV_PA_64 dest, const char* name, uint32_t size);
    int write32(DEV_PA_64 dest, uint32_t src)
    {
        return mem_write(dest, (char*)&src, sizeof(src));
    }
    int read32(uint32_t* desc, DEV_PA_64 src)
    {
        return mem_read(src, (char*)desc, sizeof(*desc));
    }

public:
    MemoryBase();
    virtual ~MemoryBase();
    MemoryBase(const MemoryBase& mem) = delete;
    MemoryBase& operator=(const MemoryBase& mem) = delete;
};
}

#endif /* _MEMORY_BASE_H_ */
