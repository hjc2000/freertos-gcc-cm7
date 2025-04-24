#include "base/define.h"
#include "base/peripheral/heap/heap.h"
#include "base/peripheral/heap/IHeap.h"
#include "Heap4.h"
#include <stdlib.h>
#include <string.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
 * all the API functions to use the MPU wrappers.  That should only be done when
 * task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#include "FreeRTOS.h"
#include "task.h"
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if (configSUPPORT_DYNAMIC_ALLOCATION == 0)
	#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/**
 * 释放内存时是否将这段内存的每个字节都设成 0.
 */
#ifndef configHEAP_CLEAR_MEMORY_ON_FREE
	#define configHEAP_CLEAR_MEMORY_ON_FREE 0
#endif

namespace
{
	uint8_t _buffer[configTOTAL_HEAP_SIZE];

	freertos::Heap4 &Heap4()
	{
		static freertos::Heap4 heap4{_buffer, sizeof(_buffer)};
		return heap4;
	}

} // namespace

PREINIT(Heap4)

/// @brief 获取主堆。
/// @return
base::heap::IHeap &base::heap::Heap()
{
	return Heap4();
}

std::shared_ptr<base::heap::IHeap> base::heap::CreateHeap(uint8_t *buffer, size_t size)
{
	return std::shared_ptr<base::heap::IHeap>{new freertos::Heap4{buffer, size}};
}

/**
 * 实现 freertos 的函数。
 */
extern "C"
{
	void *pvPortMalloc(size_t xWantedSize)
	{
		return Heap4().Malloc(xWantedSize);
	}

	/*-----------------------------------------------------------*/

	void vPortFree(void *pv)
	{
		Heap4().Free(pv);
	}

	/*-----------------------------------------------------------*/

	size_t xPortGetFreeHeapSize(void)
	{
		return Heap4().xFreeBytesRemaining;
	}

	/*-----------------------------------------------------------*/

	size_t xPortGetMinimumEverFreeHeapSize(void)
	{
		return Heap4().xMinimumEverFreeBytesRemaining;
	}

	/*-----------------------------------------------------------*/

	void vPortInitialiseBlocks(void)
	{
		/* This just exists to keep the linker quiet. */
	}

	/*-----------------------------------------------------------*/

	void *pvPortCalloc(size_t xNum, size_t xSize)
	{
		return Heap4().Calloc(xNum, xSize);
	}

	/*-----------------------------------------------------------*/

	void vPortGetHeapStats(HeapStats_t *pxHeapStats)
	{
		Heap4().GetHeapStats(pxHeapStats);
	}
}
