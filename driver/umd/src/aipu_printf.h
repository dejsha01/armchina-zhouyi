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
 * @file  aipu_printf.h
 * @brief AIPU Debug Log print header
 */

#ifndef _AIPU_PRINTF_H_
#define _AIPU_PRINTF_H_

/**
 * log buffer header size, fixed 8 bytes
 */
#define LOG_HEADER_SZ 8

/**
 * log buffer size, fix 1MB
 */
#define BUFFER_LEN (1024 * 1024)

/**
 * buffer header format
 */
typedef struct {
    int overwrite_flag;
    int write_offset;
} aipu_log_buffer_header_t;

#endif /* _AIPU_PRINTF_H_ */
