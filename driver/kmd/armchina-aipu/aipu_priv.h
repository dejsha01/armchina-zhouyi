/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

#ifndef __AIPU_PRIV_H__
#define __AIPU_PRIV_H__

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include "include/armchina_aipu_soc.h"
#include "aipu_irq.h"
#include "aipu_io.h"
#include "aipu_core.h"
#include "aipu_job_manager.h"
#include "aipu_mm.h"

/**
 * struct aipu_priv - AIPU private struct contains all core info and shared resources
 * @version:     AIPU hardware version
 * @core_cnt:    AIPU core count in system
 * @cores:       core pointer array
 * @dev:         device struct pointer of core 0
 * @soc_ops:     SoC operation pointer
 * @aipu_fops:   file operation struct
 * @misc:        misc driver struct
 * @job_manager: job manager struct
 * @mm:          memory manager
 * @is_init:     init flag
 */
struct aipu_priv {
	int version;
	int core_cnt;
	struct aipu_core **cores;
	struct device *dev;
	struct aipu_soc              *soc;
	struct aipu_soc_operations   *soc_ops;
	const struct file_operations *aipu_fops;
	struct miscdevice            misc;
	struct aipu_job_manager      job_manager;
	struct aipu_memory_manager   mm;
	bool is_init;
};

int init_aipu_priv(struct aipu_priv *aipu, struct platform_device *p_dev,
		   const struct file_operations *fops, struct aipu_soc *soc,
		   struct aipu_soc_operations *soc_ops);
int deinit_aipu_priv(struct aipu_priv *aipu);
int aipu_priv_add_core(struct aipu_priv *aipu, struct aipu_core *core,
		       int version, int id, struct platform_device *p_dev);
int aipu_priv_get_version(struct aipu_priv *aipu);
int aipu_priv_get_core_cnt(struct aipu_priv *aipu);
int aipu_priv_query_core_capability(struct aipu_priv *aipu, struct aipu_core_cap *cap);
int aipu_priv_query_capability(struct aipu_priv *aipu, struct aipu_cap *cap);
int aipu_priv_io_rw(struct aipu_priv *aipu, struct aipu_io_req *io_req);
int aipu_priv_check_status(struct aipu_priv *aipu);

#endif /* __AIPU_PRIV_H__ */
