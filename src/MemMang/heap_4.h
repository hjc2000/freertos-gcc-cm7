#pragma once
#include <stddef.h>
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

// 定义是否在释放内存的时候将释放的内存区域全部置为 0.
#ifndef configHEAP_CLEAR_MEMORY_ON_FREE
#define configHEAP_CLEAR_MEMORY_ON_FREE 0
#endif

namespace freertos
{
    /* Define the linked list structure.  This is used to link free blocks in order
     * of their memory address. */
    struct BlockLink_t
    {
        BlockLink_t *pxNextFreeBlock; /*<< The next free block in the list. */
        size_t xBlockSize;            /*<< The size of the free block. */

        bool heapBLOCK_IS_ALLOCATED();

        void heapALLOCATE_BLOCK();

        void heapFREE_BLOCK();
    };

    class FreertosHeap4
    {
    public:
        FreertosHeap4();

        /* Assumes 8bit bytes! */
        inline static size_t const heapBITS_PER_BYTE = ((size_t)8);

        /* Max value that fits in a size_t type. */
        inline static size_t const heapSIZE_MAX = (~((size_t)0));

        /* MSB of the xBlockSize member of an BlockLink_t structure is used to track
         * the allocation status of a block.  When MSB of the xBlockSize member of
         * an BlockLink_t structure is set then the block belongs to the application.
         * When the bit is free the block is still part of the free heap space. */
        inline static size_t const heapBLOCK_ALLOCATED_BITMASK = (((size_t)1) << ((sizeof(size_t) * heapBITS_PER_BYTE) - 1));

        /* Keeps track of the number of calls to allocate and free memory as well as the
         * number of free bytes remaining, but says nothing about fragmentation. */
        size_t xFreeBytesRemaining = 0U;
        size_t xMinimumEverFreeBytesRemaining = 0U;
        size_t xNumberOfSuccessfulAllocations = 0;
        size_t xNumberOfSuccessfulFrees = 0;
    };
} // namespace freertos
