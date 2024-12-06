#include "FreeRTOS.h"
#include "task.h"
#include <cstddef>
#include <hal.h>
#include <new>

void *operator new(size_t size)
{
    __disable_irq();
    void *ptr = pvPortMalloc(size);
    __enable_irq();
    if (ptr == nullptr)
    {
        throw std::bad_alloc{};
    }

    return ptr;
}

void *operator new[](size_t size)
{
    __disable_irq();
    void *ptr = pvPortMalloc(size);
    __enable_irq();
    if (ptr == nullptr)
    {
        throw std::bad_alloc{};
    }

    return ptr;
}

void operator delete(void *ptr) noexcept
{
    __disable_irq();
    vPortFree(ptr);
    __enable_irq();
}

void operator delete[](void *ptr) noexcept
{
    __disable_irq();
    vPortFree(ptr);
    __enable_irq();
}

void *operator new(size_t size, std::nothrow_t const &) noexcept
{
    __disable_irq();
    void *p = pvPortMalloc(size);
    __enable_irq();
    return p;
}

void *operator new[](size_t size, std::nothrow_t const &) noexcept
{
    __disable_irq();
    void *p = pvPortMalloc(size);
    __enable_irq();
    return p;
}

void operator delete(void *ptr, std::nothrow_t const &) noexcept
{
    __disable_irq();
    vPortFree(ptr);
    __enable_irq();
}

void operator delete[](void *ptr, std::nothrow_t const &) noexcept
{
    __disable_irq();
    vPortFree(ptr);
    __enable_irq();
}

void operator delete(void *ptr, size_t size) noexcept
{
    __disable_irq();
    vPortFree(ptr);
    __enable_irq();
}

void operator delete[](void *ptr, size_t size) noexcept
{
    __disable_irq();
    vPortFree(ptr);
    __enable_irq();
}
