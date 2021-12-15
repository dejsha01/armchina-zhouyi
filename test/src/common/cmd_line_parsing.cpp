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
 * @file  cmd_line_parsing.cpp
 * @brief AIPU UMD test implementation file: command line parsing
 */

#include <iostream>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include "cmd_line_parsing.h"
#include "helper.h"

static struct option opts[] = {
    { "bin", required_argument, NULL, 'b' },
    { "idata", required_argument, NULL, 'i' },
    { "check", required_argument, NULL, 'c' },
    { "dump_dir", required_argument, NULL, 'd' },
    { "z1_sim", optional_argument, NULL, 'z' },
    { "z2_sim", optional_argument, NULL, 'q' },
    { "z3_sim", optional_argument, NULL, 'k' },
    { "log_level", optional_argument, NULL, 'l' },
    { "verbose", optional_argument, NULL, 'v' },
    { NULL, 0, NULL, 0 }
};

int init_test_bench(int argc, char* argv[], cmd_opt_t* opt, const char* test_case)
{
    int ret = 0;
    extern char *optarg;
    int opt_idx = 0;
    int c = 0;
    char* temp = nullptr;
    char* dest = nullptr;
    uint32_t size = 0;

    if (nullptr == opt)
    {
        fprintf(stderr, "[TEST ERROR] invalid null pointer!\n");
        return -1;
    }

    while (1)
    {
        c = getopt_long (argc, argv, "hs:C:b:i:c:d:s:z:q:k:", opts, &opt_idx);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
        case 0:
            if (opts[opt_idx].flag != 0)
            {
                break;
            }
            fprintf (stdout, "option %s", opts[opt_idx].name);
            if (optarg)
            {
                printf (" with arg %s", optarg);
            }
            printf ("\n");
            break;

        case 'b':
            strcpy(opt->bin_file_name, optarg);
            break;

        case 'i':
            strcpy(opt->inputs_file_name, optarg);
            break;

        case 'c':
            strcpy(opt->gt_file_name, optarg);
            break;

        case 'd':
            strcpy(opt->dump_dir, optarg);
            break;

        case 'z':
            strcpy(opt->z1_simulator, optarg);
            break;

        case 'q':
            strcpy(opt->z2_simulator, optarg);
            break;

        case 'k':
            strcpy(opt->z3_simulator, optarg);
            break;

        case 'l':
            opt->log_level_set = true;
            opt->log_level = atoi(optarg);
            break;

        case 'v':
            opt->verbose = true;
            break;

        case '?':
            break;

        default:
            break;
        }
    }

    temp = strtok(opt->inputs_file_name, ",");
    opt->input_files.push_back(temp);
    while (temp)
    {
        temp = strtok(nullptr, ",");
        if (temp != nullptr)
        {
            opt->input_files.push_back(temp);
        }
    }

    for (uint32_t i = 0; i < opt->input_files.size(); i++)
    {
        ret = load_file_helper(opt->input_files[i].c_str(), &dest, &size);
        if (ret != 0)
        {
            goto finish;
        }
        opt->inputs.push_back(dest);
        opt->inputs_size.push_back(size);
    }

    ret = load_file_helper(opt->gt_file_name, &dest, &size);
    if (ret != 0)
    {
        goto finish;
    }
    opt->gt = dest;
    opt->gt_size = size;

finish:
    if (ret != 0)
    {
        deinit_test_bench(opt);
    }
    return ret;
}

int deinit_test_bench(cmd_opt_t* opt)
{
    if (opt == nullptr)
    {
        return 0;
    }

    for (uint32_t i = 0; i < opt->inputs.size(); i++)
    {
        unload_file_helper(opt->inputs[i]);
    }
    opt->input_files.clear();
    opt->inputs_size.clear();
    opt->inputs.clear();

    unload_file_helper(opt->gt);
    opt->gt_size = 0;
    return 0;
}