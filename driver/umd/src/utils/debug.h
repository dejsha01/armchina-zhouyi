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
 * @file  debug.h
 * @brief UMD debug usage macro header
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#define __PRINT_MACRO(x) #x
#define PRINT_MACRO(x) #x " is defined to be " __PRINT_MACRO(x)

#if ((defined RTDEBUG) && (RTDEBUG==1))

#define LOG_ENABLE 1
#define RTDEBUG_SIMULATOR_LOG_LEVEL     3
#define RTDEBUG_TRACKING_MEM_OPERATION  1

#else /* ! RTDEBUG */

#define LOG_ENABLE 0
#define RTDEBUG_SIMULATOR_LOG_LEVEL     0
#define RTDEBUG_TRACKING_MEM_OPERATION  0

#endif /* #ifdef RTDEBUG */

#endif /* _DEBUG_H_ */
