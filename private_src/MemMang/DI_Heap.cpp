#include "base/define.h"
#include "base/embedded/heap/heap.h"
#include "base/embedded/heap/Heap4.h"
#include "base/embedded/heap/IHeap.h"
#include "FreeRTOS.h"
#include <stdlib.h>
#include <string.h>

namespace
{
	uint8_t _buffer[configTOTAL_HEAP_SIZE];

	base::heap::Heap4 &Heap4Instance()
	{
		static base::heap::Heap4 heap4{_buffer, sizeof(_buffer)};
		return heap4;
	}

} // namespace

PREINIT(Heap4Instance)

/// @brief 获取主堆。
/// @return
base::heap::IHeap &base::heap::Heap()
{
	return Heap4Instance();
}

std::shared_ptr<base::heap::IHeap> base::heap::CreateHeap(uint8_t *buffer, size_t size)
{
	return std::shared_ptr<base::heap::IHeap>{new base::heap::Heap4{buffer, size}};
}

/**
 * 实现 freertos 的函数。
 */
extern "C"
{
	void *pvPortMalloc(size_t xWantedSize)
	{
		return base::heap::Malloc(xWantedSize);
	}

	/*-----------------------------------------------------------*/

	void vPortFree(void *pv)
	{
		base::heap::Free(pv);
	}
}
