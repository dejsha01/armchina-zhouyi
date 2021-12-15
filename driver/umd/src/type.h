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
 * @file  type.h
 */

#ifndef _TYPE_H_
#define _TYPE_H_

namespace aipudrv
{
typedef uint32_t DEV_PA_32;
typedef uint64_t DEV_PA_64;

inline DEV_PA_32 get_high_32(DEV_PA_64 pa)
{
    return pa >> 32;
}

inline DEV_PA_32 get_low_32(DEV_PA_64 pa)
{
    return pa & 0xFFFFFFFF;
}

typedef uint64_t GRAPH_ID;
typedef uint64_t JOB_ID;

inline GRAPH_ID job_id2graph_id(JOB_ID id)
{
    return id >> 32;
}

inline JOB_ID create_full_job_id(GRAPH_ID g, JOB_ID id)
{
    return (g << 32) + id;
}

inline GRAPH_ID get_graph_id(uint64_t id)
{
    return (id & 0xFFFFFFFF) ? (id >> 32) : id;
}

}

#endif /* _TYPE_H_ */