// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

#include <linux/slab.h>
#include <linux/of_address.h>
#include "aipu_priv.h"
#include "config.h"

static int init_misc_dev(struct aipu_priv *aipu)
{
	aipu->misc.minor = MISC_DYNAMIC_MINOR;
	aipu->misc.name = "aipu";
	aipu->misc.fops = aipu->aipu_fops;
	aipu->misc.mode = 0666;
	return misc_register(&aipu->misc);
}

static void deinit_misc_dev(struct aipu_priv *aipu)
{
	if (aipu && aipu->misc.fops) {
		misc_deregister(&aipu->misc);
		memset(&aipu->misc, 0, sizeof(aipu->misc));
	}
}

/**
 * @init_aipu_priv() - initialize an input AIPU private data struct
 * @aipu:  pointer to the aipu private struct to be initialized
 * @p_dev: pointer to the platform device struct
 * @fops:  pointer to the file_operations struct
 * @soc:   pointer to the SoC private data structure
 * @soc_ops: pointer to the SoC operations struct
 *
 * This function should be called while driver probing. It should be called
 * only one time.
 *
 * Return: 0 on success and error code otherwise.
 */
int init_aipu_priv(struct aipu_priv *aipu, struct platform_device *p_dev,
		   const struct file_operations *fops, struct aipu_soc *soc,
		   struct aipu_soc_operations *soc_ops)
{
	int ret = 0;
	int version = 0;
	int config = 0;

	if (!aipu || !p_dev || !fops)
		return -EINVAL;

	if (aipu->is_init)
		return 0;

	aipu->core_cnt = 0;
	aipu->cores = NULL;
	aipu->dev = &p_dev->dev;
	aipu->aipu_fops = fops;
	aipu->soc = soc;
	aipu->soc_ops = soc_ops;

	zhouyi_detect_aipu_version(p_dev, &version, &config);
	dev_dbg(aipu->dev, "AIPU core0 ISA version %d, configuration %d\n", version, config);

	ret = init_misc_dev(aipu);
	if (ret)
		goto err_handle;

	ret = init_aipu_job_manager(&aipu->job_manager);
	if (ret)
		goto err_handle;

	ret = aipu_init_mm(&aipu->mm, p_dev, version);
	if (ret)
		goto err_handle;

	aipu->is_init = true;
	goto finish;

err_handle:
	deinit_aipu_priv(aipu);

finish:
	return ret;
}

/**
 * @brief deinit an AIPU private data struct
 * @aipu: pointer to the aipu private struct initialized in init_aipu_priv()
 *
 * Return: 0 on success and error code otherwise.
 */
int deinit_aipu_priv(struct aipu_priv *aipu)
{
	int core_iter = 0;

	if (!aipu)
		return 0;

	for (core_iter = 0; core_iter < aipu->core_cnt; core_iter++)
		deinit_aipu_core(aipu->cores[core_iter]);

	kfree(aipu->cores);
	aipu->core_cnt = 0;

	aipu_deinit_mm(&aipu->mm);
	deinit_aipu_job_manager(&aipu->job_manager);
	deinit_misc_dev(aipu);
	aipu->is_init = 0;

	return 0;
}

/**
 * @aipu_priv_add_core() - add new detected core into aipu_priv struct in probe phase
 * @aipu: pointer to the aipu private struct initialized in init_aipu_priv()
 * @core:    pointer to an aipu core struct
 * @version: aipu core hardware version number
 * @id:      aipu core ID
 * @p_dev:   pointer to the platform device struct
 *
 * This function is called when there is a new AIPU core is probed into driver.
 *
 * Return: 0 on success and error code otherwise.
 */
int aipu_priv_add_core(struct aipu_priv *aipu, struct aipu_core *core,
		       int version, int id, struct platform_device *p_dev)
{
	int ret = 0;
	struct aipu_core **new_core_arr = NULL;

	if (!aipu || !core || !p_dev)
		return -EINVAL;

	WARN_ON(!aipu->is_init);

	ret = init_aipu_core(core, version, id, aipu, p_dev);
	if (ret)
		return ret;

	new_core_arr = kcalloc(aipu->core_cnt + 1, sizeof(*new_core_arr), GFP_KERNEL);
	if (!new_core_arr)
		return -ENOMEM;

	if (aipu->core_cnt) {
		WARN_ON(!aipu->cores);
		memcpy(new_core_arr, aipu->cores, aipu->core_cnt * sizeof(*new_core_arr));
		kfree(aipu->cores);
		aipu->cores = NULL;
	}

	new_core_arr[aipu->core_cnt] = core;
	aipu->cores = new_core_arr;
	aipu->core_cnt++;

	aipu_job_manager_set_cores_info(&aipu->job_manager, aipu->core_cnt, aipu->cores);
	return ret;
}

/**
 * @aipu_priv_get_version() - get AIPU hardware version number wrapper
 * @aipu: pointer to the aipu private struct initialized in init_aipu_priv()
 *
 * Return: AIPU ISA version
 */
int aipu_priv_get_version(struct aipu_priv *aipu)
{
	if (likely(aipu))
		return aipu->version;
	return 0;
}

/**
 * @aipu_priv_get_core_cnt() - get AIPU core count
 * @aipu: pointer to the aipu private struct initialized in init_aipu_priv()
 *
 * Return AIPU core count
 */
int aipu_priv_get_core_cnt(struct aipu_priv *aipu)
{
	if (likely(aipu))
		return aipu->core_cnt;
	return 0;
}

/**
 * @aipu_priv_query_core_capability() - query AIPU capability wrapper (per core capability)
 * @aipu: pointer to the aipu private struct initialized in init_aipu_priv()
 * @cap:  pointer to the capability struct
 *
 * Return: 0 on success and error code otherwise.
 */
int aipu_priv_query_core_capability(struct aipu_priv *aipu, struct aipu_core_cap *cap)
{
	int id = 0;
	struct aipu_core *core = NULL;

	if (unlikely(!aipu && !cap))
		return -EINVAL;

	for (id = 0; id < aipu->core_cnt; id++) {
		core = aipu->cores[id];
		cap[id].core_id = id;
		cap[id].arch = core->arch;
		cap[id].version = core->version;
		cap[id].config = core->config;
		cap[id].info.reg_base = core->reg.phys;
	}

	return 0;
}

/**
 * @aipu_priv_query_capability() - query AIPU capability wrapper (multicore common capability)
 * @aipu: pointer to the aipu private struct initialized in init_aipu_priv()
 * @cap:  pointer to the capability struct
 *
 * Return: 0 on success and error code otherwise.
 */
int aipu_priv_query_capability(struct aipu_priv *aipu, struct aipu_cap *cap)
{
	int id = 0;
	struct aipu_core_cap *core_cap = NULL;

	if (unlikely(!aipu && !cap))
		return -EINVAL;

	cap->core_cnt = aipu->core_cnt;
	cap->is_homogeneous = 1;

	core_cap = kcalloc(aipu->core_cnt, sizeof(*core_cap), GFP_KERNEL);
	if (!core_cap)
		return -ENOMEM;

	aipu_priv_query_core_capability(aipu, core_cap);
	for (id = 1; id < aipu->core_cnt; id++) {
		if (core_cap[id].arch != core_cap[id - 1].arch ||
		    core_cap[id].version != core_cap[id - 1].version ||
		    core_cap[id].config != core_cap[id - 1].config) {
			cap->is_homogeneous = 0;
			break;
		}
	}

	if (cap->is_homogeneous)
		cap->core_cap = core_cap[0];

	kfree(core_cap);
	return 0;
}

/**
 * @aipu_priv_io_rw() - AIPU external register read/write wrapper
 * @aipu:   pointer to the aipu private struct initialized in init_aipu_priv()
 * @io_req: pointer to the io_req struct
 *
 * Return: 0 on success and error code otherwise.
 */
int aipu_priv_io_rw(struct aipu_priv *aipu, struct aipu_io_req *io_req)
{
	int ret = -EINVAL;
	int id = 0;

	if (!aipu || !io_req || io_req->core_id >= aipu->core_cnt)
		return ret;

	id = io_req->core_id;
	return aipu->cores[id]->ops->io_rw(aipu->cores[id], io_req);
}

/**
 * @aipu_priv_check_status() - check if aipu status is ready for usage
 * @aipu: pointer to the aipu private struct initialized in init_aipu_priv()
 *
 * Return: 0 on success and error code otherwise.
 */
int aipu_priv_check_status(struct aipu_priv *aipu)
{
	if (aipu && aipu->is_init)
		return 0;
	return -EINVAL;
}
