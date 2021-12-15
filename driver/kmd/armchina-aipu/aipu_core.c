// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

#include <linux/slab.h>
#include <linux/string.h>
#include "aipu_priv.h"
#include "config.h"
#include "z1.h"
#include "z2.h"

#define MAX_CHAR_SYSFS 4096

inline struct aipu_soc *get_soc(struct aipu_core *core)
{
	if (core && core->priv)
		return core->priv->soc;
	return NULL;
}

inline struct aipu_soc_operations *get_soc_ops(struct aipu_core *core)
{
	if (core && core->priv)
		return core->priv->soc_ops;
	return NULL;
}

inline struct aipu_job_manager *get_job_manager(struct aipu_core *core)
{
	if (core && core->priv)
		return &core->priv->job_manager;
	return NULL;
}

#ifdef CONFIG_SYSFS
static ssize_t aipu_core_ext_register_sysfs_show(struct device *dev,
						 struct device_attribute *attr,
						 char *buf)
{
	int ret = 0;
	char tmp[512];
	struct platform_device *p_dev = container_of(dev, struct platform_device, dev);
	struct aipu_core *core = platform_get_drvdata(p_dev);

	if (unlikely(!core))
		return 0;

	if (get_soc_ops(core) &&
	    get_soc_ops(core)->is_clk_enabled &&
	    !get_soc_ops(core)->is_clk_enabled(dev, get_soc(core))) {
		return snprintf(buf, MAX_CHAR_SYSFS,
		    "AIPU is suspended and external registers cannot be read!\n");
	}

	ret += snprintf(tmp, 1024, "----------------------------------------\n");
	strcat(buf, tmp);
	ret += snprintf(tmp, 1024, "   AIPU Core%d External Register Values\n", core->id);
	strcat(buf, tmp);
	ret += snprintf(tmp, 1024, "----------------------------------------\n");
	strcat(buf, tmp);
	ret += snprintf(tmp, 1024, "%-*s%-*s%-*s\n", 8, "Offset", 22, "Name", 10, "Value");
	strcat(buf, tmp);
	ret += snprintf(tmp, 1024, "----------------------------------------\n");
	strcat(buf, tmp);
	ret += core->ops->sysfs_show(core, buf);
	ret += snprintf(tmp, 1024, "----------------------------------------\n");
	strcat(buf, tmp);

	return ret;
}

static ssize_t aipu_core_ext_register_sysfs_store(struct device *dev,
						  struct device_attribute *attr,
						  const char *buf, size_t count)
{
	int i = 0;
	int ret = 0;
	char *token = NULL;
	char *buf_dup = NULL;
	int value[3] = { 0 };
	struct aipu_io_req io_req;
	struct platform_device *p_dev = container_of(dev, struct platform_device, dev);
	struct aipu_core *core = platform_get_drvdata(p_dev);

	if (get_soc_ops(core) &&
	    get_soc_ops(core)->is_clk_enabled &&
	    !get_soc_ops(core)->is_clk_enabled(dev, get_soc(core)))
		return 0;

	buf_dup = kzalloc(1024, GFP_KERNEL);
	if (!buf_dup)
		return -ENOMEM;
	snprintf(buf_dup, 1024, buf);

	for (i = 0; i < 3; i++) {
		token = strsep(&buf_dup, "-");
		if (!token) {
			dev_err(dev, "[SYSFS] please echo as this format: <reg_offset>-<write time>-<write value>");
			goto out_free_buffer;
		}

		dev_dbg(dev, "[SYSFS] to convert str: %s", token);

		ret = kstrtouint(token, 0, &value[i]);
		if (ret) {
			dev_err(dev, "[SYSFS] convert str to int failed (%d): %s", ret, token);
			goto out_free_buffer;
		}
	}

	dev_dbg(dev, "[SYSFS] offset 0x%x, time 0x%x, value 0x%x",
		value[0], value[1], value[2]);

	io_req.rw = AIPU_IO_WRITE;
	io_req.offset = value[0];
	io_req.value = value[2];
	for (i = 0; i < value[1]; i++) {
		dev_dbg(dev, "[SYSFS] writing register 0x%x with value 0x%x", value[0], value[2]);
		core->ops->io_rw(core, &io_req);
	}

out_free_buffer:
	kfree(buf_dup);
	return count;
}

static ssize_t aipu_core_clock_sysfs_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	struct platform_device *p_dev = container_of(dev, struct platform_device, dev);
	struct aipu_core *core = platform_get_drvdata(p_dev);

	/*
	 * If SoC level provides no clock operations,
	 * the state of AIPU is by default treated as normal.
	 */
	if (get_soc_ops(core) &&
	    get_soc_ops(core)->is_clk_enabled &&
	    !get_soc_ops(core)->is_clk_enabled(dev, get_soc(core)))
		return snprintf(buf, MAX_CHAR_SYSFS,
				"AIPU is in clock gating state and suspended.\n");
	else
		return snprintf(buf, MAX_CHAR_SYSFS, "AIPU is in normal working state.\n");
}

static ssize_t aipu_core_clock_sysfs_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t count)
{
	int do_suspend = 0;
	int do_resume = 0;
	struct platform_device *p_dev = container_of(dev, struct platform_device, dev);
	struct aipu_core *core = platform_get_drvdata(p_dev);

	if (unlikely(!core))
		return count;

	if (!get_soc_ops(core) ||
	    !get_soc_ops(core)->enable_clk ||
	    !get_soc_ops(core)->disable_clk ||
	    !get_soc_ops(core)->is_clk_enabled) {
		dev_info(dev, "operation is not supported.\n");
		return count;
	}

	if ((strncmp(buf, "1", 1) == 0))
		do_suspend = 1;
	else if ((strncmp(buf, "0", 1) == 0))
		do_resume = 1;

	if (get_soc_ops(core)->is_clk_enabled(dev, get_soc(core)) &&
	    core->ops->is_idle(core) && do_suspend) {
		dev_info(dev, "disable clock\n");
		get_soc_ops(core)->disable_clk(core->dev, get_soc(core));
	} else if (!get_soc_ops(core)->is_clk_enabled(dev, get_soc(core)) && do_resume) {
		dev_info(dev, "enable clock\n");
		get_soc_ops(core)->enable_clk(core->dev, get_soc(core));
	} else {
		dev_err(dev, "operation cannot be completed!\n");
	}

	return count;
}

static ssize_t aipu_core_disable_sysfs_show(struct device *dev, struct device_attribute *attr,
					    char *buf)
{
	struct platform_device *p_dev = container_of(dev, struct platform_device, dev);
	struct aipu_core *core = platform_get_drvdata(p_dev);

	if (atomic_read(&core->disable)) {
		return snprintf(buf, MAX_CHAR_SYSFS,
		    "AIPU core #%d is disabled (echo 0 > /sys/devices/platform/[dev]/disable to enable it).\n",
		    core->id);
	} else {
		return snprintf(buf, MAX_CHAR_SYSFS,
		    "AIPU core #%d is enabled (echo 1 > /sys/devices/platform/[dev]/disable to disable it).\n",
		    core->id);
	}
}

static ssize_t aipu_core_disable_sysfs_store(struct device *dev, struct device_attribute *attr,
					     const char *buf, size_t count)
{
	int do_disable = 0;
	struct platform_device *p_dev = container_of(dev, struct platform_device, dev);
	struct aipu_core *core = platform_get_drvdata(p_dev);

	if ((strncmp(buf, "1", 1) == 0))
		do_disable = 1;
	else if ((strncmp(buf, "0", 1) == 0))
		do_disable = 0;
	else
		do_disable = -1;

	if (atomic_read(&core->disable) && !do_disable) {
		dev_info(dev, "enable core...\n");
		atomic_set(&core->disable, 0);
	} else if (!atomic_read(&core->disable) && do_disable) {
		dev_info(dev, "disable core...\n");
		atomic_set(&core->disable, 1);
	}

	return count;
}

typedef ssize_t (*sysfs_show_t)(struct device *dev, struct device_attribute *attr, char *buf);
typedef ssize_t (*sysfs_store_t)(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count);

static struct device_attribute *aipu_core_create_attr(struct device *dev,
						      struct device_attribute **attr,
						      const char *name, int mode,
						      sysfs_show_t show, sysfs_store_t store)
{
	if (!dev || !attr || !name)
		return ERR_PTR(-EINVAL);

	*attr = kzalloc(sizeof(*attr), GFP_KERNEL);
	if (!*attr)
		return ERR_PTR(-ENOMEM);

	(*attr)->attr.name = name;
	(*attr)->attr.mode = mode;
	(*attr)->show = show;
	(*attr)->store = store;
	device_create_file(dev, *attr);

	return *attr;
}

static void aipu_core_destroy_attr(struct device *dev, struct device_attribute **attr)
{
	if (!dev || !attr || !*attr)
		return;

	device_remove_file(dev, *attr);
	kfree(*attr);
	*attr = NULL;
}
#endif

/**
 * @init_aipu_core() - init an AIPU core struct in driver probe phase
 * @core:     AIPU hardware core created in a calling function
 * @version:  AIPU core hardware version
 * @id:       AIPU core ID
 * @priv:     pointer to aipu_private struct
 * @p_dev:    platform device struct pointer
 *
 * Return: 0 on success and error code otherwise.
 */
int init_aipu_core(struct aipu_core *core, int version, int id, struct aipu_priv *priv,
		   struct platform_device *p_dev)
{
	int ret = 0;
	struct resource *res = NULL;
	u64 base = 0;
	u64 size = 0;

	if (!core || !p_dev || !priv)
		return -EINVAL;

	WARN_ON(core->is_init);
	WARN_ON(version != AIPU_ISA_VERSION_ZHOUYI_V1 &&
		version != AIPU_ISA_VERSION_ZHOUYI_V2 &&
		version != AIPU_ISA_VERSION_ZHOUYI_V3);

	core->version = version;
	core->id = id;
	core->dev = &p_dev->dev;
	core->priv = priv;
	atomic_set(&core->disable, 0);
	snprintf(core->core_name, sizeof(core->core_name), "aipu%d", id);

	if (version == AIPU_ISA_VERSION_ZHOUYI_V1) {
		core->max_sched_num = ZHOUYI_V1_MAX_SCHED_JOB_NUM;
		core->ops = get_zhouyi_v1_ops();
	} else if (version == AIPU_ISA_VERSION_ZHOUYI_V2 ||
		   version == AIPU_ISA_VERSION_ZHOUYI_V3) {
		core->max_sched_num = ZHOUYI_V2_MAX_SCHED_JOB_NUM;
		core->ops = get_zhouyi_v2_ops();
	}

	res = platform_get_resource(p_dev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(core->dev, "get aipu core #%d IO region failed\n", id);
		ret = -EINVAL;
		goto init_reg_fail;
	}
	base = res->start;
	size = res->end - res->start + 1;
	dev_dbg(core->dev, "get aipu core #%d IO region: [0x%llx, 0x%llx]\n",
		id, base, res->end);

	ret = init_aipu_ioregion(&core->reg, base, size);
	if (ret) {
		dev_err(core->dev,
			"create aipu core #%d IO region failed: base 0x%llx, size 0x%llx\n",
			id, base, size);
		goto init_reg_fail;
	}
	dev_dbg(core->dev, "init aipu core #%d IO region done: [0x%llx, 0x%llx]\n",
		id, base, res->end);

	res = platform_get_resource(p_dev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(core->dev, "get aipu core #%d IRQ number failed\n", id);
		ret = -EINVAL;
		goto init_irq_fail;
	}
	dev_dbg(core->dev, "get aipu core #%d IRQ number: 0x%x\n", id, (int)res->start);

	core->irq_obj = aipu_create_irq_object(res->start, core, core->core_name);
	if (!core->irq_obj) {
		dev_err(core->dev, "create IRQ object for core #%d failed: IRQ 0x%x\n",
			id, (int)res->start);
		ret = -EFAULT;
		goto init_irq_fail;
	}
	dev_dbg(core->dev, "init aipu core #%d IRQ done\n", id);

#ifdef CONFIG_SYSFS
	if (IS_ERR(aipu_core_create_attr(core->dev, &core->reg_attr, "ext_registers", 0644,
					 aipu_core_ext_register_sysfs_show,
					 aipu_core_ext_register_sysfs_store))) {
		ret = -EFAULT;
		goto init_sysfs_fail;
	}

	if (priv->soc_ops &&
	    priv->soc_ops->enable_clk && priv->soc_ops->disable_clk &&
	    IS_ERR(aipu_core_create_attr(core->dev, &core->clk_attr, "soc_clock", 0644,
					 aipu_core_clock_sysfs_show,
					 aipu_core_clock_sysfs_store))) {
		ret = -EFAULT;
		goto init_sysfs_fail;
	}

	if (IS_ERR(aipu_core_create_attr(core->dev, &core->disable_attr, "disable", 0644,
					 aipu_core_disable_sysfs_show,
					 aipu_core_disable_sysfs_store))) {
		ret = -EFAULT;
		goto init_sysfs_fail;
	}
#else
	core->reg_attr = NULL;
	core->clk_attr = NULL;
	core->disable_attr = NULL;
#endif

	core->arch = AIPU_ARCH_ZHOUYI;
	core->config = core->ops->get_config(core);
	core->ops->enable_interrupt(core);
	core->ops->print_hw_id_info(core);

	core->is_init = 1;
	goto finish;

#ifdef CONFIG_SYSFS
init_sysfs_fail:
	aipu_core_destroy_attr(core->dev, &core->reg_attr);
	aipu_core_destroy_attr(core->dev, &core->clk_attr);
	aipu_core_destroy_attr(core->dev, &core->disable_attr);
#endif
	aipu_destroy_irq_object(core->irq_obj);
init_irq_fail:
init_reg_fail:
	deinit_aipu_ioregion(&core->reg);

finish:
	return ret;
}

/**
 * @deinit_aipu_core() - deinit a created aipu_core struct
 * @core: pointer to struct aipu_core initialized in init_aipu_core()
 */
void deinit_aipu_core(struct aipu_core *core)
{
	if (!core)
		return;

	core->ops->disable_interrupt(core);
	deinit_aipu_ioregion(&core->reg);

	if (core->irq_obj) {
		aipu_destroy_irq_object(core->irq_obj);
		core->irq_obj = NULL;
	}

#ifdef CONFIG_SYSFS
	aipu_core_destroy_attr(core->dev, &core->reg_attr);
	aipu_core_destroy_attr(core->dev, &core->clk_attr);
	aipu_core_destroy_attr(core->dev, &core->disable_attr);
#endif
	core->is_init = 0;
}
