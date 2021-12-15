#ifndef __AIPU_H__
#define __AIPU_H__

#include <stdint.h>
#include <memory>

#define TSM_CMD_SCHED_ADDR_HI    0x8
#define TSM_CMD_SCHED_ADDR_LO    0xC
#define TSM_CMD_SCHED_CTRL       0x0
#define CREATE_CMD_POOL          0x1
#define DESTROY_CMD_POOL         0x2
#define DISPATCH_CMD_POOL        0x4
#define CMD_POOL0_STATUS         0x804
#define CLUSTER0_CONFIG          0xC00
#define CLUSTER0_CTRL            0xC04
#define CMD_POOL0_IDLE           0b100000

namespace sim_aipu
{
    class IMemEngine;

    class Aipu
    {
    public:
        Aipu(const struct config_t &, IMemEngine &);
        ~Aipu();

        Aipu(const Aipu &) = delete;
        Aipu &operator=(const Aipu &) = delete;

        int read_register(uint32_t addr, uint32_t &v) const;

        int write_register(uint32_t addr, uint32_t v);

        static int version();

        int test_execute(uint32_t pc);

        int test_aiff();
        int test_func();

    private:
        std::unique_ptr<struct AipuImpl> impl_;
    };
} //!sim_aipu

#endif //!__AIPU_H__
