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
 * @file  standard_api.h
 * @brief Zhouyi Z5 AIPU User Mode Driver (UMD) Standard API header
 * @version 1.0
 */

#ifndef _Z5_STANDARD_API_H_
#define _Z5_STANDARD_API_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct ctx_handle aipu_ctx_handle_t;

typedef enum {
    AIPU_DATA_TYPE_NONE = 0,
    AIPU_DATA_TYPE_BOOL = 1,
    AIPU_DATA_TYPE_U8   = 2,
    AIPU_DATA_TYPE_S8   = 3,
    AIPU_DATA_TYPE_U16  = 4,
    AIPU_DATA_TYPE_S16  = 5,
} aipu_data_type_t;

typedef enum {
    AIPU_TENSOR_TYPE_INPUT    = 0,
    AIPU_TENSOR_TYPE_OUTPUT   = 1,
    AIPU_TENSOR_TYPE_PRINTF   = 2,
    AIPU_TENSOR_TYPE_PROFILER = 3,
} aipu_tensor_type_t;

typedef struct
{
    uint32_t id;
    uint32_t size;
    float    scale;
    float    zero_point;
    aipu_data_type_t data_type;
} aipu_tensor_desc_t;

/**
 * @brief AIPU job status; returned by status querying API aipu_get_job_status().
 */
typedef enum {
    AIPU_JOB_STATUS_NO_STATUS, /**< no status */
    AIPU_JOB_STATUS_DONE,      /**< job execution successfully */
    AIPU_JOB_STATUS_EXCEPTION  /**< job execution failed, encountering exception */
} aipu_job_status_t;

typedef struct {
    uint64_t instr_base;
    void* simulation_aipu;
    void* simulation_mem_engine;
} aipu_debugger_job_info_t;

typedef enum {
    /**
     * AIPU_GLOBAL_CONFIG_TYPE_[*]: for aipu_config_global() only;
     * AIPU_JOB_CONFIG_TYPE_[*]: for aipu_config_job() only;
     * AIPU_CONFIG_TYPE_[*]: for aipu_config_global/aipu_config_job();
     */
    AIPU_JOB_CONFIG_TYPE_DUMP_TEXT            = 0x1,
    AIPU_JOB_CONFIG_TYPE_DUMP_WEIGHT          = 0x2,
    AIPU_JOB_CONFIG_TYPE_DUMP_RODATA          = 0x4,
    AIPU_JOB_CONFIG_TYPE_DUMP_DESCRIPTOR      = 0x8,
    AIPU_JOB_CONFIG_TYPE_DUMP_INPUT           = 0x10,
    AIPU_JOB_CONFIG_TYPE_DUMP_OUTPUT          = 0x20,
    AIPU_JOB_CONFIG_TYPE_DUMP_TCB_CHAIN       = 0x40,
    AIPU_JOB_CONFIG_TYPE_DUMP_EMULATION       = 0x80,
    AIPU_CONFIG_TYPE_SIMULATION               = 0x100,
    AIPU_GLOBAL_CONFIG_TYPE_DISABLE_VER_CHECK = 0x200,
    AIPU_GLOBAL_CONFIG_TYPE_ENABLE_VER_CHECK  = 0x400,
} aipu_config_type_t;

typedef struct {
    /**
     * dump_dir is used as file dump-path
     */
    const char* dump_dir;
    /**
     * name prefix of dump files
     */
    const char* prefix;
} aipu_job_config_dump_t;

typedef struct {
    /**
     * data_dir is used as z1/2/3 simulation data file directory
     */
    const char* data_dir;
} aipu_job_config_simulation_t;

typedef struct {
    /* configure one or more simulator file name for z1/2/3 */
    /* set z[n]_simulator to be NULL for z5 */
    /* log_level works for all simulator versions */
    const char* z1_simulator;
    const char* z2_simulator;
    const char* z3_simulator;
    const char* log_file_path;
    uint32_t log_level;
    bool verbose;
    bool enable_avx;
    bool enable_calloc;
} aipu_global_config_simulation_t;

typedef struct aipu_io_tensors {
    uint32_t count;
    struct aipu_io_tensor {
        aipu_tensor_desc_t desc;
        void* data;
    }* tensors;
} aipu_io_tensors_t;

typedef int (*aipu_job_handler_callback)(void* priv, uint64_t job, bool exception, aipu_io_tensors_t* outputs);

/**
 * @brief This aipu_status_t enumeration captures the result of any API function
 *        that has been executed. Success is represented by AIPU_STATUS_SUCCESS
 *        which has a value of zero. Error statuses are assigned positive integers
 *        and their identifiers start with the AIPU_STATUS_ERROR prefix.
 */
typedef enum {
    AIPU_STATUS_SUCCESS                    = 0x0,
    AIPU_STATUS_ERROR_NULL_PTR             = 0x1,
    AIPU_STATUS_ERROR_INVALID_CTX          = 0x2,
    AIPU_STATUS_ERROR_OPEN_DEV_FAIL        = 0x3,
    AIPU_STATUS_ERROR_DEV_ABNORMAL         = 0x4,
    AIPU_STATUS_ERROR_DEINIT_FAIL          = 0x5,
    AIPU_STATUS_ERROR_INVALID_CONFIG       = 0x6,
    AIPU_STATUS_ERROR_UNKNOWN_BIN          = 0x7,
    AIPU_STATUS_ERROR_GVERSION_UNSUPPORTED = 0x8,
    AIPU_STATUS_ERROR_INVALID_GBIN         = 0x9,
    AIPU_STATUS_ERROR_TARGET_NOT_FOUND     = 0xA,
    AIPU_STATUS_ERROR_INVALID_GRAPH_ID     = 0xB,
    AIPU_STATUS_ERROR_OPEN_FILE_FAIL       = 0xC,
    AIPU_STATUS_ERROR_MAP_FILE_FAIL        = 0xD,
    AIPU_STATUS_ERROR_READ_FILE_FAIL       = 0xE,
    AIPU_STATUS_ERROR_WRITE_FILE_FAIL      = 0xF,
    AIPU_STATUS_ERROR_INVALID_JOB_ID       = 0x10,
    AIPU_STATUS_ERROR_JOB_EXCEPTION        = 0x11,
    AIPU_STATUS_ERROR_JOB_TIMEOUT          = 0x12,
    AIPU_STATUS_ERROR_OP_NOT_SUPPORTED     = 0x13,
    AIPU_STATUS_ERROR_INVALID_OP           = 0x14,
    AIPU_STATUS_ERROR_INVALID_SIZE         = 0x15,
    AIPU_STATUS_ERROR_BUF_ALLOC_FAIL       = 0x16,
    AIPU_STATUS_ERROR_BUF_FREE_FAIL        = 0x17,
    AIPU_STATUS_ERROR_INVALID_CORE_ID      = 0x18,
    AIPU_STATUS_ERROR_RESERVE_SRAM_FAIL    = 0x19,
    AIPU_STATUS_ERROR_INVALID_TENSOR_ID    = 0x1A,
    AIPU_STATUS_ERROR_INVALID_CLUSTER_ID   = 0x1B,
    AIPU_STATUS_ERROR_PRINTF_FAIL          = 0x1C,
    AIPU_STATUS_MAX = 0x1D
} aipu_status_t;

/**
 * @brief This API is used to initialize AIPU UMD context
 *
 * @param[out] ctx Pointer to a memory location allocated by application where UMD stores the
 *                     opaque context handle struct
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_OPEN_DEV_FAIL
 * @retval AIPU_STATUS_ERROR_DEV_ABNORMAL
 *
 * @note Before invoking any other UMD API calls, any UMD application must initialize a context first.
 */
aipu_status_t aipu_init_context(aipu_ctx_handle_t** ctx);
/**
 * @brief This API is used to destroy AIPU UMD context
 *
 * @param[in] ctx Pointer to a context handle struct returned by aipu_init_context
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_DEINIT_FAIL
 */
aipu_status_t aipu_deinit_context(const aipu_ctx_handle_t* ctx);
/**
 * @brief This API is used to track what happens when an error code is returned by a UMD API.
 *
 * @param[in]  ctx    Pointer to a context handle struct returned by aipu_init_context
 * @param[in]  status Status returned by UMD standard API
 * @param[out] msg    Pointer to a memory location allocated by application where UMD stores the
 *                        message string pointer
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 */
aipu_status_t aipu_get_error_message(const aipu_ctx_handle_t* ctx, aipu_status_t status, const char** msg);
/**
 * @brief This API is used to configure a specified global option.
 *
 * @param[in]  ctx    Pointer to a context handle struct returned by aipu_init_context
 * @param[in]  types  Configuration type(s)
 * @param[out] config Pointer to a memory location allocated by application where application stores the
 *                        configuration data struct
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_CONFIG
 *
 * @note accepted types/config: AIPU_CONFIG_TYPE_SIMULATION/aipu_global_config_simulation_t
 * @note accepted types/config: AIPU_GLOBAL_CONFIG_TYPE_DISABLE_VER_CHECK/none
 * @note accepted types/config: AIPU_GLOBAL_CONFIG_TYPE_ENABLE_VER_CHECK/none
 */
aipu_status_t aipu_config_global(const aipu_ctx_handle_t* ctx, uint64_t types, void* config);
/**
 * @brief This API loads an offline built AIPU executable graph binary from file system.
 *
 * @param[in]  ctx   Pointer to a context handle struct returned by aipu_init_context
 * @param[in]  garph Executable graph binary file path
 * @param[out] id    Pointer to a memory location allocated by application where UMD stores the
 *                       graph ID
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_OPEN_FILE_FAIL
 * @retval AIPU_STATUS_ERROR_MAP_FILE_FAIL
 * @retval AIPU_STATUS_ERROR_UNKNOWN_BIN
 * @retval AIPU_STATUS_ERROR_GVERSION_UNSUPPORTED
 * @retval AIPU_STATUS_ERROR_INVALID_GBIN
 * @retval AIPU_STATUS_ERROR_TARGET_NOT_FOUND
 * @retval AIPU_STATUS_ERROR_BUF_ALLOC_FAIL
 * @retval AIPU_STATUS_ERROR_RESERVE_SRAM_FAIL
 */
aipu_status_t aipu_load_graph(const aipu_ctx_handle_t* ctx, const char* graph, uint64_t* id);
/**
 * @brief This API is used to unload a loaded graph
 *
 * @param[in] ctx   Pointer to a context handle struct returned by aipu_init_context
 * @param[in] graph Graph ID returned by aipu_load_graph
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_GRAPH_ID
 */
aipu_status_t aipu_unload_graph(const aipu_ctx_handle_t* ctx, uint64_t graph);
/**
 * @brief This API is used to create a new job for a graph with provided buffer handle.
 *
 * @param[in]  ctx    Pointer to a context handle struct returned by aipu_init_context
 * @param[in]  graph  Graph ID returned by aipu_load_graph
 * @param[out] job    Pointer to a memory location allocated by application where UMD stores
 *                        the new created job ID
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_GRAPH_ID
 * @retval AIPU_STATUS_ERROR_BUF_ALLOC_FAIL
 *
 * @note The application can create one or multiple jobs by calling this API one or multiple times.
 * @note The application can schedule one created job one or multiple times by calling
 *           aipu_finish_job/aipu_flush_job, and at last clean it by calling aipu_clean_job.
 */
aipu_status_t aipu_create_job(const aipu_ctx_handle_t* ctx, uint64_t graph, uint64_t* job);
/**
 * @brief This API is used to flush a new computation job onto AIPU (blocking)
 *
 * @param[in] ctx      Pointer to a context handle struct returned by aipu_init_context
 * @param[in] job      Job ID returned by aipu_create_job
 * @param[in] time_out Time out (in millisecond) specified by application for this job
 *                     (A timeout of value <= 0 means an infinite timeout.)
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_JOB_ID
 * @retval AIPU_STATUS_ERROR_INVALID_OP
 * @retval AIPU_STATUS_ERROR_JOB_EXCEPTION
 * @retval AIPU_STATUS_ERROR_JOB_TIMEOUT
 */
aipu_status_t aipu_finish_job(const aipu_ctx_handle_t* ctx, uint64_t job, int32_t time_out);
/**
 * @brief This API is used to flush a new computation job onto AIPU (non-blocking)
 *
 * @param[in] ctx      Pointer to a context handle struct returned by aipu_init_context
 * @param[in] job      Job ID returned by aipu_create_job
 * @param[in] callback Pointer to a callback function the UMD application implemented which will be called
 *                         when the flushed job is done
 * @param[in] priv     Pointer to the UMD application private data structure used as
 *                         argument of the callback when it is called
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_JOB_ID
 * @retval AIPU_STATUS_ERROR_INVALID_OP
 *
 * @note The callback pointer should be NULL if the application would not use this feature.
 * @note A flushed job cannot be flushed again before the previous scheduled one is done.
 */
aipu_status_t aipu_flush_job(const aipu_ctx_handle_t* ctx, uint64_t job, aipu_job_handler_callback callback,
    void* priv);
/**
 * @brief This API is used to get the execution status of a flushed job (non-blocking)
 *
 * @param[in]  ctx    Pointer to a context handle struct returned by aipu_init_context
 * @param[in]  job    Job ID returned by aipu_create_job
 * @param[out] status Pointer to a memory location allocated by the application where UMD stores the job status
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_JOB_ID
 *
 * @note This API should be used by the application after aipu_flush_job successfully returns.
 */
aipu_status_t aipu_get_job_status(const aipu_ctx_handle_t* ctx, uint64_t job, aipu_job_status_t* status);
/**
 * @brief This API is used to clean a finished job object scheduled by aipu_finish_job/aipu_flush_job
 *
 * @param[in] ctx Pointer to a context handle struct returned by aipu_init_context
 * @param[in] job Job ID returned by aipu_create_job
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_JOB_ID
 */
aipu_status_t aipu_clean_job(const aipu_ctx_handle_t* ctx, uint64_t job);
/**
 * @brief This API is used to get tensor count of specified type
 *
 * @param[in]  ctx  Pointer to a context handle struct returned by aipu_init_context
 * @param[in]  id   Job ID returned by aipu_create_job, or graph ID returned by aipu_load_graph
 * @param[in]  type Tensor type
 * @param[out] cnt  Pointer to a memory location allocated by application where UMD stores the count
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_GRAPH_ID
 */
aipu_status_t aipu_get_tensor_count(const aipu_ctx_handle_t* ctx, uint64_t id, aipu_tensor_type_t type, uint32_t* cnt);
/**
 * @brief This API is used to get tensor count of specified type
 *
 * @param[in]  ctx    Pointer to a context handle struct returned by aipu_init_context
 * @param[in]  id     Job ID returned by aipu_create_job, or graph ID returned by aipu_load_graph
 * @param[in]  type   Tensor type
 * @param[in]  tensor Tensor ID
 * @param[out] desc   Pointer to a memory location allocated by application where UMD stores the tensor descriptor
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_GRAPH_ID
 * @retval AIPU_STATUS_ERROR_INVALID_TENSOR_ID
 */
aipu_status_t aipu_get_tensor_descriptor(const aipu_ctx_handle_t* ctx, uint64_t id, aipu_tensor_type_t type,
    uint32_t tensor, aipu_tensor_desc_t* desc);
/**
 * @brief This API is used to load input tensor data
 *
 * @param[in] ctx    Pointer to a context handle struct returned by aipu_init_context
 * @param[in] job    Job ID returned by aipu_create_job
 * @param[in] tensor Input tensor ID
 * @param[in] data   Data of input tensor
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_JOB_ID
 * @retval AIPU_STATUS_ERROR_INVALID_TENSOR_ID
 * @retval AIPU_STATUS_ERROR_INVALID_OP
 */
aipu_status_t aipu_load_tensor(const aipu_ctx_handle_t* ctx, uint64_t job, uint32_t tensor, const void* data);
/**
 * @brief This API is used to get tensor data of specified type
 *
 * @param[in]  ctx    Pointer to a context handle struct returned by aipu_init_context
 * @param[in]  job    Job ID returned by aipu_create_job
 * @param[in]  type   Tensor type
 * @param[in]  tensor Input tensor ID
 * @param[out] data   Data of output tensor
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_JOB_ID
 * @retval AIPU_STATUS_ERROR_INVALID_TENSOR_ID
 * @retval AIPU_STATUS_ERROR_INVALID_OP
 */
aipu_status_t aipu_get_tensor(const aipu_ctx_handle_t* ctx, uint64_t job, aipu_tensor_type_t type, uint32_t tensor, void* data);
/**
 * @brief This API is used to configure a specified option of a job.
 *
 * @param[in]  ctx    Pointer to a context handle struct returned by aipu_init_context
 * @param[in]  job    Job ID returned by aipu_create_job
 * @param[in]  types  Configuration type(s)
 * @param[out] config Pointer to a memory location allocated by application where application stores the
 *                        configuration data struct
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_JOB_ID
 * @retval AIPU_STATUS_ERROR_INVALID_CONFIG
 *
 * @note accepted types/config: AIPU_JOB_CONFIG_TYPE_DUMP_[*]/aipu_job_config_dump_t
 * @note accepted types/config: AIPU_CONFIG_TYPE_SIMULATION/aipu_job_config_simulation_t
 */
aipu_status_t aipu_config_job(const aipu_ctx_handle_t* ctx, uint64_t job, uint64_t types, void* config);
/**
 * @brief This API is used to get AIPU cluster count.
 *
 * @param[in]  ctx Pointer to a context handle struct returned by aipu_init_context
 * @param[out] cnt Pointer to a memory location allocated by application where UMD stores the cluster count
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_OP
 */
aipu_status_t aipu_get_cluster_count(const aipu_ctx_handle_t* ctx, uint32_t* cnt);
/**
 * @brief This API is used to get AIPU core count in a specific cluster.
 *
 * @param[in]  ctx     Pointer to a context handle struct returned by aipu_init_context
 * @param[in]  cluster Cluster ID
 * @param[out] cnt     Pointer to a memory location allocated by application where UMD stores the core count
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_OP
 * @retval AIPU_STATUS_ERROR_INVALID_CLUSTER_ID
 *
 * @note Cluster ID is numbered within [0, cluster_cnt).
 */
aipu_status_t aipu_get_core_count(const aipu_ctx_handle_t* ctx, uint32_t cluster, uint32_t* cnt);
/**
 * @brief This API is used by debugger to get information of a job
 *
 * @param[in]  ctx  Pointer to a context handle struct returned by aipu_init_context
 * @param[in]  job  Job ID
 * @param[out] info Pointer to a memory location allocated by application where UMD stores
 *                      a pointer to the job information
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_JOB_ID
 */
aipu_status_t aipu_debugger_get_job_info(const aipu_ctx_handle_t* ctx,
    uint64_t job, aipu_debugger_job_info_t* info);
/**
 * @brief This API bind a created job to an idle AIPU core for execution later;
 *        External registers of the specified AIPU core is writen after this API returns,
 *        but the start PC register is not triggerred to run.
 *
 * @param[in] ctx     Pointer to a context handle struct returned by aipu_init_context
 * @param[in] job_id  Job ID returned by aipu_create_job
 * @param[in] core_id ID of an AIPU core
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_CORE_ID
 * @retval AIPU_STATUS_ERROR_INVALID_JOB_ID
 * @retval AIPU_STATUS_ERROR_INVALID_OP

 * @note For the same core or job, it should only be bind once, unless the job is done.
 * @note The core to be bind should be in idle state; otherwise UMD returns error code
 */
aipu_status_t aipu_debugger_bind_job(const aipu_ctx_handle_t* ctx, uint32_t core_id, uint32_t job_id);
/**
 * @brief This API trigger a previously bind job to run on a target AIPU core;
 *        This API is a blocking API which returns after the job execution ends on hardware.
 *
 * @param[in] ctx     Pointer to a context handle struct returned by aipu_init_context
 * @param[in] job_id  Job ID returned by aipu_create_job and bind by aipu_debugger_bind_job
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_JOB_ID
 * @retval AIPU_STATUS_ERROR_JOB_EXCEPTION
 * @retval AIPU_STATUS_ERROR_INVALID_OP
 */
aipu_status_t aipu_debugger_run_job(const aipu_ctx_handle_t* ctx, uint32_t job_id);
/**
 * @brief This API allocates a buffer for AIPU debugger's usage
 *
 * @param[in] ctx     Pointer to a context handle struct returned by aipu_init_context
 * @param[in] size    Size of the requested buffer in byte
 * @param[out] va     Pointer to a virtual address stores the base address of the buffer
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_INVALID_SIZE
 * @retval AIPU_STATUS_ERROR_BUF_ALLOC_FAIL
 *
 * @note This API shall be used after aipu_load_graph and before calling aipu_debugger_bind_job.
 * @note When this API is used in multi-core scenario, only the core under debugging should
 *       be used to schedule a debugger job (in a single user thread), and other cores should
 *       keep in idle state.
 */
aipu_status_t aipu_debugger_malloc(const aipu_ctx_handle_t* ctx, uint32_t size, void** va);
/**
 * @brief This API frees a buffer allocated by aipu_debugger_malloc
 *
 * @param[in] ctx  Pointer to a context handle struct returned by aipu_init_context
 * @param[in] va   Virtual address to free
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_INVALID_CTX
 * @retval AIPU_STATUS_ERROR_BUF_FREE_FAIL
 */
aipu_status_t aipu_debugger_free(const aipu_ctx_handle_t* ctx, void* va);
/**
 * @brief this API print AIPU execution log information after corresponding job ends
 *
 * @param[in] printf_base   Pointer to a tensor buffer where stores the printf log data
 * @param[in] redirect_file Printf output redirect file path
 *
 * @retval AIPU_STATUS_SUCCESS
 * @retval AIPU_STATUS_ERROR_NULL_PTR
 * @retval AIPU_STATUS_ERROR_PRINTF_FAIL
 *
 * @note Application should get the printf log data via aipu_get_tensor before print it.
 */
aipu_status_t aipu_printf(char* printf_base, char* redirect_file);
#endif /* _Z5_STANDARD_API_H_ */
