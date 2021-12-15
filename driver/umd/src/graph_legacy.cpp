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
 * @file  graph_legacy.cpp
 * @brief AIPU User Mode Driver (UMD) z1/2/3 legacy graph module implementation
 */

#include <cstring>
#include "graph_legacy.h"
#include "parser_legacy.h"
#include "job_legacy.h"
#include "utils/helper.h"
#include "utils/log.h"

aipudrv::GraphLegacy::GraphLegacy(GRAPH_ID id, DeviceBase* dev): Graph(id, dev)
{
    m_parser = new ParserLegacy();
}

aipudrv::GraphLegacy::~GraphLegacy()
{
    unload();
    delete m_parser;
}

aipu_status_t aipudrv::GraphLegacy::create_job(JOB_ID* id, const aipu_global_config_simulation_t* cfg)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    JobLegacy* job = new JobLegacy(*this, m_dev);

    ret = job->init(cfg);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    *id = add_job(job);
    return ret;
}

aipu_status_t aipudrv::GraphLegacy::get_tensor_count(aipu_tensor_type_t type, uint32_t* cnt)
{
    if (nullptr == cnt)
    {
        return AIPU_STATUS_ERROR_NULL_PTR;
    }

    if (type == AIPU_TENSOR_TYPE_INPUT)
    {
        *cnt = (uint32_t)m_io.inputs.size();
    }
    else if (type == AIPU_TENSOR_TYPE_OUTPUT)
    {
        *cnt = (uint32_t)m_io.outputs.size();
    }
    else if (type == AIPU_TENSOR_TYPE_PRINTF)
    {
        *cnt = (uint32_t)m_io.printf.size();
    }
    else if (type == AIPU_TENSOR_TYPE_PROFILER)
    {
        *cnt = (uint32_t)m_io.profiler.size();
    }

    return AIPU_STATUS_SUCCESS;
}

aipu_status_t aipudrv::GraphLegacy::get_tensor_descriptor(aipu_tensor_type_t type, uint32_t tensor, aipu_tensor_desc_t* desc)
{
    uint32_t cnt = 0;
    GraphIOTensorDesc io;

    get_tensor_count(type, &cnt);
    if (tensor >= cnt)
    {
        return AIPU_STATUS_ERROR_INVALID_TENSOR_ID;
    }

    if (nullptr == desc)
    {
        return AIPU_STATUS_ERROR_NULL_PTR;
    }

    if (type == AIPU_TENSOR_TYPE_INPUT)
    {
        io = m_io.inputs[tensor];
    }
    else if (type == AIPU_TENSOR_TYPE_OUTPUT)
    {
        io = m_io.outputs[tensor];
    }
    else if (type == AIPU_TENSOR_TYPE_PRINTF)
    {
        io = m_io.printf[tensor];
    }
    else if (type == AIPU_TENSOR_TYPE_PROFILER)
    {
        io = m_io.profiler[tensor];
    }
    else
    {
        return AIPU_STATUS_ERROR_INVALID_TENSOR_ID;
    }

    desc->id = tensor;
    desc->size = io.size;
    desc->scale = io.scale;
    desc->zero_point = io.zero_point;
    desc->data_type = io.data_type;

    return AIPU_STATUS_SUCCESS;
}
