#include "FreeRTOS.h"
#include "task.h"
#include <cstddef>
#include <new>

void *operator new(size_t size)
{
	void *ptr = pvPortMalloc(size);
	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}

	return ptr;
}

void *operator new[](size_t size)
{
	void *ptr = pvPortMalloc(size);
	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}

	return ptr;
}

void operator delete(void *ptr) noexcept
{
	vPortFree(ptr);
}

void operator delete[](void *ptr) noexcept
{
	vPortFree(ptr);
}

void *operator new(size_t size, std::nothrow_t const &) noexcept
{
	return pvPortMalloc(size);
}

void *operator new[](size_t size, std::nothrow_t const &) noexcept
{
	return pvPortMalloc(size);
}

void operator delete(void *ptr, std::nothrow_t const &) noexcept
{
	vPortFree(ptr);
}

void operator delete[](void *ptr, std::nothrow_t const &) noexcept
{
	vPortFree(ptr);
}

void operator delete(void *ptr, size_t size) noexcept
{
	vPortFree(ptr);
}

void operator delete[](void *ptr, size_t size) noexcept
{
	vPortFree(ptr);
}
