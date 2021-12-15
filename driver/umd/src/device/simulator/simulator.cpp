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
 * @file  simulator.cpp
 * @brief AIPU User Mode Driver (UMD) zhouyi z1/2/3 simulator module implementation
 */

#include <cstring>
#include <unistd.h>
#include <assert.h>
#include "simulator.h"
#include "parser_base.h"

aipudrv::Simulator* aipudrv::Simulator::m_sim = nullptr;

aipudrv::Simulator::Simulator()
{
    m_dev_type = DEV_TYPE_SIMULATOR_LEGACY;
    m_dram = UMemory::get_memory();
}

aipudrv::Simulator::~Simulator()
{
    delete m_dram;
}

bool aipudrv::Simulator::has_target(uint32_t arch, uint32_t version, uint32_t config, uint32_t rev)
{
    if ((arch != 0) || (version > 3))
    {
        return false;
    }

    return true;
}

aipu_status_t aipudrv::Simulator::create_simulation_input_file(char* fname, const char* interfix,
    JOB_ID id, DEV_PA_64 pa, uint32_t size, const JobDesc& job)
{
    snprintf(fname, FNAME_LEN, "%s/Simulation_JOB0x%lx_%s_Base0x%lx_Size0x%x.bin",
        job.output_dir.c_str(), id, interfix, pa, size);
    return m_dram->dump_file(pa, fname, size);
}

aipu_status_t aipudrv::Simulator::update_simulation_rtcfg(const JobDesc& job, SimulationJobCtx& ctx)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    char fname[FNAME_LEN];
    char cfg_fname[FNAME_LEN];
    char cfg_item[OPT_LEN];
    FILE* fp = NULL;
    uint32_t input_data_cnt;

    /* text */
    ret = create_simulation_input_file(fname, "Text", job.kdesc.job_id, job.instruction_base_pa, job.text_size, job);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        goto finish;
    }
    ctx.text = fname;

    /* weight */
    ret = create_simulation_input_file(fname, "Weight", job.kdesc.job_id, job.weight_pa, job.weight_size, job);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        goto finish;
    }
    ctx.weight = fname;

    /* rodata */
    ret = create_simulation_input_file(fname, "Rodata", job.kdesc.job_id, job.kdesc.data_0_addr, job.rodata_size, job);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        goto finish;
    }
    ctx.rodata = fname;

    /* dcr */
    ret = create_simulation_input_file(fname, "Descriptor", job.kdesc.job_id, job.dcr_pa, job.dcr_size, job);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        goto finish;
    }
    ctx.dcr = fname;

    /* stack */
    ret = create_simulation_input_file(fname, "Stack", job.kdesc.job_id, job.kdesc.data_1_addr, job.stack_size, job);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        goto finish;
    }
    ctx.stack = fname;

    /* reuse */
    for (uint32_t i = 0; i < job.reuses.size(); i++)
    {
        char inter_fix[32];
        snprintf(inter_fix, 32, "Reuse%u", i);
        ret = create_simulation_input_file(fname, inter_fix, job.kdesc.job_id, job.reuses[i].pa, job.reuses[i].size, job);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            goto finish;
        }
        ctx.reuses.push_back(fname);
    }

    /* output */
    for (uint32_t i = 0; i < job.outputs.size(); i++)
    {
        char inter_fix[32];
        snprintf(inter_fix, 32, "Output%u", i);
        ret = create_simulation_input_file(fname, inter_fix, job.kdesc.job_id, job.outputs[i].pa, job.outputs[i].size, job);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            goto finish;
        }
        ctx.outputs.push_back(fname);
    }

    /* create config file */
    snprintf(cfg_fname, FNAME_LEN, "%s/runtime.cfg", job.output_dir.c_str());
    fp = fopen(cfg_fname, "w");
    if (NULL == fp)
    {
        ret = AIPU_STATUS_ERROR_OPEN_FILE_FAIL;
        goto finish;
    }

    /* init config file */
    if (job.aipu_revision == 0)
    {
        snprintf(cfg_item, sizeof(cfg_item), "CONFIG=Z%d-%04d\n", job.kdesc.aipu_version, job.kdesc.aipu_config);
    }
    else if (job.aipu_revision == AIPU_REVISION_P)
    {
        snprintf(cfg_item, sizeof(cfg_item), "CONFIG=Z%d-%04dp\n", job.kdesc.aipu_version, job.kdesc.aipu_config);
    }
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "LOG_LEVEL=%u\n", job.log_level);
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "LOG_FILE=log_default\n");
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "FAST_FWD_INST=0\n");
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "INPUT_INST_CNT=1\n");
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "INPUT_INST_FILE0=%s\n", ctx.text.c_str());
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "INPUT_INST_BASE0=0x%x\n", (uint32_t)job.instruction_base_pa);
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "INPUT_INST_STARTPC0=0x%x\n", (uint32_t)job.kdesc.start_pc_addr);
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "INT_PC=0x%x\n", (uint32_t)job.kdesc.intr_handler_addr);
    fputs(cfg_item, fp);
    input_data_cnt = 4 + job.reuses.size();
    assert(job.reuses.size() == ctx.reuses.size());
    snprintf(cfg_item, sizeof(cfg_item), "INPUT_DATA_CNT=%u\n", input_data_cnt);
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "INPUT_DATA_FILE0=%s\n", ctx.rodata.c_str());
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "INPUT_DATA_BASE0=0x%x\n", (uint32_t)job.kdesc.data_0_addr);
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "INPUT_DATA_FILE1=%s\n", ctx.stack.c_str());
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "INPUT_DATA_BASE1=0x%x\n", (uint32_t)job.kdesc.data_1_addr);
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "INPUT_DATA_FILE2=%s\n", ctx.dcr.c_str());
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "INPUT_DATA_BASE2=0x%x\n", (uint32_t)job.dcr_pa);
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "INPUT_DATA_FILE3=%s\n", ctx.weight.c_str());
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "INPUT_DATA_BASE3=0x%x\n", (uint32_t)job.weight_pa);
    fputs(cfg_item, fp);
    for(uint32_t i = 0; i < job.reuses.size(); i++)
    {
        snprintf(cfg_item, sizeof(cfg_item), "INPUT_DATA_FILE%u=%s\n", 4 + i, ctx.reuses[i].c_str());
        fputs(cfg_item, fp);
        snprintf(cfg_item, sizeof(cfg_item), "INPUT_DATA_BASE%u=0x%x\n", 4 + i, (uint32_t)job.reuses[i].pa);
        fputs(cfg_item, fp);
    }

    snprintf(cfg_item, sizeof(cfg_item), "OUTPUT_DATA_CNT=%u\n", (uint32_t)job.outputs.size());
    fputs(cfg_item, fp);
    for (uint32_t i = 0; i < job.outputs.size(); i++)
    {
        snprintf(cfg_item, sizeof(cfg_item), "OUTPUT_DATA_FILE%u=%s\n", i, ctx.outputs[i].c_str());
        fputs(cfg_item, fp);
        snprintf(cfg_item, sizeof(cfg_item), "OUTPUT_DATA_BASE%u=0x%x\n", i, (uint32_t)job.outputs[i].pa);
        fputs(cfg_item, fp);
        snprintf(cfg_item, sizeof(cfg_item), "OUTPUT_DATA_SIZE%u=0x%x\n", i, (uint32_t)job.outputs[i].size);
        fputs(cfg_item, fp);
    }
    snprintf(cfg_item, sizeof(cfg_item), "RUN_DESCRIPTOR=BIN[0]\n");
    fputs(cfg_item, fp);
    fclose(fp);

    snprintf(ctx.simulation_cmd, sizeof(ctx.simulation_cmd), "%s %s", job.simulator.c_str(), cfg_fname);

finish:
    return ret;
}

aipu_status_t aipudrv::Simulator::schedule(const JobDesc& job)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    int sys_ret = 0;
    SimulationJobCtx ctx;

    ret = update_simulation_rtcfg(job, ctx);
    if (ret != AIPU_STATUS_SUCCESS)
    {
        goto error;
    }

    LOG(LOG_DEFAULT, "[UMD SIMULATION] %s", ctx.simulation_cmd);
    sys_ret = system(ctx.simulation_cmd);
    if (sys_ret == -1)
    {
        LOG(LOG_ERR, "Simulation execution failed!");
        goto error;
    }
    else if (WIFEXITED(sys_ret) && (WEXITSTATUS(sys_ret) != 0))
    {
        LOG(LOG_ERR, "Simulation execution failed! (simulator ret = %d)", WEXITSTATUS(sys_ret));
        goto error;
    }
    else if (WIFSIGNALED(sys_ret))
    {
        LOG(LOG_ERR, "Simulation terminated by signal %d!", WTERMSIG(sys_ret));
        goto error;
    }

    for (uint32_t i = 0; i < ctx.outputs.size(); i++)
    {
        ret = m_dram->load_file(job.outputs[i].pa, ctx.outputs[i].c_str(), job.outputs[i].size);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            goto error;
        }
    }

error:
    return ret;
}

aipu_status_t aipudrv::Simulator::set_sim_log_level(uint32_t level)
{
    return AIPU_STATUS_SUCCESS;
}