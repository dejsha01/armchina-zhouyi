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
 * @file  standard_api_impl.cpp
 * @brief AIPU User Mode Driver (UMD) Standard API implementation file
 */

#include <stdlib.h>
#include <assert.h>
#include "standard_api.h"
#include "graph_base.h"
#include "job_base.h"
#include "ctx_ref_map.h"
#include "type.h"
#include "utils/helper.h"
#include "utils/log.h"

static aipu_status_t api_get_graph(const aipu_ctx_handle_t* ctx, uint64_t graph_id, aipudrv::GraphBase** graph)
{
    aipudrv::CtxRefMap& ctx_map = aipudrv::CtxRefMap::get_ctx_map();
    aipudrv::MainContext* p_ctx = nullptr;

    assert(graph != nullptr);

    if (nullptr == ctx)
    {
        return AIPU_STATUS_ERROR_NULL_PTR;
    }

    p_ctx = ctx_map.get_ctx_ref(ctx->handle);
    if (nullptr == p_ctx)
    {
        return AIPU_STATUS_ERROR_INVALID_CTX;
    }

    *graph = p_ctx->get_graph_object(graph_id);
    if (nullptr == graph)
    {
        return AIPU_STATUS_ERROR_INVALID_GRAPH_ID;
    }

    return AIPU_STATUS_SUCCESS;
}


static aipu_status_t api_get_job(const aipu_ctx_handle_t* ctx, uint64_t job_id, aipudrv::JobBase** job)
{
    aipudrv::CtxRefMap& ctx_map = aipudrv::CtxRefMap::get_ctx_map();
    aipudrv::MainContext* p_ctx = nullptr;

    assert(job != nullptr);

    if (nullptr == ctx)
    {
        return AIPU_STATUS_ERROR_NULL_PTR;
    }

    p_ctx = ctx_map.get_ctx_ref(ctx->handle);
    if (nullptr == p_ctx)
    {
        return AIPU_STATUS_ERROR_INVALID_CTX;
    }

    *job = p_ctx->get_job_object(job_id);
    if (nullptr == job)
    {
        return AIPU_STATUS_ERROR_INVALID_JOB_ID;
    }

    return AIPU_STATUS_SUCCESS;
}

aipu_status_t aipu_get_error_message(const aipu_ctx_handle_t* ctx, aipu_status_t status, const char** msg)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::CtxRefMap& ctx_map = aipudrv::CtxRefMap::get_ctx_map();
    aipudrv::MainContext* p_ctx = nullptr;

    if ((nullptr == ctx) || (nullptr == msg))
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    p_ctx = ctx_map.get_ctx_ref(ctx->handle);
    if (nullptr == p_ctx)
    {
        ret = AIPU_STATUS_ERROR_INVALID_CTX;
    }
    else
    {
        ret = p_ctx->get_status_msg(status, msg);
    }

finish:
    return ret;
}

aipu_status_t aipu_init_context(aipu_ctx_handle_t** ctx)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::CtxRefMap& ctx_map = aipudrv::CtxRefMap::get_ctx_map();
    aipudrv::MainContext* p_ctx = nullptr;
    aipu_ctx_handle_t* ctx_handle = nullptr;
    uint32_t handle = 0;

    if (nullptr == ctx)
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    handle = ctx_map.create_ctx_ref();
    p_ctx = ctx_map.get_ctx_ref(handle);
    assert(p_ctx != nullptr);

    ret = p_ctx->init();
    if (AIPU_STATUS_SUCCESS != ret)
    {
        ctx_map.destroy_ctx_ref(handle);
    }
    else
    {
        /* success */
        ctx_handle = new aipu_ctx_handle_t;
        ctx_handle->handle = handle;
        *ctx = ctx_handle;
    }

finish:
    return ret;
}

aipu_status_t aipu_deinit_context(const aipu_ctx_handle_t* ctx)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::CtxRefMap& ctx_map = aipudrv::CtxRefMap::get_ctx_map();
    aipudrv::MainContext* p_ctx = nullptr;

    if (nullptr == ctx)
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    p_ctx = ctx_map.get_ctx_ref(ctx->handle);
    if (nullptr == p_ctx)
    {
        ret = AIPU_STATUS_ERROR_INVALID_CTX;
    }
    else
    {
        ret = p_ctx->deinit();
        if (AIPU_STATUS_SUCCESS == ret)
        {
            ctx_map.destroy_ctx_ref(ctx->handle);
            delete ctx;
        }
    }

finish:
    return ret;
}

aipu_status_t aipu_load_graph(const aipu_ctx_handle_t* ctx, const char* graph_file, uint64_t* id)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::CtxRefMap& ctx_map = aipudrv::CtxRefMap::get_ctx_map();
    aipudrv::MainContext* p_ctx = nullptr;

    if (nullptr == ctx)
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    p_ctx = ctx_map.get_ctx_ref(ctx->handle);
    if (nullptr == p_ctx)
    {
        ret = AIPU_STATUS_ERROR_INVALID_CTX;
    }
    else
    {
        ret = p_ctx->load_graph(graph_file, id);
    }

finish:
    return ret;
}

aipu_status_t aipu_unload_graph(const aipu_ctx_handle_t* ctx, uint64_t graph)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::CtxRefMap& ctx_map = aipudrv::CtxRefMap::get_ctx_map();
    aipudrv::MainContext* p_ctx = nullptr;

    if (nullptr == ctx)
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    p_ctx = ctx_map.get_ctx_ref(ctx->handle);
    if (nullptr == p_ctx)
    {
        ret = AIPU_STATUS_ERROR_INVALID_CTX;
    }
    else
    {
        ret = p_ctx->unload_graph(graph);
    }

finish:
    return ret;
}

aipu_status_t aipu_create_job(const aipu_ctx_handle_t* ctx, uint64_t graph, uint64_t* job)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::CtxRefMap& ctx_map = aipudrv::CtxRefMap::get_ctx_map();
    aipudrv::MainContext* p_ctx = nullptr;

    if ((nullptr == ctx) || (nullptr == job))
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    p_ctx = ctx_map.get_ctx_ref(ctx->handle);
    if (nullptr == p_ctx)
    {
        ret = AIPU_STATUS_ERROR_INVALID_CTX;
    }
    else
    {
        ret = p_ctx->create_job(graph, job);
    }

finish:
    return ret;
}

aipu_status_t aipu_finish_job(const aipu_ctx_handle_t* ctx, uint64_t job_id, int32_t time_out)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::JobBase* job = nullptr;
    aipu_job_status_t status;

    ret = api_get_job(ctx, job_id, &job);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    ret = job->schedule();
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    if (time_out <= 0)
    {
        time_out = -1;
    }

    ret = job->get_status_blocking(&status, time_out);
    if ((AIPU_STATUS_SUCCESS == ret) && (AIPU_JOB_STATUS_DONE != status))
    {
        ret = AIPU_STATUS_ERROR_JOB_EXCEPTION;
    }

    return ret;
}

aipu_status_t aipu_flush_job(const aipu_ctx_handle_t* ctx, uint64_t id, aipu_job_handler_callback callback,
    void* priv)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::JobBase* job = nullptr;

    ret = api_get_job(ctx, id, &job);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    /* callback to be implemented */
    return job->schedule();
}

aipu_status_t aipu_get_job_status(const aipu_ctx_handle_t* ctx, uint64_t id, aipu_job_status_t* status)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::JobBase* job = nullptr;

    ret = api_get_job(ctx, id, &job);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    return job->get_status(status);
}

aipu_status_t aipu_clean_job(const aipu_ctx_handle_t* ctx, uint64_t id)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::GraphBase* graph = nullptr;

    ret = api_get_graph(ctx, aipudrv::get_graph_id(id), &graph);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    return graph->destroy_job(id);
}

aipu_status_t aipu_get_tensor_count(const aipu_ctx_handle_t* ctx, uint64_t id, aipu_tensor_type_t type,
    uint32_t* cnt)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::GraphBase* graph = nullptr;

    ret = api_get_graph(ctx, aipudrv::get_graph_id(id), &graph);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    return graph->get_tensor_count(type, cnt);
}

aipu_status_t aipu_get_tensor_descriptor(const aipu_ctx_handle_t* ctx, uint64_t id, aipu_tensor_type_t type,
    uint32_t tensor, aipu_tensor_desc_t* desc)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::GraphBase* graph = nullptr;

    ret = api_get_graph(ctx, aipudrv::get_graph_id(id), &graph);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    return graph->get_tensor_descriptor(type, tensor, desc);
}

aipu_status_t aipu_load_tensor(const aipu_ctx_handle_t* ctx, uint64_t job_id, uint32_t tensor, const void* data)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::JobBase* job = nullptr;

    ret = api_get_job(ctx, job_id, &job);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    return job->load_tensor(tensor, data);
}

aipu_status_t aipu_get_tensor(const aipu_ctx_handle_t* ctx, uint64_t job_id, aipu_tensor_type_t type, uint32_t tensor,
    void* data)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::JobBase* job = nullptr;

    ret = api_get_job(ctx, job_id, &job);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    return job->get_tensor(type, tensor, data);
}

aipu_status_t aipu_get_cluster_count(const aipu_ctx_handle_t* ctx, uint32_t* cnt)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::CtxRefMap& ctx_map = aipudrv::CtxRefMap::get_ctx_map();
    aipudrv::MainContext* p_ctx = nullptr;

    if (nullptr == ctx)
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    p_ctx = ctx_map.get_ctx_ref(ctx->handle);
    if (nullptr == p_ctx)
    {
        ret = AIPU_STATUS_ERROR_INVALID_CTX;
    }
    else
    {
        ret = p_ctx->get_cluster_count(cnt);
    }

finish:
    return ret;
}

aipu_status_t aipu_get_core_count(const aipu_ctx_handle_t* ctx, uint32_t cluster, uint32_t* cnt)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::CtxRefMap& ctx_map = aipudrv::CtxRefMap::get_ctx_map();
    aipudrv::MainContext* p_ctx = nullptr;

    if (nullptr == ctx)
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    p_ctx = ctx_map.get_ctx_ref(ctx->handle);
    if (nullptr == p_ctx)
    {
        ret = AIPU_STATUS_ERROR_INVALID_CTX;
    }
    else
    {
        ret = p_ctx->get_core_count(cluster, cnt);
    }

finish:
    return ret;
}

aipu_status_t aipu_debugger_get_job_info(const aipu_ctx_handle_t* ctx,
    uint64_t job, aipu_debugger_job_info_t* info)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::CtxRefMap& ctx_map = aipudrv::CtxRefMap::get_ctx_map();
    aipudrv::MainContext* p_ctx = nullptr;

    if (nullptr == ctx)
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    p_ctx = ctx_map.get_ctx_ref(ctx->handle);
    if (nullptr == p_ctx)
    {
        ret = AIPU_STATUS_ERROR_INVALID_CTX;
    }
    else
    {
        ret = p_ctx->debugger_get_job_info(job, info);
    }

finish:
    return ret;
}

aipu_status_t aipu_debugger_bind_job(const aipu_ctx_handle_t* ctx, uint32_t core_id, uint32_t job_id)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::JobBase* job = nullptr;

    ret = api_get_job(ctx, job_id, &job);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    return job->bind_core(core_id);
}

aipu_status_t aipu_debugger_run_job(const aipu_ctx_handle_t* ctx, uint32_t job_id)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::JobBase* job = nullptr;

    ret = api_get_job(ctx, job_id, &job);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    return job->debugger_run();
}

aipu_status_t aipu_debugger_malloc(const aipu_ctx_handle_t* ctx, uint32_t size, void** va)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::CtxRefMap& ctx_map = aipudrv::CtxRefMap::get_ctx_map();
    aipudrv::MainContext* p_ctx = nullptr;

    if (nullptr == ctx)
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    p_ctx = ctx_map.get_ctx_ref(ctx->handle);
    if (nullptr == p_ctx)
    {
        ret = AIPU_STATUS_ERROR_INVALID_CTX;
    }
    else
    {
        ret = p_ctx->debugger_malloc(size, va);
    }

finish:
    return ret;
}

aipu_status_t aipu_debugger_free(const aipu_ctx_handle_t* ctx, void* va)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::CtxRefMap& ctx_map = aipudrv::CtxRefMap::get_ctx_map();
    aipudrv::MainContext* p_ctx = nullptr;

    if (nullptr == ctx)
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    p_ctx = ctx_map.get_ctx_ref(ctx->handle);
    if (nullptr == p_ctx)
    {
        ret = AIPU_STATUS_ERROR_INVALID_CTX;
    }
    else
    {
        ret = p_ctx->debugger_free(va);
    }

finish:
    return ret;
}

aipu_status_t aipu_config_job(const aipu_ctx_handle_t* ctx, uint64_t job_id, uint64_t types, void* config)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::JobBase* job = nullptr;

    ret = api_get_job(ctx, job_id, &job);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        return ret;
    }

    if (types & (AIPU_JOB_CONFIG_TYPE_DUMP_TEXT |
         AIPU_JOB_CONFIG_TYPE_DUMP_WEIGHT       |
         AIPU_JOB_CONFIG_TYPE_DUMP_RODATA       |
         AIPU_JOB_CONFIG_TYPE_DUMP_DESCRIPTOR   |
         AIPU_JOB_CONFIG_TYPE_DUMP_INPUT        |
         AIPU_JOB_CONFIG_TYPE_DUMP_OUTPUT       |
         AIPU_JOB_CONFIG_TYPE_DUMP_TCB_CHAIN    |
         AIPU_JOB_CONFIG_TYPE_DUMP_EMULATION))
    {
        ret = job->config_mem_dump(types, (aipu_job_config_dump_t*)config);
    }
    else if (types == AIPU_CONFIG_TYPE_SIMULATION)
    {
        ret = job->config_simulation(types, (aipu_job_config_simulation_t*)config);
    }
    else
    {
        ret = AIPU_STATUS_ERROR_INVALID_CONFIG;
    }

    return ret;
}

aipu_status_t aipu_config_global(const aipu_ctx_handle_t* ctx, uint64_t types, void* config)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipudrv::CtxRefMap& ctx_map = aipudrv::CtxRefMap::get_ctx_map();
    aipudrv::MainContext* p_ctx = nullptr;

    if (nullptr == ctx)
    {
        ret = AIPU_STATUS_ERROR_NULL_PTR;
        goto finish;
    }

    p_ctx = ctx_map.get_ctx_ref(ctx->handle);
    if (nullptr == p_ctx)
    {
        ret = AIPU_STATUS_ERROR_INVALID_CTX;
    }
    else
    {
        if (types & AIPU_CONFIG_TYPE_SIMULATION)
        {
            ret = p_ctx->config_simulation(types, (aipu_global_config_simulation_t*)config);
            if (ret != AIPU_STATUS_SUCCESS)
            {
                goto finish;
            }
            types &= ~AIPU_CONFIG_TYPE_SIMULATION;
        }

        if (types & AIPU_GLOBAL_CONFIG_TYPE_DISABLE_VER_CHECK)
        {
            p_ctx->disable_version_check();
            types &= ~AIPU_GLOBAL_CONFIG_TYPE_DISABLE_VER_CHECK;
        }

        if (types & AIPU_GLOBAL_CONFIG_TYPE_ENABLE_VER_CHECK)
        {
            p_ctx->enable_version_check();
            types &= ~AIPU_GLOBAL_CONFIG_TYPE_ENABLE_VER_CHECK;
        }

        if (types)
        {
            ret = AIPU_STATUS_ERROR_INVALID_CONFIG;
        }
    }

finish:
    return ret;
}
