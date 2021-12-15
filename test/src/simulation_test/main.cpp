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
 * @file  main.cpp
 * @brief Z5 AIPU UMD test application: basic simulation test
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include <string.h>
#include <errno.h>
#include <vector>
#include <math.h>
#include "standard_api.h"
#include "common/cmd_line_parsing.h"
#include "common/helper.h"

using namespace std;

int main(int argc, char* argv[])
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    aipu_ctx_handle_t* ctx;
    const char* msg = nullptr;
    uint32_t cluster_cnt, core_cnt;
    uint64_t graph_id, job_id;
    uint32_t input_cnt, output_cnt;
    vector<aipu_tensor_desc_t> input_desc;
    vector<char*> input_data;
    vector<aipu_tensor_desc_t> output_desc;
    vector<char*> output_data;
    vector<char*> gt;
    cmd_opt_t opt;
    uint32_t frame_cnt = 1;
    int pass = 0;
    uint64_t cfg_types = 0;

    /**
     * For compatibility and avoiding segfault issues in the future,
     * strongly suggest to memset the config struct to be zero because the structs
     * are updated time to time.
     */
    aipu_global_config_simulation_t sim_glb_config;
    memset(&sim_glb_config, 0, sizeof(sim_glb_config));
    aipu_job_config_simulation_t sim_job_config;
    memset(&sim_job_config, 0, sizeof(sim_job_config));
    aipu_job_config_dump_t mem_dump_config;
    memset(&mem_dump_config, 0, sizeof(mem_dump_config));

    if(init_test_bench(argc, argv, &opt, "simulation_test"))
    {
        fprintf(stderr, "[TEST ERROR] invalid command line options/args\n");
        goto finish;
    }

    /* works for z1/2/3/5 simulation and execution on AIPU */
    mem_dump_config.dump_dir = opt.dump_dir;

    /* works for z1/2/3/5 simulation */
    if (opt.log_level_set)
    {
        sim_glb_config.log_level = opt.log_level;
    }
    else
    {
#if ((defined RTDEBUG) && (RTDEBUG == 1))
        sim_glb_config.log_level = 3;
#else
        sim_glb_config.log_level = 0;
#endif
    }
    sim_glb_config.verbose = opt.verbose;

    /* works for z1/2/3 simulation only */
    sim_glb_config.z1_simulator = opt.z1_simulator;
    sim_glb_config.z2_simulator = opt.z2_simulator;
    sim_glb_config.z3_simulator = opt.z3_simulator;
    sim_job_config.data_dir     = opt.dump_dir;

    ret = aipu_init_context(&ctx);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        aipu_get_error_message(ctx, ret, &msg);
        fprintf(stderr, "[TEST ERROR] aipu_init_context: %s\n", msg);
        goto finish;
    }
    fprintf(stdout, "[TEST INFO] aipu_init_context success\n");

    ret = aipu_config_global(ctx, AIPU_CONFIG_TYPE_SIMULATION, &sim_glb_config);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        aipu_get_error_message(ctx, ret, &msg);
        fprintf(stderr, "[TEST ERROR] aipu_config_global: %s\n", msg);
        goto finish;
    }
    fprintf(stdout, "[TEST INFO] set global simulation config success\n");

    ret = aipu_load_graph(ctx, opt.bin_file_name, &graph_id);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        aipu_get_error_message(ctx, ret, &msg);
        fprintf(stderr, "[TEST ERROR] aipu_load_graph_helper: %s (%s)\n",
            msg, opt.bin_file_name);
        goto finish;
    }
    fprintf(stdout, "[TEST INFO] aipu_load_graph_helper success: %s\n", opt.bin_file_name);

    ret = aipu_get_cluster_count(ctx, &cluster_cnt);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        aipu_get_error_message(ctx, ret, &msg);
        fprintf(stderr, "[TEST ERROR] aipu_get_cluster_count: %s (%s)\n",
            msg, opt.bin_file_name);
        goto finish;
    }
    //fprintf(stdout, "[TEST INFO] aipu_get_cluster_count success: cnt = %u\n", cluster_cnt);

    ret = aipu_get_core_count(ctx, 0, &core_cnt);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        aipu_get_error_message(ctx, ret, &msg);
        fprintf(stderr, "[TEST ERROR] aipu_get_core_count: %s (%s)\n",
            msg, opt.bin_file_name);
        goto finish;
    }
    //fprintf(stdout, "[TEST INFO] aipu_get_core_count success: cnt = %u\n", core_cnt);

    ret = aipu_get_tensor_count(ctx, graph_id, AIPU_TENSOR_TYPE_INPUT, &input_cnt);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        aipu_get_error_message(ctx, ret, &msg);
        fprintf(stderr, "[TEST ERROR] aipu_get_tensor_count: %s\n", msg);
        goto finish;
    }
    //fprintf(stdout, "[TEST INFO] aipu_get_tensor_count success: input cnt = %d\n", input_cnt);

    for (uint32_t i = 0; i < input_cnt; i++)
    {
        aipu_tensor_desc_t desc;
        ret = aipu_get_tensor_descriptor(ctx, graph_id, AIPU_TENSOR_TYPE_INPUT, i, &desc);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(ctx, ret, &msg);
            fprintf(stderr, "[TEST ERROR] aipu_get_tensor_descriptor: %s\n", msg);
            goto finish;
        }
        input_desc.push_back(desc);
    }

    ret = aipu_get_tensor_count(ctx, graph_id, AIPU_TENSOR_TYPE_OUTPUT, &output_cnt);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        aipu_get_error_message(ctx, ret, &msg);
        fprintf(stderr, "[TEST ERROR] aipu_get_tensor_count: %s\n", msg);
        goto finish;
    }
    //fprintf(stdout, "[TEST INFO] aipu_get_tensor_count success: output cnt = %d\n", output_cnt);

    for (uint32_t i = 0; i < output_cnt; i++)
    {
        aipu_tensor_desc_t desc;
        ret = aipu_get_tensor_descriptor(ctx, graph_id, AIPU_TENSOR_TYPE_OUTPUT, i, &desc);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(ctx, ret, &msg);
            fprintf(stderr, "[TEST ERROR] aipu_get_tensor_descriptor: %s\n", msg);
            goto finish;
        }
        output_desc.push_back(desc);
    }
    //fprintf(stderr, "[TEST INFO] aipu_get_tensor_descriptor done\n");

    ret = aipu_create_job(ctx, graph_id, &job_id);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        aipu_get_error_message(ctx, ret, &msg);
        fprintf(stderr, "[TEST ERROR] aipu_create_job: %s\n", msg);
        goto finish;
    }
    fprintf(stdout, "[TEST INFO] aipu_create_job success\n");

#if ((defined RTDEBUG) && (RTDEBUG == 1))
    cfg_types = AIPU_JOB_CONFIG_TYPE_DUMP_TEXT  |
        AIPU_JOB_CONFIG_TYPE_DUMP_WEIGHT        |
        AIPU_JOB_CONFIG_TYPE_DUMP_RODATA        |
        AIPU_JOB_CONFIG_TYPE_DUMP_DESCRIPTOR    |
        AIPU_JOB_CONFIG_TYPE_DUMP_INPUT         |
        AIPU_JOB_CONFIG_TYPE_DUMP_OUTPUT        |
        AIPU_JOB_CONFIG_TYPE_DUMP_TCB_CHAIN;
#else
    cfg_types = AIPU_JOB_CONFIG_TYPE_DUMP_OUTPUT;
#endif
    ret = aipu_config_job(ctx, job_id, cfg_types, &mem_dump_config);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        aipu_get_error_message(ctx, ret, &msg);
        fprintf(stderr, "[TEST ERROR] aipu_config_job: %s\n", msg);
        goto finish;
    }
    fprintf(stdout, "[TEST INFO] set dump config success\n");

    ret = aipu_config_job(ctx, job_id, AIPU_CONFIG_TYPE_SIMULATION, &sim_job_config);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        aipu_get_error_message(ctx, ret, &msg);
        fprintf(stderr, "[TEST ERROR] aipu_config_job: %s\n", msg);
        goto finish;
    }
    fprintf(stdout, "[TEST INFO] set job simulation config success\n");

    if (opt.inputs.size() != input_cnt)
    {
        fprintf(stdout, "[TEST WARN] input file count (%u) != input tensor count (%u)\n",
            (uint32_t)opt.inputs.size(), input_cnt);
    }

    for (uint32_t i = 0; i < output_cnt; i++)
    {
        char* output = new char[output_desc[i].size];
        output_data.push_back(output);
    }

    /* run with with multiple frames */

    for (uint32_t frame = 0; frame < frame_cnt; frame++)
    {
        fprintf(stdout, "[TEST INFO] Frame #%u\n", frame);
        for (uint32_t i = 0; i < min((uint32_t)opt.inputs.size(), input_cnt); i++)
        {
            if (input_desc[i].size > opt.inputs_size[i])
            {
                fprintf(stderr, "[TEST ERROR] input file %s len 0x%x < input tensor %u size 0x%x\n",
                    opt.input_files[i].c_str(), opt.inputs_size[i], i, input_desc[i].size);
                goto finish;
            }
            ret = aipu_load_tensor(ctx, job_id, i, opt.inputs[i]);
            if (ret != AIPU_STATUS_SUCCESS)
            {
                aipu_get_error_message(ctx, ret, &msg);
                fprintf(stderr, "[TEST ERROR] aipu_load_tensor: %s\n", msg);
                goto finish;
            }
            fprintf(stdout, "[TEST INFO] load input tensor %d from %s (%u/%u)\n",
                i, opt.input_files[i].c_str(), i+1, input_cnt);
        }

        ret = aipu_finish_job(ctx, job_id, -1);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(ctx, ret, &msg);
            fprintf(stderr, "[TEST ERROR] aipu_finish_job: %s\n", msg);
            pass = -1;
            goto finish;
        }
        fprintf(stdout, "[TEST INFO] aipu_finish_job success\n");

        for (uint32_t i = 0; i < output_cnt; i++)
        {
            ret = aipu_get_tensor(ctx, job_id, AIPU_TENSOR_TYPE_OUTPUT, i, output_data[i]);
            if (ret != AIPU_STATUS_SUCCESS)
            {
                aipu_get_error_message(ctx, ret, &msg);
                fprintf(stderr, "[TEST ERROR] aipu_get_tensor: %s\n", msg);
                goto finish;
            }
            fprintf(stdout, "[TEST INFO] get output tensor %u success (%u/%u)\n",
                i, i+1, output_cnt);
        }

        pass = check_result_helper(output_data, output_desc, opt.gt, opt.gt_size);
    }

    ret = aipu_clean_job(ctx, job_id);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        aipu_get_error_message(ctx, ret, &msg);
        fprintf(stderr, "[TEST ERROR] aipu_clean_job: %s\n", msg);
        goto finish;
    }
    //fprintf(stdout, "[TEST INFO] aipu_clean_job success\n");

    ret = aipu_unload_graph(ctx, graph_id);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        aipu_get_error_message(ctx, ret, &msg);
        fprintf(stderr, "[TEST ERROR] aipu_unload_graph: %s\n", msg);
        goto finish;
    }
    //fprintf(stdout, "[TEST INFO] aipu_unload_graph success\n");

    ret = aipu_deinit_context(ctx);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        aipu_get_error_message(ctx, ret, &msg);
        fprintf(stderr, "[TEST ERROR] aipu_deinit_ctx: %s\n", msg);
        goto finish;
    }
    //fprintf(stdout, "[TEST INFO] aipu_deinit_ctx success\n");

finish:
    if (AIPU_STATUS_SUCCESS != ret)
    {
        pass = -1;
    }
    for (uint32_t i = 0; i < output_data.size(); i++)
    {
        delete[] output_data[i];
    }
    deinit_test_bench(&opt);
    return pass;
}