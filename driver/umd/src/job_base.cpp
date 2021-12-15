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
 * @file  job_base.cpp
 * @brief AIPU User Mode Driver (UMD) job base module implementation
 */

#include <cstring>
#include <assert.h>
#include "job_base.h"
#include "utils/helper.h"

aipudrv::JobBase::JobBase(const GraphBase& graph, DeviceBase* dev):
    m_graph(graph), m_dev(dev)
{
    m_mem = m_dev->get_mem();
    m_rodata.reset();
    m_descriptor.reset();
}

aipudrv::JobBase::~JobBase()
{
}

aipu_status_t aipudrv::JobBase::get_status(aipu_job_status_t* status)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    std::vector<aipu_job_status_desc> jobs_status;

    ret = convert_ll_status(m_dev->get_status(jobs_status, 1));
    if (ret != AIPU_STATUS_SUCCESS)
    {
        return ret;
    }

    if (jobs_status.size() != 0)
    {
        m_status = jobs_status[0].state;
    }

    if ((m_status == AIPU_JOB_STATUS_DONE) || (m_status == AIPU_JOB_STATUS_EXCEPTION))
    {
        *status = (aipu_job_status_t)m_status;
        dump_job_private_buffers_after_run(m_rodata, m_descriptor);
    }
    else
    {
        *status = AIPU_JOB_STATUS_NO_STATUS;
    }

    return ret;
}

aipu_status_t aipudrv::JobBase::get_status_blocking(aipu_job_status_t* status, int32_t time_out)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    std::vector<aipu_job_status_desc> jobs_status;

    ret = convert_ll_status(m_dev->poll_status(jobs_status, 1, time_out, true));
    if (ret != AIPU_STATUS_SUCCESS)
    {
        return ret;
    }

    if (jobs_status.size() != 0)
    {
        m_status = jobs_status[0].state;
    }

    if ((m_status == AIPU_JOB_STATUS_DONE) || (m_status == AIPU_JOB_STATUS_EXCEPTION))
    {
        *status = (aipu_job_status_t)m_status;
        dump_job_private_buffers_after_run(m_rodata, m_descriptor);
    }
    else
    {
        *status = AIPU_JOB_STATUS_NO_STATUS;
    }

    return ret;
}

aipu_status_t aipudrv::JobBase::load_tensor(uint32_t tensor, const void* data)
{
    if (nullptr == data)
    {
        return AIPU_STATUS_ERROR_NULL_PTR;
    }

    if (tensor >= m_inputs.size())
    {
        return AIPU_STATUS_ERROR_INVALID_TENSOR_ID;
    }

    /* Applications cannot load tensors if a job is not in the to-be-scheduled status */
    if ((m_status != AIPU_JOB_STATUS_INIT) &&
        (m_status != AIPU_JOB_STATUS_DONE) &&
        (m_status != AIPU_JOB_STATUS_BIND))
    {
        return AIPU_STATUS_ERROR_INVALID_OP;
    }

    assert(m_mem->write(m_inputs[tensor].pa, (const char*)data, m_inputs[tensor].size)
        == (int)m_inputs[tensor].size);
    return AIPU_STATUS_SUCCESS;
}

aipu_status_t aipudrv::JobBase::get_tensor(aipu_tensor_type_t type, uint32_t tensor, void* data)
{
    DEV_PA_64 pa = 0;
    uint64_t size = 0;

    if (nullptr == data)
    {
        return AIPU_STATUS_ERROR_NULL_PTR;
    }

    /* Applications cannot get tensors if a job is not done status */
    if (m_status != AIPU_JOB_STATUS_DONE)
    {
        return AIPU_STATUS_ERROR_INVALID_OP;
    }

    if (AIPU_TENSOR_TYPE_INPUT == type)
    {
        if (tensor >= m_inputs.size())
        {
            return AIPU_STATUS_ERROR_INVALID_TENSOR_ID;
        }
        pa   = m_inputs[tensor].pa;
        size = m_inputs[tensor].size;
    }
    else if (AIPU_TENSOR_TYPE_OUTPUT == type)
    {
        if (tensor >= m_outputs.size())
        {
            return AIPU_STATUS_ERROR_INVALID_TENSOR_ID;
        }
        pa   = m_outputs[tensor].pa;
        size = m_outputs[tensor].size;
    }
    else if (AIPU_TENSOR_TYPE_PRINTF == type)
    {
        if (tensor >= m_printf.size())
        {
            return AIPU_STATUS_ERROR_INVALID_TENSOR_ID;
        }
        pa   = m_printf[tensor].pa;
        size = m_printf[tensor].size;
    }
    else if (AIPU_TENSOR_TYPE_PROFILER == type)
    {
        if (tensor >= m_profiler.size())
        {
            return AIPU_STATUS_ERROR_INVALID_TENSOR_ID;
        }
        pa   = m_profiler[tensor].pa;
        size = m_profiler[tensor].size;
    }

    assert(m_mem->read(pa, (char*)data, size) == (int)size);
    return AIPU_STATUS_SUCCESS;
}

aipu_status_t aipudrv::JobBase::setup_rodata(
    const std::vector<struct GraphParamMapLoadDesc>& param_map,
    const std::vector<BufferDesc>& reuse_buf,
    const std::vector<BufferDesc>& static_buf,
    BufferDesc rodata,
    BufferDesc dcr
)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    char* ro_va = nullptr;
    char* dcr_va = nullptr;

    m_mem->pa_to_va(rodata.pa, rodata.size, &ro_va);
    if (dcr.size != 0)
    {
        m_mem->pa_to_va(dcr.pa, dcr.size, &dcr_va);
    }

    for (uint32_t i = 0; i < param_map.size(); i++)
    {
        char* entry = nullptr;
        uint32_t init_val = 0;
        uint32_t finl_val = 0;
        uint32_t ref_iter = param_map[i].ref_section_iter;
        uint32_t sec_offset = param_map[i].sub_section_offset;
        uint32_t sub_sec_pa_32 = 0;

        if (param_map[i].offset_in_map < rodata.req_size)
        {
            entry = ro_va + param_map[i].offset_in_map;
        }
        else
        {
            if (dcr.size == 0)
            {
                ret = AIPU_STATUS_ERROR_INVALID_GBIN;
                goto finish;
            }
            entry = dcr_va + param_map[i].offset_in_map - rodata.req_size;
        }

        if (param_map[i].load_type == PARAM_MAP_LOAD_TYPE_REUSE)
        {
            if (ref_iter >= reuse_buf.size())
            {
                ret = AIPU_STATUS_ERROR_INVALID_SIZE;
                goto finish;
            }
            sub_sec_pa_32 = get_low_32(reuse_buf[ref_iter].pa) + sec_offset;
        }
        else if (param_map[i].load_type == PARAM_MAP_LOAD_TYPE_STATIC)
        {
            if (ref_iter >= static_buf.size())
            {
                ret = AIPU_STATUS_ERROR_INVALID_SIZE;
                goto finish;
            }
            sub_sec_pa_32 = get_low_32(static_buf[ref_iter].pa + sec_offset);
        }

        memcpy(&init_val, entry, 4);
        finl_val = ((sub_sec_pa_32 & param_map[i].addr_mask) | (init_val & (~param_map[i].addr_mask)));
        memcpy(entry, &finl_val, 4);

        LOG(LOG_CLOSE, "param %u: write addr/final_val 0x%x/0x%x (%s section %u offset 0x%x) into %s",
            i,
            sub_sec_pa_32,
            finl_val,
            (param_map[i].load_type == PARAM_MAP_LOAD_TYPE_REUSE) ? "reuse" : "weight",
            ref_iter,
            sec_offset,
            (param_map[i].offset_in_map < rodata.req_size) ? "rodata" : "descriptor");
    }

finish:
    return ret;
}

aipudrv::DEV_PA_64 aipudrv::JobBase::get_base_pa(int sec_type, BufferDesc& rodata,
        BufferDesc& descriptor)
{
    DEV_PA_64 pa = 0;

    if (sec_type == SECTION_TYPE_RODATA)
    {
        pa = rodata.pa;
    }
    else if (sec_type == SECTION_TYPE_DESCRIPTOR)
    {
        pa = descriptor.pa;
    }
    else if (sec_type == SECTION_TYPE_TEXT)
    {
        pa = get_graph().m_text.pa;
    }

    return pa;
}

void aipudrv::JobBase::setup_remap(BufferDesc& rodata, BufferDesc& descriptor)
{
    for (uint32_t i = 0; i < get_graph().m_remap.size(); i++)
    {
        DEV_PA_64 dest = get_base_pa(get_graph().m_remap[i].type, rodata, descriptor) +
            get_graph().m_remap[i].next_addr_entry_offset;
        DEV_PA_32 pa = get_base_pa(get_graph().m_remap[i].next_type, rodata, descriptor) +
            get_graph().m_remap[i].next_offset;
        assert(m_mem->write32(dest, pa) == 4);
    }
}

void aipudrv::JobBase::create_io_buffers(std::vector<struct JobIOBuffer>& bufs,
        const std::vector<GraphIOTensorDesc>& desc,
        const std::vector<BufferDesc>& reuses)
{
    uint32_t cnt = desc.size();

    for (uint32_t i = 0; i < cnt; i++)
    {
        uint32_t sec_iter = desc[i].ref_section_iter;
        DEV_PA_64 pa = reuses[sec_iter].pa + desc[i].offset_in_section;
        JobIOBuffer iobuf;

        iobuf.init(desc[i].id, desc[i].size, pa);
        bufs.push_back(iobuf);
    }
}

void aipudrv::JobBase::create_io_buffers(const struct GraphIOTensors& io,
    const std::vector<BufferDesc>& reuses)
{
    create_io_buffers(m_inputs, io.inputs, reuses);
    create_io_buffers(m_outputs, io.outputs, reuses);
    create_io_buffers(m_inter_dumps, io.inter_dumps, reuses);
    create_io_buffers(m_profiler, io.profiler, reuses);
    create_io_buffers(m_printf, io.printf, reuses);
    create_io_buffers(m_layer_counter, io.layer_counter, reuses);
}

void aipudrv::JobBase::dump_buffer(DEV_PA_64 pa, const char* bin_va, uint32_t size, const char* name)
{
    char file_name[4096];

    if (bin_va != nullptr)
    {
        snprintf(file_name, 4096, "%s/Graph_0x%lx_Job_0x%lx_%s_Dump_in_Binary_Size_0x%x.bin",
        m_dump_dir.c_str(), get_graph().m_id, m_id, name, size);
        umd_dump_file_helper(file_name, bin_va, size);
    }

    snprintf(file_name, 4096, "%s/Graph_0x%lx_Job_0x%lx_%s_Dump_in_DRAM_PA_0x%lx_Size_0x%x.bin",
        m_dump_dir.c_str(), get_graph().m_id, m_id, name, pa, size);
    m_mem->dump_file(pa, file_name, size);
}

aipu_status_t aipudrv::JobBase::config_mem_dump(uint64_t types, const aipu_job_config_dump_t* config)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;

    if ((nullptr != config) && (nullptr != config->dump_dir))
    {
        m_dump_dir = config->dump_dir;
    }

    if ((nullptr != config) && (nullptr != config->prefix))
    {
        m_dump_prefix = config->prefix;
    }

    m_dump_text = types & AIPU_JOB_CONFIG_TYPE_DUMP_TEXT;
    m_dump_weight = types & AIPU_JOB_CONFIG_TYPE_DUMP_WEIGHT;
    m_dump_rodata = types & AIPU_JOB_CONFIG_TYPE_DUMP_RODATA;
    m_dump_dcr = types & AIPU_JOB_CONFIG_TYPE_DUMP_DESCRIPTOR;
    m_dump_input = types & AIPU_JOB_CONFIG_TYPE_DUMP_INPUT;
    m_dump_output = types & AIPU_JOB_CONFIG_TYPE_DUMP_OUTPUT;
    m_dump_tcb = types & AIPU_JOB_CONFIG_TYPE_DUMP_TCB_CHAIN;
    m_dump_emu = types & AIPU_JOB_CONFIG_TYPE_DUMP_EMULATION;

    return ret;
}

void aipudrv::JobBase::dump_job_shared_buffers()
{
    DEV_PA_64 dump_pa;
    uint32_t dump_size;
    const char* bin_va = nullptr;

    if (m_dump_text)
    {
        dump_pa = get_graph().m_text.pa;
        bin_va = get_graph().m_btext.va;
        dump_size = get_graph().m_btext.size;
        dump_buffer(dump_pa, bin_va, dump_size, "Text");
    }

    if (m_dump_weight)
    {
        dump_pa = get_graph().m_weight.pa;
        bin_va = get_graph().m_bweight.va;
        dump_size = get_graph().m_bweight.size;
        if (dump_size != 0)
        {
            dump_buffer(dump_pa, bin_va, dump_size, "Weight");
        }
    }
}

void aipudrv::JobBase::dump_job_private_buffers(BufferDesc& rodata, BufferDesc& descriptor)
{
    DEV_PA_64 dump_pa;
    uint32_t dump_size;
    const char* bin_va = nullptr;

    if (m_dump_rodata)
    {
        dump_pa = rodata.pa;
        bin_va = get_graph().m_brodata.va;
        dump_size = get_graph().m_brodata.size;
        if (dump_size != 0)
        {
            dump_buffer(dump_pa, bin_va, dump_size, "Rodata");
        }
    }

    if (m_dump_dcr)
    {
        dump_pa = descriptor.pa;
        bin_va = get_graph().m_bdesc.va;
        dump_size = get_graph().m_bdesc.size;
        if (dump_size != 0)
        {
            dump_buffer(dump_pa, bin_va, dump_size, "Descriptor");
        }
    }

    if (m_dump_input)
    {
        for (uint32_t i = 0; i < m_inputs.size(); i++)
        {
            char name[32];
            dump_pa   = m_inputs[i].pa;
            dump_size = m_inputs[i].size;
            snprintf(name, 32, "Input%u", m_inputs[i].id);
            if (dump_size != 0)
            {
                dump_buffer(dump_pa, nullptr, dump_size, name);
            }
        }
    }
}

void aipudrv::JobBase::dump_job_private_buffers_after_run(BufferDesc& rodata, BufferDesc& descriptor)
{
    DEV_PA_64 dump_pa;
    uint32_t dump_size;

    if (m_dump_output)
    {
        for (uint32_t i = 0; i < m_outputs.size(); i++)
        {
            char name[32];
            dump_pa   = m_outputs[i].pa;
            dump_size = m_outputs[i].size;
            snprintf(name, 32, "Output%u", m_outputs[i].id);
            if (dump_size != 0)
            {
                dump_buffer(dump_pa, nullptr, dump_size, name);
            }
        }
    }
}

aipu_status_t aipudrv::JobBase::validate_schedule_status()
{
    if ((m_status == AIPU_JOB_STATUS_INIT) ||
        (m_status == AIPU_JOB_STATUS_DONE))
    {
        return AIPU_STATUS_SUCCESS;
    }
    return AIPU_STATUS_ERROR_INVALID_OP;
}