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
 * @file  device.h
 * @brief AIPU User Mode Driver (UMD) device module header
 */

#include <assert.h>
#include "standard_api.h"
#include "device_base.h"
#include "parser_base.h"
#ifdef SIMULATION
#include "simulator/simulator.h"
#include "simulator/z5_simulator.h"
#else
#include "aipu/aipu.h"
#endif

#ifndef _DEVICE_H_
#define _DEVICE_H_

namespace aipudrv
{
inline aipu_status_t test_get_device(uint32_t graph_version, DeviceBase** dev,
    const aipu_global_config_simulation_t* cfg)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;

    assert(dev != nullptr);

    if ((AIPU_LOADABLE_GRAPH_V0005 != graph_version) &&
        (AIPU_LOADABLE_GRAPH_ELF_V0 != graph_version))
    {
        return AIPU_STATUS_ERROR_GVERSION_UNSUPPORTED;
    }

#if (defined SIMULATION)
#if (defined ZHOUYI_V123)
    if (AIPU_LOADABLE_GRAPH_V0005 == graph_version)
    {
        if ((*dev != nullptr) && ((*dev)->get_dev_type() != DEV_TYPE_SIMULATOR_LEGACY))
        {
            return AIPU_STATUS_ERROR_TARGET_NOT_FOUND;
        }
        else if (nullptr == *dev)
        {
            *dev = Simulator::get_simulator();
        }
    }
#endif
#if (defined ZHOUYI_V5)
    if (AIPU_LOADABLE_GRAPH_ELF_V0 == graph_version)
    {
        if ((*dev != nullptr) && ((*dev)->get_dev_type() != DEV_TYPE_SIMULATOR_Z5))
        {
            return AIPU_STATUS_ERROR_TARGET_NOT_FOUND;
        }
        else if (nullptr == *dev)
        {
            *dev = Z5Simulator::get_z5_simulator(cfg);
        }
    }
#endif
#else /* !SIMULATION */
    ret = Aipu::get_aipu(dev);
#endif

    if (nullptr == *dev)
    {
        return AIPU_STATUS_ERROR_TARGET_NOT_FOUND;
    }
    return ret;
}

inline aipu_status_t get_device(DeviceBase** dev)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;

#ifndef SIMULATION
    assert(dev != nullptr);
    ret = Aipu::get_aipu(dev);
#endif

    return ret;
}

inline void put_device(DeviceBase* dev)
{
    if (nullptr == dev)
    {
        return;
    }

    if (dev->dec_ref_cnt() == 0)
    {
        delete dev;
    }
}
}

#endif /* _DEVICE_H_ */
