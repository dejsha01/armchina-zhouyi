// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

#include <linux/irqreturn.h>
#include <linux/bitops.h>
#include "aipu_priv.h"
#include "z1.h"
#include "aipu_io.h"
#include "config.h"

static int zhouyi_v1_get_hw_version_number(struct aipu_core *core)
{
	if (likely(core))
		return zhouyi_get_hw_version_number(&core->reg);
	return 0;
}

static int zhouyi_v1_get_hw_config_number(struct aipu_core *core)
{
	if (likely(core))
		return zhouyi_get_hw_config_number(&core->reg);
	return 0;
}

static void zhouyi_v1_enable_interrupt(struct aipu_core *core)
{
	if (likely(core))
		aipu_write32(&core->reg, ZHOUYI_CTRL_REG_OFFSET,
			     ZHOUYIV1_IRQ_ENABLE_FLAG);
}

static void zhouyi_v1_disable_interrupt(struct aipu_core *core)
{
	if (likely(core))
		aipu_write32(&core->reg, ZHOUYI_CTRL_REG_OFFSET,
			     ZHOUYIV1_IRQ_DISABLE_FLAG);
}

static void zhouyi_v1_clear_qempty_interrupt(struct aipu_core *core)
{
	if (likely(core))
		zhouyi_clear_qempty_interrupt(&core->reg);
}

static void zhouyi_v1_clear_done_interrupt(struct aipu_core *core)
{
	if (likely(core))
		zhouyi_clear_done_interrupt(&core->reg);
}

static void zhouyi_v1_clear_excep_interrupt(struct aipu_core *core)
{
	if (likely(core))
		zhouyi_clear_excep_interrupt(&core->reg);
}

static void zhouyi_v1_trigger(struct aipu_core *core)
{
	int start_pc = 0;

	if (likely(core)) {
		start_pc = aipu_read32(&core->reg, ZHOUYI_START_PC_REG_OFFSET) & 0xFFFFFFF0;
		aipu_write32(&core->reg, ZHOUYI_START_PC_REG_OFFSET, start_pc | 0xD);
	}
}

static int zhouyi_v1_reserve(struct aipu_core *core, struct aipu_job_desc *udesc, int do_trigger)
{
	unsigned int phys_addr = 0;
	unsigned int phys_addr0 = 0;
	unsigned int phys_addr1 = 0;
	unsigned int start_pc = 0;

	if (unlikely(!core || !udesc))
		return -EINVAL;

	/* Load data addr 0 register */
	phys_addr0 = (unsigned int)udesc->data_0_addr;
	aipu_write32(&core->reg, ZHOUYI_DATA_ADDR_0_REG_OFFSET, phys_addr0);

	/* Load data addr 1 register */
	phys_addr1 = (unsigned int)udesc->data_1_addr;
	aipu_write32(&core->reg, ZHOUYI_DATA_ADDR_1_REG_OFFSET, phys_addr1);

	/* Load interrupt PC */
	aipu_write32(&core->reg, ZHOUYI_INTR_PC_REG_OFFSET,
		     (unsigned int)udesc->intr_handler_addr);

	/* Load start PC register */
	phys_addr = (unsigned int)udesc->start_pc_addr;
	if (do_trigger)
		start_pc = phys_addr | 0xD;
	else
		start_pc = phys_addr;
	aipu_write32(&core->reg, ZHOUYI_START_PC_REG_OFFSET, start_pc);

	dev_dbg(core->dev, "[Job %d] trigger done: start pc = 0x%x, dreg0 = 0x%x, dreg1 = 0x%x\n",
		udesc->job_id, start_pc, phys_addr0, phys_addr1);

	return 0;
}

static bool zhouyi_v1_is_idle(struct aipu_core *core)
{
	unsigned long val = 0;

	if (unlikely(!core))
		return false;

	val = aipu_read32(&core->reg, ZHOUYI_STAT_REG_OFFSET);
	return test_bit(16, &val) && test_bit(17, &val) && test_bit(18, &val);
}

static int zhouyi_v1_read_status_reg(struct aipu_core *core)
{
	return zhouyi_read_status_reg(&core->reg);
}

static void zhouyi_v1_print_hw_id_info(struct aipu_core *core)
{
	if (unlikely(!core))
		return;

	dev_info(core->dev, "AIPU Initial Status: 0x%x",
		 aipu_read32(&core->reg, ZHOUYI_STAT_REG_OFFSET));

	dev_info(core->dev, "########## AIPU CORE %d: ZHOUYI V1 ##########", core->id);
	dev_info(core->dev, "# ISA Version Register: 0x%x",
		 aipu_read32(&core->reg, ZHOUYI_ISA_VERSION_REG_OFFSET));
	dev_info(core->dev, "# TPC Feature Register: 0x%x",
		 aipu_read32(&core->reg, ZHOUYI_TPC_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# SPU Feature Register: 0x%x",
		 aipu_read32(&core->reg, ZHOUYI_SPU_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# HWA Feature Register: 0x%x",
		 aipu_read32(&core->reg, ZHOUYI_HWA_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# Revision ID Register: 0x%x",
		 aipu_read32(&core->reg, ZHOUYI_REVISION_ID_REG_OFFSET));
	dev_info(core->dev, "# Memory Hierarchy Feature Register: 0x%x",
		 aipu_read32(&core->reg, ZHOUYI_MEM_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# Instruction RAM Feature Register:  0x%x",
		 aipu_read32(&core->reg, ZHOUYI_INST_RAM_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# TEC Local SRAM Feature Register:   0x%x",
		 aipu_read32(&core->reg, ZHOUYI_LOCAL_SRAM_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# Global SRAM Feature Register:      0x%x",
		 aipu_read32(&core->reg, ZHOUYI_GLOBAL_SRAM_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# Instruction Cache Feature Register:0x%x",
		 aipu_read32(&core->reg, ZHOUYI_INST_CACHE_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# Data Cache Feature Register:       0x%x",
		 aipu_read32(&core->reg, ZHOUYI_DATA_CACHE_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# L2 Cache Feature Register:         0x%x",
		 aipu_read32(&core->reg, ZHOUYI_L2_CACHE_FEATURE_REG_OFFSET));
	dev_info(core->dev, "############################################");
}

static int zhouyi_v1_io_rw(struct aipu_core *core, struct aipu_io_req *io_req)
{
	if (unlikely(!io_req))
		return -EINVAL;

	if (!core || io_req->offset > ZHOUYI_V1_MAX_REG_OFFSET)
		return -EINVAL;

	zhouyi_io_rw(&core->reg, io_req);
	return 0;
}

static int zhouyi_v1_upper_half(void *data)
{
	int ret = 0;
	struct aipu_core *core = (struct aipu_core *)data;

	if (get_soc_ops(core) &&
	    get_soc_ops(core)->is_aipu_irq &&
	    !get_soc_ops(core)->is_aipu_irq(core->dev, get_soc(core), core->id))
		return IRQ_NONE;

	ret = zhouyi_v1_read_status_reg(core);
	if (ret & ZHOUYI_IRQ_QEMPTY)
		zhouyi_v1_clear_qempty_interrupt(core);

	if (ret & ZHOUYI_IRQ_DONE) {
		zhouyi_v1_clear_done_interrupt(core);
		aipu_job_manager_irq_upper_half(core, 0);
		aipu_irq_schedulework(core->irq_obj);
	}

	if (ret & ZHOUYI_IRQ_EXCEP) {
		zhouyi_v1_clear_excep_interrupt(core);
		aipu_job_manager_irq_upper_half(core,
						aipu_read32(&core->reg,
							    ZHOUYI_INTR_CAUSE_REG_OFFSET));
		aipu_irq_schedulework(core->irq_obj);
	}

	return IRQ_HANDLED;
}

static void zhouyi_v1_bottom_half(void *data)
{
	aipu_job_manager_irq_bottom_half(data);
}

#ifdef CONFIG_SYSFS
static int zhouyi_v1_sysfs_show(struct aipu_core *core, char *buf)
{
	int ret = 0;
	char tmp[512];

	if (unlikely(!core || !buf))
		return -EINVAL;

	ret += zhouyi_sysfs_show(&core->reg, buf);
	ret += zhouyi_print_reg_info(&core->reg, tmp, "Intr Cause Reg",
				     ZHOUYI_INTR_CAUSE_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(&core->reg, tmp, "Intr Status Reg",
	    ZHOUYI_INTR_STAT_REG_OFFSET);
	strcat(buf, tmp);
	return ret;
}
#endif

static struct aipu_core_operations zhouyi_v1_ops = {
	.get_version = zhouyi_v1_get_hw_version_number,
	.get_config = zhouyi_v1_get_hw_config_number,
	.enable_interrupt = zhouyi_v1_enable_interrupt,
	.disable_interrupt = zhouyi_v1_disable_interrupt,
	.trigger = zhouyi_v1_trigger,
	.reserve = zhouyi_v1_reserve,
	.is_idle = zhouyi_v1_is_idle,
	.read_status_reg = zhouyi_v1_read_status_reg,
	.print_hw_id_info = zhouyi_v1_print_hw_id_info,
	.io_rw = zhouyi_v1_io_rw,
	.upper_half = zhouyi_v1_upper_half,
	.bottom_half = zhouyi_v1_bottom_half,
#ifdef CONFIG_SYSFS
	.sysfs_show = zhouyi_v1_sysfs_show,
#endif
};

struct aipu_core_operations *get_zhouyi_v1_ops(void)
{
	return &zhouyi_v1_ops;
}
