#include "Heap4.h"
#include <bsp-interface/di/heap.h>
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
    freertos::Heap4 _heap4{_buffer, sizeof(_buffer)};

} // namespace

/// @brief 获取主堆。
/// @return
bsp::IHeap &bsp::di::heap::Heap()
{
    return _heap4;
}

std::shared_ptr<bsp::IHeap> bsp::di::heap::CreateHeap(uint8_t *buffer, size_t size)
{
    return std::shared_ptr<bsp::IHeap>{new freertos::Heap4{buffer, size}};
}

/**
 * 实现 freertos 的函数。
 */
extern "C"
{
    void *pvPortMalloc(size_t xWantedSize)
    {
        return _heap4.Malloc(xWantedSize);
    }

    /*-----------------------------------------------------------*/

    void vPortFree(void *pv)
    {
        _heap4.Free(pv);
    }

    /*-----------------------------------------------------------*/

    size_t xPortGetFreeHeapSize(void)
    {
        return _heap4.xFreeBytesRemaining;
    }

    /*-----------------------------------------------------------*/

    size_t xPortGetMinimumEverFreeHeapSize(void)
    {
        return _heap4.xMinimumEverFreeBytesRemaining;
    }

    /*-----------------------------------------------------------*/

    void vPortInitialiseBlocks(void)
    {
        /* This just exists to keep the linker quiet. */
    }

    /*-----------------------------------------------------------*/

    void *pvPortCalloc(size_t xNum, size_t xSize)
    {
        return _heap4.Calloc(xNum, xSize);
    }

    /*-----------------------------------------------------------*/

    void vPortGetHeapStats(HeapStats_t *pxHeapStats)
    {
        _heap4.GetHeapStats(pxHeapStats);
    }
}
