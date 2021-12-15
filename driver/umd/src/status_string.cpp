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

#include "context.h"

char aipudrv::MainContext::umd_status_string[][1024] = {
    "AIPU UMD API executes successfully.",
    "AIPU UMD API input argument(s) contain NULL pointer.",
    "AIPU UMD runtime context is invalid. Please init first.",
    "AIPU device open operation failed. Please check kernel module insmod status or hardware status.",
    "AIPU device state is abnormal. Please check kernel module insmod status or hardware status.",
    "AIPU UMD runtime context de-initialization failed.",
    "AIPU UMD runtime configuration input(s) is/are invalid.",
    "The type of the binary loaded is unknown. It cannot be loaded or executed.",
    "The version of the loadable graph binary provided is not supported on UMD.",
    "Loadable graph binary is invalid which contains unrecognized or invalid items.",
    "The specified target device is not found at runtime. Please double check the binary target.",
    "Graph ID provided is an invalid one which has been unloaded or never existed.",
    "UMD fails in opening the specified file.",
    "UMD fails in maping the specified file.",
    "UMD fails in reading the specified file.",
    "UMD fails in writing the specified file.",
    "Job ID provided is an invalid one which has been cleaned or never existed.",
    "The flushed job encounters exception during execution.",
    "The flushed job execution timeout.",
    "This API call here is not supported on this platform.",
    "This API call here is invalid.",
    "Job size provided is an invalid one.",
    "UMD fails in allocating buffers.",
    "UMD fails in releasing buffers.",
    "The AIPU core ID application provides is invalid and cannot be found in system.",
    "UMD fails in reserving SRAM as the executable binary requested because there is no SoC SRAM or SRAM is busy.",
    "The tensor ID application provides is invalid.",
    "The AIPU cluster ID application provides is invalid and cannot be found in system.",
    "UMD fails in parsing the printf buffer and print corresponding logs.",
    "Status Max value which should not be returned to application.",
};