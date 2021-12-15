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
 * @file  swig_cpp2py_api.hpp
 * @brief AIPU User Mode Driver (UMD) C++ to Python API header
 * @version 1.0
 */

#ifndef _SWIG_CPP2PY_API_HPP_
#define _SWIG_CPP2PY_API_HPP_

#include <string>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <vector>
#include "standard_api.h"

class Graph
{
public:
    /**
     * Internal
     */
    int create_job()
    {
        aipu_status_t ret = AIPU_STATUS_SUCCESS;

        ret = aipu_create_job(m_ctx, m_graph_id, &m_job_id);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(m_ctx, ret, &m_status_msg);
            fprintf(stderr, "[PY UMD ERROR] aipu_create_job: %s\n", m_status_msg);
        }
        return ret;
    }

public:
    /**
     * @brief This API is exposed as a SWIG-Python API.
     *        It is used to get the number of input tensor(s) of a graph.
     */
    int GetInputTensorNumber()
    {
        aipu_status_t ret = AIPU_STATUS_SUCCESS;

        if (m_input_cnt != 0)
        {
            return m_input_cnt;
        }

        ret = aipu_get_tensor_count(m_ctx, m_graph_id, AIPU_TENSOR_TYPE_INPUT, &m_input_cnt);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(m_ctx, ret, &m_status_msg);
            fprintf(stderr, "[PY UMD ERROR] aipu_get_tensor_count: %s\n", m_status_msg);
        }
        return m_input_cnt;
    }

    /**
     * @brief This API is exposed as a SWIG-Python API.
     *        It is used to get the number of output tensor(s) of a graph.
     */
    int GetOutputTensorNumber()
    {
        aipu_status_t ret = AIPU_STATUS_SUCCESS;

        if (m_output_cnt != 0)
        {
            return m_output_cnt;
        }

        ret = aipu_get_tensor_count(m_ctx, m_graph_id, AIPU_TENSOR_TYPE_OUTPUT, &m_output_cnt);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(m_ctx, ret, &m_status_msg);
            fprintf(stderr, "[PY UMD ERROR] aipu_get_tensor_count: %s\n", m_status_msg);
        }
        return m_output_cnt;
    }

#if (defined BUILD_WITH_NUMPY)
    /**
     * @brief This API is exposed as a SWIG-Python API.
     *        It is used to load input tensor(s) via a NumPy object.
     *
     * @param[in] id    An input tensor ID
     * @param[in] data  Pointer to the input tensor data
     * @param[in] bytes Tensor size in byte (C++ API only)
     *
     * @python_api LoadInputTensor(int id, numpy obj)
     */
    int LoadInputTensor(int id, int* data, int bytes)
    {
        aipu_status_t ret = AIPU_STATUS_SUCCESS;

        ret = aipu_load_tensor(m_ctx, m_job_id, id, data);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(m_ctx, ret, &m_status_msg);
            fprintf(stderr, "[PY UMD ERROR] aipu_load_tensor: %s\n", m_status_msg);
        }
        return ret;
    }
#endif

    /**
     * @brief This API is exposed as a SWIG-Python API.
     *        It is used to load input tensor(s) via a binary file name.
     *
     * @param[in] id        An input tensor ID
     * @param[in] filename  Name of the input tensor specified by id which contains binary data
     */
    int LoadInputTensor(int id, const char* filename)
    {
        aipu_status_t ret = AIPU_STATUS_SUCCESS;
        int fd = 0;
        void* data = nullptr;
        aipu_tensor_desc_t desc;

        ret = aipu_get_tensor_descriptor(m_ctx, m_job_id, AIPU_TENSOR_TYPE_INPUT, id, &desc);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(m_ctx, ret, &m_status_msg);
            fprintf(stderr, "[PY UMD ERROR] aipu_get_tensor_descriptor: %s\n", m_status_msg);
            return ret;
        }

        fd = open(filename, O_RDONLY);
        if (fd <= 0)
        {
            fprintf(stderr, "[PY UMD ERROR] open file failed: %s! (errno = %d)\n", filename, errno);
            return errno;
        }

        data = mmap(nullptr, desc.size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (MAP_FAILED == data)
        {
            fprintf(stderr, "[PY UMD ERROR] RT failed in mapping graph file: %s! (errno = %d)\n", filename, errno);
            close(fd);
            return -1;
        }

        ret = aipu_load_tensor(m_ctx, m_job_id, id, data);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(m_ctx, ret, &m_status_msg);
            fprintf(stderr, "[PY UMD ERROR] aipu_load_tensor: %s\n", m_status_msg);
        }

        munmap(data, desc.size);

        if (fd > 0)
        {
            close(fd);
        }
        return ret;
    }

    /**
     * @brief This API is exposed as a a SWIG-Python API.
     *        It is used to run a loaded graph after all input tensors are loaded.
     */
    int Run()
    {
        aipu_status_t ret = AIPU_STATUS_SUCCESS;
        aipu_job_config_dump_t mem_dump_config;
        mem_dump_config.dump_dir = "./";

        ret = aipu_config_job(m_ctx, m_job_id, AIPU_JOB_CONFIG_TYPE_DUMP_OUTPUT, &mem_dump_config);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(m_ctx, ret, &m_status_msg);
            fprintf(stderr, "[PY UMD ERROR] aipu_config_job: %s\n", m_status_msg);
            return ret;
        }

        ret = aipu_finish_job(m_ctx, m_job_id, -1);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(m_ctx, ret, &m_status_msg);
            fprintf(stderr, "[PY UMD ERROR] aipu_finish_job: %s\n", m_status_msg);
            return ret;
        }

        return ret;
    }

    /**
     * @brief This API is exposed as a a SWIG-Python API.
     *        It is used to get output tensor(s) after then graph execution finishes.
     *
     * @param[in] id  A output tensor ID
     */
    std::vector<int> GetOutputTensor(int id)
    {
        aipu_status_t ret = AIPU_STATUS_SUCCESS;
        std::vector<int> output;
        char* out_data;
        aipu_tensor_desc_t desc;

        ret = aipu_get_tensor_descriptor(m_ctx, m_job_id, AIPU_TENSOR_TYPE_OUTPUT, id, &desc);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(m_ctx, ret, &m_status_msg);
            fprintf(stderr, "[PY UMD ERROR] aipu_get_tensor_descriptor: %s\n", m_status_msg);
            return output;
        }

        out_data = new char[desc.size];
        ret = aipu_get_tensor(m_ctx, m_job_id, AIPU_TENSOR_TYPE_OUTPUT, id, out_data);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(m_ctx, ret, &m_status_msg);
            fprintf(stderr, "[PY UMD ERROR] aipu_get_tensor: %s\n", m_status_msg);
            goto finish;
        }

        for (uint32_t i = 0; i < desc.size; i++)
        {
            output.push_back(out_data[i]);
        }

    finish:
        delete[] out_data;
        return output;
    }

public:
    Graph(aipu_ctx_handle_t* ctx, uint64_t graph_id)
    {
        m_ctx = ctx;
        m_graph_id = graph_id;
    }
    Graph(const Graph& graph) = delete;
    Graph& operator=(const Graph& graph) = delete;
    ~Graph()
    {
        if (m_job_id)
        {
            aipu_clean_job(m_ctx, m_job_id);
            m_job_id = 0;
        }
        aipu_unload_graph(m_ctx, m_graph_id);
        m_graph_id = 0;
    }

private:
    aipu_ctx_handle_t* m_ctx;
    const char* m_status_msg;
    uint64_t m_graph_id;
    uint64_t m_job_id;
    uint32_t m_input_cnt = 0;
    uint32_t m_output_cnt = 0;
};

class Aipu
{
public:
    /**
     * @brief This API is exposed as a SWIG-Python API.
     *        It is used to provide z1 simulator path.
     *
     * @param[in] z1_sim  Name of the simulator
     */
    void SetZ1Simulator(const char* z1_sim)
    {
        if (z1_sim != nullptr)
        {
            strcpy(m_z1_sim, z1_sim);
            m_enable_z1 = true;
        }
    }

    /**
     * @brief This API is exposed as a SWIG-Python API.
     *        It is used to provide z2 simulator path.
     *
     * @param[in] z1_sim  Name of the simulator
     */
    void SetZ2Simulator(const char* z2_sim)
    {
        if (z2_sim != nullptr)
        {
            strcpy(m_z2_sim, z2_sim);
            m_enable_z2 = true;
        }
    }

    /**
     * @brief This API is exposed as a SWIG-Python API.
     *        It is used to provide z3 simulator path.
     *
     * @param[in] z1_sim  Name of the simulator
     */
    void SetZ3Simulator(const char* z3_sim)
    {
        if (z3_sim != nullptr)
        {
            strcpy(m_z3_sim, z3_sim);
            m_enable_z3 = true;
        }
    }

    /**
     * @brief This API is exposed as a SWIG-Python API.
     *        It is used to load a graph.
     *
     * @param[in] filename  Name of the graph binary file
     */
    Graph* LoadGraph(char* filename)
    {
        aipu_status_t ret = AIPU_STATUS_SUCCESS;
        Graph* graph = nullptr;

#if (defined SIMULATION)
        aipu_global_config_simulation_t sim_glb_config;
        if (m_enable_z1)
        {
            sim_glb_config.z1_simulator = m_z1_sim;
        }
        else
        {
            sim_glb_config.z1_simulator = nullptr;
        }
        if (m_enable_z2)
        {
            sim_glb_config.z2_simulator = m_z2_sim;
        }
        else
        {
            sim_glb_config.z2_simulator = nullptr;
        }
        if (m_enable_z3)
        {
            sim_glb_config.z3_simulator = m_z1_sim;
        }
        else
        {
            sim_glb_config.z3_simulator = nullptr;
        }

#if ((defined RTDEBUG) && (RTDEBUG == 1))
        sim_glb_config.log_level = 3;
#else
        sim_glb_config.log_level = 0;
#endif
        sim_glb_config.verbose = 0;
        ret = aipu_config_global(m_ctx, AIPU_CONFIG_TYPE_SIMULATION, &sim_glb_config);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(m_ctx, ret, &m_status_msg);
            fprintf(stderr, "[PY UMD ERROR] aipu_config_global: %s\n", m_status_msg);
            return nullptr;
        }
#endif

        ret = aipu_load_graph(m_ctx, filename, &m_graph_id);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(m_ctx, ret, &m_status_msg);
            fprintf(stderr, "[PY UMD ERROR] AIPU_load_graph_helper: %s\n", m_status_msg);
            return nullptr;
        }

        graph = new Graph(m_ctx, m_graph_id);
        graph->create_job();

        return graph;
    }

    /**
     * @brief This API is exposed as a SWIG-Python API.
     *        It is used to unload a graph.
     *
     * @param[in] graph Pointer to the graph object
     */
    void UnloadGraph(Graph* graph)
    {
        if (graph != nullptr)
        {
            delete graph;
        }
    }

public:
    static Aipu& get_aipu()
    {
        static Aipu aipu;
        return aipu;
    }
    int init_dev()
    {
        aipu_status_t ret = AIPU_STATUS_SUCCESS;

        if (m_ctx != nullptr)
        {
            return 0;
        }

        ret = aipu_init_context(&m_ctx);
        if (ret != AIPU_STATUS_SUCCESS)
        {
            aipu_get_error_message(m_ctx, ret, &m_status_msg);
            fprintf(stderr, "[PY UMD ERROR] aipu_init_context: %s\n", m_status_msg);
            m_ctx = nullptr;
        }
        return ret;
    }
    void deinit_dev()
    {
        if (m_ctx)
        {
            aipu_deinit_context(m_ctx);
            m_ctx = nullptr;
        }
    }

private:
    Aipu(){};
    Aipu(const Aipu& aipu) = delete;
    Aipu& operator=(const Aipu& aipu) = delete;
    ~Aipu(){};

private:
    aipu_ctx_handle_t* m_ctx = nullptr;
    const char* m_status_msg;
    char        m_z1_sim[1024];
    bool        m_enable_z1 = false;
    char        m_z2_sim[1024];
    bool        m_enable_z2 = false;
    char        m_z3_sim[1024];
    bool        m_enable_z3 = false;
    uint64_t    m_graph_id;
};

/**
 * @brief This API is exposed as a SWIG-Python API.
 *        It is used to open the AIPU device which should be called first of all.
 */
Aipu& OpenDevice();
/**
 * @brief This API is exposed as a SWIG-Python API.
 *        It is used to close an opened AIPU device which should be called at last.
 */
void  CloseDevice();

#endif /* _SWIG_CPP2PY_API_HPP_ */