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
 * @file  swig_cpp2py_api.cpp
 * @brief AIPU User Mode Driver (UMD) C++ to Python API implementation
 * @version 1.0
 */

#include "swig_cpp2py_api.hpp"

Aipu& OpenDevice()
{
    Aipu& aipu = Aipu::get_aipu();
    aipu.init_dev();
    return aipu;
}

void CloseDevice()
{
    Aipu& aipu = Aipu::get_aipu();
    aipu.deinit_dev();
}