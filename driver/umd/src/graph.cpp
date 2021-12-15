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
 * @file  graph.cpp
 * @brief AIPU User Mode Driver (UMD) graph module implementation
 */

#include <cstring>
#include "graph.h"
#include "parser_base.h"
#include "utils/helper.h"
#include "utils/log.h"

aipudrv::Graph::Graph(GRAPH_ID id, DeviceBase* dev): GraphBase(id, dev)
{
    m_btext.init(nullptr, 0);
    m_brodata.init(nullptr, 0);
    m_bdesc.init(nullptr, 0);
    m_bweight.init(nullptr, 0);
    m_bdata.init(nullptr, 0);
    m_text.reset();
    m_weight.reset();
}

aipudrv::Graph::~Graph()
{
}

aipu_status_t aipudrv::Graph::load(std::ifstream& gbin, uint32_t size, bool ver_check)
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;

    ret = m_parser->parse_graph(gbin, size, *this);
    if (AIPU_STATUS_SUCCESS != ret)
    {
        goto finish;
    }

    m_do_vcheck = ver_check;
    if (ver_check && !m_dev->has_target(m_arch, m_hw_version, m_hw_config, m_hw_revision))
    {
        return AIPU_STATUS_ERROR_TARGET_NOT_FOUND;
    }

    m_mem->dump_tracking_log_start();

    /* alloc and load text buffer */
    ret = m_mem->malloc(m_btext.size, 0, &m_text, "text");
    if (AIPU_STATUS_SUCCESS != ret)
    {
        goto finish;
    }
    assert(m_mem->write(m_text.pa, m_btext.va, m_btext.size) == (int)m_btext.size);

    /* alloc and load weight buffer */
    if (m_bweight.size != 0)
    {
        ret = m_mem->malloc(m_bweight.size, 0, &m_weight, "weight");
        if (AIPU_STATUS_SUCCESS != ret)
        {
            goto finish;
        }
        assert(m_mem->write(m_weight.pa, m_bweight.va, m_bweight.size) == (int)m_bweight.size);
    }

finish:
    return ret;
}

aipu_status_t aipudrv::Graph::unload()
{
    aipu_status_t ret = AIPU_STATUS_SUCCESS;

    ret = destroy_jobs();
    if (ret != AIPU_STATUS_SUCCESS)
    {
        return ret;
    }

    if (m_text.size != 0)
    {
        m_mem->free(&m_text);
        m_text.reset();
    }

    if (m_weight.size != 0)
    {
        m_mem->free(&m_weight);
        m_weight.reset();
    }

    m_mem->dump_tracking_log_end();

    return ret;
}
