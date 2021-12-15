#ifndef __AIPU_CONFIG_H__
#define __AIPU_CONFIG_H__

#include <stdint.h>

namespace sim_aipu
{
    struct config_t
    {
        enum
        {
            Z5_1308 = 0,
            Z5_1204,
            Z5_1308_MP4,
            Z5_0901,
        };
        int code;
        bool enable_avx;
        struct
        {
            std::string filepath;
            int level;
            bool verbose;
        }log;
    };
} //!sim_aipu

#endif //!__AIPU_CONFIG_H__
