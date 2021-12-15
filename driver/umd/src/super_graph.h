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
 * @file  super_base.h
 * @brief AIPU User Mode Driver (UMD) graph base module header
 */

#ifndef _SUPER_GRAPH_H_
#define _SUPER_GRAPH_H_

#include <fstream>
#include "standard_api.h"
#include "graph_base.h"

namespace aipudrv
{
class SuperGraph: public GraphBase
{
    /* To be implemented! */
public:
    virtual void print_parse_info(){};
    virtual aipu_status_t load(std::ifstream& gbin, uint32_t size, DeviceBase* dev){return AIPU_STATUS_SUCCESS;};
    virtual aipu_status_t unload(){return AIPU_STATUS_SUCCESS;};
    virtual aipu_status_t create_job(JOB_ID* id){return AIPU_STATUS_SUCCESS;};
    virtual aipu_status_t destroy_job(JOB_ID id){return AIPU_STATUS_SUCCESS;};
    virtual aipu_status_t get_tensor_count(aipu_tensor_type_t type, uint32_t* cnt){return AIPU_STATUS_SUCCESS;};
    virtual aipu_status_t get_tensor_descriptor(aipu_tensor_type_t type,
        uint32_t tensor, aipu_tensor_desc_t* desc){return AIPU_STATUS_SUCCESS;};
    virtual JobBase* get_job_object(JOB_ID id){return nullptr;};

public:
    SuperGraph(GRAPH_ID id, DeviceBase* dev): GraphBase(id, dev){};
    virtual ~SuperGraph(){};
    SuperGraph(const SuperGraph& base) = delete;
    SuperGraph& operator=(const SuperGraph& base) = delete;
};
}

#endif /* _SUPER_GRAPH_H_ */
