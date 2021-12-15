#ifndef __MEM_ENGINE_BASE_H__
#define __MEM_ENGINE_BASE_H__

#include <stddef.h>
#include <stdint.h>
#include <memory>

namespace sim_aipu
{
    class IMemEngine;

    std::unique_ptr<IMemEngine> make_mem_engine();

    std::unique_ptr<IMemEngine> make_mem_engine(uint64_t msize, uint32_t psize);

    class IMemEngine
    {
    public:
        virtual ~IMemEngine() = default;

        virtual int read(uint64_t addr, void *dest, size_t size) const = 0;

        virtual int write(uint64_t addr, const void *src, size_t size) = 0;

        virtual int bzero(uint64_t addr, size_t size) = 0;

        virtual size_t size() const = 0;

        virtual bool invalid(uint64_t addr) const = 0;
    };
} // namespace sim_aipu
#endif // !__MEM_ENGINE_BASE_H__
