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
 * @file  helper.h
 * @brief UMD helper function header
 */

#ifndef _HELPER_H_
#define _HELPER_H_

#include <iostream>
#include <fstream>

/**
 * @brief Align buffer bytes per page_size (4KB)
 */
#define ALIGN_PAGE(bytes) (((bytes)+(4096)-1)&(~((4096)-1)))
#define ALIGN_ADDR(bytes) ((ALIGN_PAGE(bytes))/(4096))

/**
 * @brief This function is used to dump memory data into file
 *
 * @param[in] fname File full name
 * @param[in] src   Source location of data
 * @param[in] size  Dumped size
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_SIZE
 * @retval AIPU_STATUS_ERROR_OPEN_FILE_FAIL
 * @retval AIPU_STATUS_ERROR_WRITE_FILE_FAIL
 */
aipu_status_t umd_dump_file_helper(const char* fname, const void* src, unsigned int size);
/**
 * @brief This function is used to load file into memory
 *
 * @param[in]  fname File full name
 * @param[out] dest  Destination of data
 * @param[in]  size  Load size
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_SIZE
 * @retval AIPU_STATUS_ERROR_OPEN_FILE_FAIL
 * @retval AIPU_STATUS_ERROR_READ_FILE_FAIL
 */
aipu_status_t umd_load_file_helper(const char* fname, void* dest, unsigned int size);
/**
 * @brief This function is used to open and mmap a file into memory
 *
 * @param[in]  fname File full name
 * @param[out] data  Pointer to file mmap buffer
 * @param[out] size  File size
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_OPEN_FILE_FAIL
 * @retval AIPU_STATUS_ERROR_MAP_FILE_FAIL
 */
aipu_status_t umd_mmap_file_helper(const char* fname, void** data, unsigned int* size);
/**
 * @brief This function is used to draw a line composed of a character into an opened file
 *
 * @param[in] file File stream
 * @param[in] ch   Character of the line to be drawn
 * @param[in] num  Character number, i.e. length of the line
 *
 */
void umd_draw_line_helper(std::ofstream& file, char ch, uint32_t num);
/**
 * @brief This function is used to check if a given pointer is within a valid region;
 *        When size == 0, only check the ptr itself
 *
 * @param[in] lower_bound Lower bound of the region
 * @param[in] upper_bound Upper bound of the region
 * @param[in] ptr         A given pointer
 * @param[in] size        Size to operate after pointer ptr
 *
 */
bool umd_is_valid_ptr(const void* lower_bound, const void* upper_bound,
        const void* ptr, uint32_t size = 0);

#endif /* _HELPER_H_ */
