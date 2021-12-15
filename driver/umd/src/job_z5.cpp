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
 * @file  job_z5.cpp
 * @brief AIPU User Mode Driver (UMD) z5 job module implementation
 */

#include <cstring>
#include <assert.h>
#include "job_z5.h"
#include "graph_z5.h"
#include "parser_base.h"

aipudrv::JobZ5::JobZ5(const GraphBase& graph, DeviceBase* dev):
    JobBase(graph, dev)
{
    m_tcbs.reset();
    m_init_tcb.init(0);

    set_job_params(get_graph().m_subgraphs.size(), m_dev->tec_cnt_per_core(get_graph().m_hw_config),
        get_graph().m_remap_flag);
}

aipudrv::JobZ5::~JobZ5()
{
}

void aipudrv::JobZ5::set_job_params(uint32_t sg_cnt, uint32_t task_per_sg, uint32_t remap)
{
    m_sg_cnt = sg_cnt;
    m_task_per_sg = task_per_sg;
    m_tot_tcb_cnt = m_sg_cnt * m_task_per_sg + 1;
    m_remap_flag = remap;
}

aipu_status_t aipudrv::JobZ5::setup_rodata_sg(uint32_t sg_id)
{
    const std::vector<struct GraphParamMapLoadDesc>& param_map =
        get_graph().m_subgraphs[sg_id].param_map;
    std::vector<BufferDesc>& reuse_buf  = m_sg_job[sg_id].reuses;
    std::vector<BufferDesc>& static_buf = m_sg_job[sg_id].weights;
    BufferDesc rodata;
    rodata.init(m_rodata.pa + get_graph().m_subgraphs[sg_id].rodata.offset,
        get_graph().m_subgraphs[sg_id].rodata.size,
        get_graph().m_subgraphs[sg_id].rodata.size);
    BufferDesc dcr;
    dcr.init(m_descriptor.pa + get_graph().m_subgraphs[sg_id].dcr.offset,
        get_graph().m_subgraphs[sg_id].dcr.size,
        get_graph().m_subgraphs[sg_id].dcr.size);

    return setup_rodata(param_map, reuse_buf, static_buf, rodata, dcr);
}

aipu_status_t aipudrv::JobZ5::alloc_load_job_buffers()
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    SubGraphTask sg;

    /* 1. allocate and load job rodata */
    ret = m_mem->malloc(get_graph().m_brodata.size, 0, &m_rodata, "rodata");
    if (AIPU_STATUS_SUCCESS != ret)
    {
        goto finish;
    }
    assert(m_mem->write(m_rodata.pa, get_graph().m_brodata.va, get_graph().m_brodata.size)
        == (int)get_graph().m_brodata.size);

    /* 2. allocate and load job descriptor */
    if (get_graph().m_bdesc.size != 0)
    {
        ret = m_mem->malloc(get_graph().m_bdesc.size, 0, &m_descriptor, "dcr");
        if (AIPU_STATUS_SUCCESS != ret)
        {
            goto finish;
        }
        assert(m_mem->write(m_descriptor.pa, get_graph().m_bdesc.va, get_graph().m_bdesc.size)
            == (int)get_graph().m_bdesc.size);
    }

    /* 3. allocate and reset job TCBs */
    ret = m_mem->malloc(m_tot_tcb_cnt * sizeof(tcb_t), 0, &m_tcbs, "tcbs");
    if (AIPU_STATUS_SUCCESS != ret)
    {
        goto finish;
    }
    m_mem->bzero(m_tcbs.pa, m_tot_tcb_cnt * sizeof(tcb_t));
    m_init_tcb.init(m_tcbs.pa);

    /* 4. allocate subgraph buffers */
    for (uint32_t i = 0; i < m_sg_cnt; i++)
    {
        sg.reset();

        /* 4.1 allocate reuse buffers */
        for (uint32_t k = 0; k < get_graph().m_subgraphs[i].reuse_sections.size(); k++)
        {
            BufferDesc buf;
            buf.reset();
            if (get_graph().m_subgraphs[i].reuse_sections[k].size != 0)
            {
                char str[20];
                snprintf(str, 20, "reuse_%u", k);
                ret = m_mem->malloc(get_graph().m_subgraphs[i].reuse_sections[k].size,
                    get_graph().m_subgraphs[i].reuse_sections[k].align_in_page, &buf, str);
                if (AIPU_STATUS_SUCCESS != ret)
                {
                    goto finish;
                }
            }
            sg.reuses.push_back(buf);
        }

        /* 4.2 init task weights address */
        for (uint32_t w = 0; w < get_graph().m_subgraphs[i].static_sections.size(); w++)
        {
            BufferDesc buf;
            buf.init(get_graph().m_weight.pa + get_graph().m_subgraphs[i].static_sections[w].offset,
                get_graph().m_subgraphs[i].static_sections[w].size,
                get_graph().m_subgraphs[i].static_sections[w].size);
            sg.weights.push_back(buf);
        }

        /* 4.3 init per-task data structs */
        for (uint32_t j = 0; j < m_task_per_sg; j++)
        {
            Task task;
            memset(&task, 0, sizeof(task));

            /* 4.3.1. init task tcb */
            task.tcb.init(m_tcbs.pa + (i * m_task_per_sg + j + 1)* sizeof(tcb_t));

            /* 4.3.2. allocate task stack */
            ret = m_mem->malloc(get_graph().m_subgraphs[i].stack_size, get_graph().m_subgraphs[i].stack_align_in_page,
                &task.stack, "stack");
            if (AIPU_STATUS_SUCCESS != ret)
            {
                goto finish;
            }

            /* 4.3.3. allocate and load task dp */
            if (get_graph().m_bdata.size != 0)
            {
                ret = m_mem->malloc(get_graph().m_bdata.size, 0, &task.dp_cc, "data_cc");
                if (AIPU_STATUS_SUCCESS != ret)
                {
                    goto finish;
                }
                assert(m_mem->write(task.dp_cc.pa, get_graph().m_bdata.va, get_graph().m_bdata.size)
                    == (int)get_graph().m_bdata.size);
            }
            sg.tasks.push_back(task);
        }
        m_sg_job.push_back(sg);
    }

    /* 5. setup rodata & dcr */
    for (uint32_t i = 0; i < m_sg_cnt; i++)
    {
        ret = setup_rodata_sg(get_graph().m_subgraphs[i].id);
        if (AIPU_STATUS_SUCCESS != ret)
        {
            goto finish;
        }
    }

    /* 6. setup remap */
    setup_remap(m_rodata, m_descriptor);

    /* 7. get IO buffer address */
    /* only 1 sg */
    create_io_buffers(get_graph().m_subgraphs[0].io, m_sg_job[0].reuses);

finish:
    if (ret)
    {
        free_sg_buffers(sg);
        free_job_buffers();
    }
    return ret;
}

void aipudrv::JobZ5::free_sg_buffers(const SubGraphTask& sg)
{
    for (uint32_t i = 0; i < sg.reuses.size(); i++)
    {
        if (sg.reuses[i].size != 0)
        {
            m_mem->free(&sg.reuses[i]);
        }
    }

    for (uint32_t i = 0; i < sg.tasks.size(); i++)
    {
        if (sg.tasks[i].stack.size != 0)
        {
            m_mem->free(&sg.tasks[i].stack);
        }
        if (sg.tasks[i].dp_cc.size != 0)
        {
            m_mem->free(&sg.tasks[i].dp_cc);
        }
    }
}

aipu_status_t aipudrv::JobZ5::free_job_buffers()
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;

    if (m_rodata.size != 0)
    {
        m_mem->free(&m_rodata);
        m_rodata.reset();
    }
    if (m_descriptor.size != 0)
    {
        m_mem->free(&m_descriptor);
        m_descriptor.reset();
    }
    if (m_tcbs.size != 0)
    {
        m_mem->free(&m_tcbs);
        m_tcbs.reset();
    }
    m_init_tcb.init(0);

    for (uint32_t i = 0; i < m_sg_job.size(); i++)
    {
        free_sg_buffers(m_sg_job[i]);
    }

    m_sg_job.clear();
    m_inputs.clear();
    m_outputs.clear();
    m_inter_dumps.clear();
    m_profiler.clear();
    m_printf.clear();
    m_layer_counter.clear();

    return ret;
}

aipu_status_t aipudrv::JobZ5::setup_tcb_task(uint32_t sg_id, uint32_t task_id)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    Task& task = m_sg_job[sg_id].tasks[task_id];
    tcb_t* tcb = new tcb_t;
    TCB*   next_tcb = nullptr;

    if (task_id != (m_task_per_sg - 1))
    {
        next_tcb = &m_sg_job[sg_id].tasks[task_id + 1].tcb;
    }
    else if (sg_id != (m_sg_cnt - 1))
    {
        next_tcb = &m_sg_job[sg_id + 1].tasks[0].tcb;
    }
    else
    {
        next_tcb = nullptr;
    }

    memset(tcb, 0, sizeof(tcb_t));
    tcb->flag = TCB_FLAG_TASK_TYPE_TASK;
    if (next_tcb != nullptr)
    {
        tcb->next = get_low_32(next_tcb->pa);
    }
    else
    {
        tcb->next = 0;
        tcb->flag |= TCB_FLAG_END_TYPE_END_WITHOUT_DESTROY;
    }

    /* It is assumed that subgraphs are topology sorted. */
    if (get_graph().m_subgraphs[sg_id].precursors.size() == 0)
    {
        tcb->flag |= TCB_FLAG_DEP_TYPE_NONE;
    }
    else if (get_graph().m_subgraphs[sg_id].precursors.size() == 1)
    {
        tcb->flag |= TCB_FLAG_DEP_TYPE_IMMEDIATE;
    }
    else
    {
        tcb->flag |= TCB_FLAG_DEP_TYPE_PRE_ALL;
    }
    tcb->spc = get_low_32(get_graph().m_text.pa + get_graph().m_subgraphs[sg_id].text.offset);
    tcb->gridid = 0;
    tcb->groupid = 0;
    tcb->taskid = (uint16_t)task_id;
    tcb->grid_dim_x = 1;
    tcb->grid_dim_y = 1;
    tcb->grid_dim_z = 1;
    tcb->group_dim_x = m_sg_cnt * m_task_per_sg;
    tcb->group_dim_y = 1;
    tcb->group_dim_z = 1;
    tcb->group_id_x = 0;
    tcb->group_id_y = 0;
    tcb->group_id_z = 0;
    tcb->task_id_x = (uint16_t)task_id;
    tcb->task_id_y = 0;
    tcb->task_id_z = 0;
    tcb->sp = get_low_32(task.stack.pa);
    tcb->pp = get_low_32(m_rodata.pa + get_graph().m_subgraphs[sg_id].rodata.offset);
    tcb->dp = get_low_32(task.dp_cc.pa);
    if (m_sg_job[sg_id].weights.size() != 0)
    {
        tcb->cp = get_low_32(m_sg_job[sg_id].weights[0].pa);
    }
    else
    {
        tcb->cp = 0;
    }

    /* flush TCB to AIPU mem */
    assert(m_mem->write(task.tcb.pa, (const char*)tcb, sizeof(*tcb))
        == sizeof(*tcb));

    delete tcb;
    return ret;
}

aipu_status_t aipudrv::JobZ5::setup_tcb_sg(uint32_t sg_id)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;

    /* setup task TCBs */
    for (uint32_t t = 0; t < m_task_per_sg; t++)
    {
        ret = setup_tcb_task(sg_id, t);
        if (AIPU_STATUS_SUCCESS != ret)
        {
            return ret;
        }
    }

    return ret;
}

aipu_status_t aipudrv::JobZ5::setup_tcbs()
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    tcb_t* tcb = new tcb_t;

    /* setup init TCB */
    memset(tcb, 0, sizeof(tcb_t));
    tcb->flag = TCB_FLAG_TASK_TYPE_INIT;
    tcb->next = get_low_32(m_sg_job[0].tasks[0].tcb.pa);
    tcb->asids[0].hi = (get_graph().m_text.pa + get_graph().m_subgraphs[0].text.offset) >> 32;
    tcb->asids[0].lo = 0;
    tcb->asids[0].ctrl = 0xC0000000;
    tcb->asids[1].hi = tcb->asids[0].hi;
    tcb->asids[1].lo = 0;
    tcb->asids[1].ctrl = tcb->asids[0].ctrl;
    tcb->asids[2].hi = tcb->asids[0].hi;
    tcb->asids[2].lo = 0;
    tcb->asids[2].ctrl = tcb->asids[0].ctrl;
    tcb->asids[3].hi = tcb->asids[0].hi;
    tcb->asids[3].lo = 0;
    tcb->asids[3].ctrl = tcb->asids[0].ctrl;
    assert(m_mem->write(m_init_tcb.pa, (const char*)tcb, sizeof(*tcb))
        == sizeof(*tcb));

    /* setup task TCBs */
    for (uint32_t i = 0; i < get_graph().m_subgraphs.size(); i++)
    {
        ret = setup_tcb_sg(get_graph().m_subgraphs[i].id);
        if (AIPU_STATUS_SUCCESS != ret)
        {
            goto finish;
        }
    }

    m_status = AIPU_JOB_STATUS_INIT;

finish:
    delete tcb;
    return ret;
}

aipu_status_t aipudrv::JobZ5::init(const aipu_global_config_simulation_t* cfg)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;

    assert(cfg != nullptr);
    m_cfg = cfg;

    /* allocate and load job buffers */
    ret = alloc_load_job_buffers();
    if (AIPU_STATUS_SUCCESS != ret)
    {
        goto finish;
    }

    ret = setup_tcbs();
    if (AIPU_STATUS_SUCCESS != ret)
    {
        goto finish;
    }

finish:
    return ret;
}

aipu_status_t aipudrv::JobZ5::schedule()
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;
    JobDesc desc;

    ret = validate_schedule_status();
    if (ret != AIPU_STATUS_SUCCESS)
    {
        return ret;
    }

    if (m_status == AIPU_JOB_STATUS_DONE)
    {
        /* reload task TCBs because it was updated at runtime */
        for (uint32_t i = 0; i < get_graph().m_subgraphs.size(); i++)
        {
            ret = setup_tcb_sg(get_graph().m_subgraphs[i].id);
            if (AIPU_STATUS_SUCCESS != ret)
            {
                return ret;
            }
        }
    }

    dump_job_shared_buffers();
    dump_job_private_buffers(m_rodata, m_descriptor);
    dump_z5_specific_buffers();
    ret = dump_for_emulation();
    if (ret != AIPU_STATUS_SUCCESS)
    {
        return ret;
    }

    memset(&desc.kdesc, 0, sizeof(desc.kdesc));
    desc.kdesc.job_id = m_id;
    desc.kdesc.version_compatible = get_graph().m_do_vcheck;
    desc.kdesc.aipu_config = get_graph().m_hw_config;
    desc.tcb_head = m_init_tcb.pa;
    desc.tcb_tail = m_sg_job[m_sg_cnt-1].tasks[m_task_per_sg-1].tcb.pa;
    ret = m_dev->schedule(desc);
    m_status = AIPU_JOB_STATUS_SCHED;

    return ret;
}

aipu_status_t aipudrv::JobZ5::destroy()
{
    return free_job_buffers();
}

void aipudrv::JobZ5::dump_z5_specific_buffers()
{
    DEV_PA_64 dump_pa;
    uint32_t dump_size;

    if (m_dump_tcb)
    {
        dump_pa = m_tcbs.pa;
        dump_size = m_tot_tcb_cnt * sizeof(tcb_t);
        if (dump_size != 0)
        {
            dump_buffer(dump_pa, nullptr, dump_size, "TCBs");
        }
    }
}

aipu_status_t aipudrv::JobZ5::dump_for_emulation()
{
    DEV_PA_64 dump_pa;
    uint32_t dump_size;
    char dump_name[4096];
    char cfg_item[1024];
    FILE* fp = NULL;
    int emu_input_cnt = 3 + m_inputs.size() + (get_graph().m_bweight.size != 0 ? 1 : 0) +
        (m_descriptor.size != 0 ? 1 : 0);
    int emu_output_cnt = m_outputs.size();
    int file_id = -1;

    if (m_dump_emu == false)
    {
        return AIPU_STATUS_SUCCESS;
    }

    /* create runtime.cfg */
    snprintf(dump_name, sizeof(dump_name), "%s/runtime.cfg", m_dump_dir.c_str());
    fp = fopen(dump_name, "w");
    if (NULL == fp)
    {
        return AIPU_STATUS_ERROR_OPEN_FILE_FAIL;
    }

    /* runtime.cfg: [COMMON] */
    snprintf(cfg_item, sizeof(cfg_item), "[COMMON]\n");
    fputs(cfg_item, fp);

    /* runtime.cfg: config */
    snprintf(cfg_item, sizeof(cfg_item),
        "#configuration 0:Z5_1308 1:Z5_1204 2:Z5_1308_MP4 3:Z5_0901\n");
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "CONFIG=%d\n", m_dev->get_config_code());
    fputs(cfg_item, fp);

    /* runtime.cfg: enable_avx */
    snprintf(cfg_item, sizeof(cfg_item),
        "#if ENABLE_AVX is true then using the intel SIMD instructions to speedup.\n");
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "ENABLE_AVX=%s\n", m_cfg->enable_avx ? "true" : "false");
    fputs(cfg_item, fp);

    /* runtime.cfg: log file path */
    snprintf(cfg_item, sizeof(cfg_item), "#Where log output to store is.\n");
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "LOG_FILEPATH=%s\n", m_cfg->log_file_path);
    fputs(cfg_item, fp);

    /* runtime.cfg: log_level */
    snprintf(cfg_item, sizeof(cfg_item),
        "#which level is your selected: 0:ERROR, 1: WARN, 2: INFO, 3: DEBUG\n");
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "LOG_LEVEL=%d\n", m_cfg->log_level);
    fputs(cfg_item, fp);

    /* runtime.cfg: verbose */
    snprintf(cfg_item, sizeof(cfg_item),
        "#if LOG_VERBOSE is true then print log to console. otherwise no\n");
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "LOG_VERBOSE=%s\n", m_cfg->verbose ? "true" : "false");
    fputs(cfg_item, fp);

    /* runtime.cfg: enable_calloc */
    snprintf(cfg_item, sizeof(cfg_item),
        "##if ENABLE_CALLOC is true the allocation memory is set to zero.\n");
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "ENABLE_CALLOC=%s\n", m_cfg->enable_calloc ? "true" : "false");
    fputs(cfg_item, fp);
    fputs("\n", fp);

    /* runtime.cfg: [INPUT] */
    snprintf(cfg_item, sizeof(cfg_item), "[INPUT]\n");
    fputs(cfg_item, fp);

    snprintf(cfg_item, sizeof(cfg_item), "COUNT=%d\n", emu_input_cnt);
    fputs(cfg_item, fp);

    /* dump temp.text */
    dump_pa = get_graph().m_text.pa;
    dump_size = get_graph().m_btext.size;
    snprintf(dump_name, 128, "%s/%s.text", m_dump_dir.c_str(), m_dump_prefix.c_str());
    m_mem->dump_file(dump_pa, dump_name, dump_size);
    snprintf(cfg_item, sizeof(cfg_item), "FILE%d=%s.text\n", ++file_id, m_dump_prefix.c_str());
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "BASE%d=0x%lx\n", file_id, dump_pa);
    fputs(cfg_item, fp);

    /* dump temp.weight */
    dump_pa = get_graph().m_weight.pa;
    dump_size = get_graph().m_bweight.size;
    if (dump_size != 0)
    {
        snprintf(dump_name, 128, "%s/%s.weight", m_dump_dir.c_str(), m_dump_prefix.c_str());
        m_mem->dump_file(dump_pa, dump_name, dump_size);
        snprintf(cfg_item, sizeof(cfg_item), "FILE%d=%s.weight\n", ++file_id, m_dump_prefix.c_str());
        fputs(cfg_item, fp);
        snprintf(cfg_item, sizeof(cfg_item), "BASE%d=0x%lx\n", file_id, dump_pa);
        fputs(cfg_item, fp);
    }

    /* dump temp.rodata */
    dump_pa = m_rodata.pa;
    dump_size = m_rodata.size;
    snprintf(dump_name, 128, "%s/%s.ro", m_dump_dir.c_str(), m_dump_prefix.c_str());
    m_mem->dump_file(dump_pa, dump_name, dump_size);
    snprintf(cfg_item, sizeof(cfg_item), "FILE%d=%s.ro\n", ++file_id, m_dump_prefix.c_str());
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "BASE%d=0x%lx\n", file_id, dump_pa);
    fputs(cfg_item, fp);

    /* dump temp.dcr */
    if (m_descriptor.size != 0)
    {
        dump_pa = m_descriptor.pa;
        dump_size = m_descriptor.size;
        snprintf(dump_name, 128, "%s/%s.dcr", m_dump_dir.c_str(), m_dump_prefix.c_str());
        m_mem->dump_file(dump_pa, dump_name, dump_size);
        snprintf(cfg_item, sizeof(cfg_item), "FILE%d=%s.dcr\n", ++file_id, m_dump_prefix.c_str());
        fputs(cfg_item, fp);
        snprintf(cfg_item, sizeof(cfg_item), "BASE%d=0x%lx\n", file_id, dump_pa);
        fputs(cfg_item, fp);
    }

    /* dump temp.tcb */
    dump_pa = m_tcbs.pa;
    dump_size = m_tot_tcb_cnt * sizeof(tcb_t);
    snprintf(dump_name, 128, "%s/%s.tcb", m_dump_dir.c_str(), m_dump_prefix.c_str());
    m_mem->dump_file(dump_pa, dump_name, dump_size);
    snprintf(cfg_item, sizeof(cfg_item), "FILE%d=%s.tcb\n", ++file_id, m_dump_prefix.c_str());
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "BASE%d=0x%lx\n", file_id, dump_pa);
    fputs(cfg_item, fp);

    /* dump temp.input[n] */
    for (uint32_t i = 0; i < m_inputs.size(); i++)
    {
        dump_pa   = m_inputs[i].pa;
        dump_size = m_inputs[i].size;
        snprintf(dump_name, 128, "%s/%s.input%d", m_dump_dir.c_str(), m_dump_prefix.c_str(), i);
        m_mem->dump_file(dump_pa, dump_name, dump_size);
        snprintf(cfg_item, sizeof(cfg_item), "FILE%d=%s.input%d\n", ++file_id, m_dump_prefix.c_str(), i);
        fputs(cfg_item, fp);
        snprintf(cfg_item, sizeof(cfg_item), "BASE%d=0x%lx\n", file_id, dump_pa);
        fputs(cfg_item, fp);
    }
    fputs("\n", fp);

    /* runtime.cfg: [HOST] */
    snprintf(cfg_item, sizeof(cfg_item), "[HOST]\n");
    fputs(cfg_item, fp);

    snprintf(cfg_item, sizeof(cfg_item), "TCBP_HI=0x%x\n", get_high_32(m_init_tcb.pa));
    fputs(cfg_item, fp);
    snprintf(cfg_item, sizeof(cfg_item), "TCBP_LO=0x%x\n", get_low_32(m_init_tcb.pa));
    fputs(cfg_item, fp);
    fputs("\n", fp);

    /* runtime.cfg: [OUTPUT] */
    snprintf(cfg_item, sizeof(cfg_item), "[OUTPUT]\n");
    fputs(cfg_item, fp);

    snprintf(cfg_item, sizeof(cfg_item), "COUNT=%d\n", emu_output_cnt);
    fputs(cfg_item, fp);

    /* dump temp.output[n] */
    for (uint32_t i = 0; i < m_outputs.size(); i++)
    {
        dump_pa   = m_outputs[i].pa;
        dump_size = m_outputs[i].size;
        snprintf(cfg_item, sizeof(cfg_item), "FILE%d=%s.output%d\n", i, m_dump_prefix.c_str(), i);
        fputs(cfg_item, fp);
        snprintf(cfg_item, sizeof(cfg_item), "BASE%d=0x%lx\n", i, dump_pa);
        fputs(cfg_item, fp);
        snprintf(cfg_item, sizeof(cfg_item), "SIZE%d=0x%x\n", i, dump_size);
        fputs(cfg_item, fp);
    }

    fclose(fp);
    return AIPU_STATUS_SUCCESS;
}