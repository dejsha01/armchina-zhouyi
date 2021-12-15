/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

#ifndef __AIPU_CORE_H__
#define __AIPU_CORE_H__

#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/atomic.h>
#include <uapi/misc/armchina_aipu.h>
#include "aipu_irq.h"
#include "aipu_io.h"
#include "zhouyi/zhouyi.h"
#include "config.h"

struct aipu_core;
struct aipu_priv;

/**
 * struct aipu_core_operations - a struct contains AIPU hardware operation methods
 * @get_version:        get hardware version number
 * @get_config:         get hardware configuration number
 * @enable_interrupt:   enable all AIPU interrupts
 * @disable_interrupt:  disable all AIPU interrupts
 * @trigger:            trigger a deferred-job to run on a reserved core
 * @reserve:            reserve AIPU core for a job/deferred-job
 * @is_idle:            is AIPU hardware idle or not
 * @read_status_reg:    read status register value
 * @print_hw_id_info:   print AIPU version ID registers information
 * @io_rw:              direct IO read/write operations
 * @upper_half:         interrupt upper half handler
 * @bottom_half:        interrupt bottom half handler
 * @sysfs_show:         show core external register values
 */
struct aipu_core_operations {
	int (*get_version)(struct aipu_core *core);
	int (*get_config)(struct aipu_core *core);
	void (*enable_interrupt)(struct aipu_core *core);
	void (*disable_interrupt)(struct aipu_core *core);
	void (*trigger)(struct aipu_core *core);
	int (*reserve)(struct aipu_core *core, struct aipu_job_desc *udesc,
		       int do_trigger);
	bool (*is_idle)(struct aipu_core *core);
	int (*read_status_reg)(struct aipu_core *core);
	void (*print_hw_id_info)(struct aipu_core *core);
	int (*io_rw)(struct aipu_core *core, struct aipu_io_req *io_req);
	int (*upper_half)(void *data);
	void (*bottom_half)(void *data);
#ifdef CONFIG_SYSFS
	int (*sysfs_show)(struct aipu_core *core, char *buf);
#endif
};

/**
 * struct aipu_core - a general struct describe a hardware AIPU core
 * @id:              AIPU core ID
 * @arch:            AIPU architecture number
 * @version:         AIPU hardware version number
 * @config:          AIPU hardware configuration number
 * @core_name:       AIPU core name string
 * @max_sched_num:   maximum number of jobs can be scheduled in pipeline
 * @dev:             device struct pointer
 * @reg:             IO region array of this AIPU core
 * @ops:             operations of this core
 * @irq_obj:         interrupt object of this core
 * @priv:            pointer to aipu private struct
 * @reg_attr:        external register attribute
 * @clk_attr:        clock attribute
 * @disable_attr:    disable core attribute
 * @disable:         core disable flag (for debug usage)
 * @is_init:         init flag
 */
struct aipu_core {
	int id;
	int arch;
	int version;
	int config;
	char core_name[10];
	int max_sched_num;
	struct device *dev;
	struct io_region reg;
	struct aipu_core_operations *ops;
	struct aipu_irq_object *irq_obj;
	struct aipu_priv *priv;
	struct device_attribute *reg_attr;
	struct device_attribute *clk_attr;
	struct device_attribute *disable_attr;
	atomic_t disable;
	int is_init;
};

int init_aipu_core(struct aipu_core *core, int version, int id, struct aipu_priv *priv,
		   struct platform_device *p_dev);
void deinit_aipu_core(struct aipu_core *core);
struct aipu_soc *get_soc(struct aipu_core *core);
struct aipu_soc_operations *get_soc_ops(struct aipu_core *core);
struct aipu_job_manager *get_job_manager(struct aipu_core *core);

#endif /* __AIPU_CORE_H__ */
