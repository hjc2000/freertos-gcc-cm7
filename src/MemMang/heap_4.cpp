/*
 * A sample implementation of pvPortMalloc() and vPortFree() that combines
 * (coalescences) adjacent memory blocks as they are freed, and in so doing
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the
 * memory management pages of https://www.FreeRTOS.org for more information.
 */
#include "heap_4.h"
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
    /* The size of the structure placed at the beginning of each allocated memory
     * block must by correctly byte aligned. */
    size_t constexpr _size_of_heap_block_linklist_element = (sizeof(freertos::BlockLink_t) + ((size_t)(portBYTE_ALIGNMENT - 1))) & ~((size_t)portBYTE_ALIGNMENT_MASK);

    /// @brief 堆块的最小大小。
    /// @note 将 _size_of_heap_block_linklist_element 左移 1 位，即乘 2，说明最小大小定为
    /// _size_of_heap_block_linklist_element 的 2 倍。这可能是因为如果块太小了，链表的空间利用率
    /// 很低，链表节点本身占用的内存都比指向的内存块大了，那这块内存就不值得用一个链表节点去指向它。
    size_t constexpr _heap_minimum_block_size = ((size_t)(_size_of_heap_block_linklist_element << 1));

    /// @brief 假设 1 个字节有 8 位。
    size_t constexpr _bit_count_per_byte = ((size_t)8);

    /* Max value that fits in a size_t type. */
    size_t constexpr heapSIZE_MAX = (~((size_t)0));

    /* Check if multiplying a and b will result in overflow. */
    bool constexpr heapMULTIPLY_WILL_OVERFLOW(size_t a, size_t b)
    {
        return (((a) > 0) && ((b) > (heapSIZE_MAX / (a))));
    }

    /* Check if adding a and b will result in overflow. */
    bool constexpr heapADD_WILL_OVERFLOW(size_t a, size_t b)
    {
        return ((a) > (heapSIZE_MAX - (b)));
    }

    /* MSB of the _size member of an BlockLink_t structure is used to track
     * the allocation status of a block.  When MSB of the _size member of
     * an BlockLink_t structure is set then the block belongs to the application.
     * When the bit is free the block is still part of the free heap space. */
    size_t constexpr heapBLOCK_ALLOCATED_BITMASK = ((size_t)1) << ((sizeof(size_t) * _bit_count_per_byte) - 1);

    bool constexpr heapBLOCK_SIZE_IS_VALID(size_t _size)
    {
        return ((_size)&heapBLOCK_ALLOCATED_BITMASK) == 0;
    }

    bool constexpr heapBLOCK_IS_ALLOCATED(freertos::BlockLink_t *pxBlock)
    {
        return ((pxBlock->_size) & heapBLOCK_ALLOCATED_BITMASK) != 0;
    }

    void constexpr heapALLOCATE_BLOCK(freertos::BlockLink_t *pxBlock)
    {
        (pxBlock->_size) |= heapBLOCK_ALLOCATED_BITMASK;
    }

    void constexpr heapFREE_BLOCK(freertos::BlockLink_t *pxBlock)
    {
        (pxBlock->_size) &= ~heapBLOCK_ALLOCATED_BITMASK;
    }

    uint8_t _ucHeap[configTOTAL_HEAP_SIZE];

    freertos::Heap4 _heap4;

} // namespace

extern "C"
{
    /*-----------------------------------------------------------*/

    /*
     * Inserts a block of memory that is being freed into the correct position in
     * the list of free memory blocks.  The block being freed will be merged with
     * the block in front it and/or the block behind it if the memory blocks are
     * adjacent to each other.
     */
    void prvInsertBlockIntoFreeList(freertos::BlockLink_t *pxBlockToInsert);

    void *pvPortMalloc(size_t xWantedSize)
    {
        freertos::BlockLink_t *pxBlock;
        freertos::BlockLink_t *pxPreviousBlock;
        freertos::BlockLink_t *pxNewBlockLink;
        void *pvReturn = NULL;
        size_t xAdditionalRequiredSize;

        vTaskSuspendAll();
        {
            /* If this is the first call to malloc then the heap will require
             * initialisation to setup the list of free blocks. */
            if (_heap4.pxEnd == NULL)
            {
                // 已被构造函数取代。
                //  prvHeapInit();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            if (xWantedSize > 0)
            {
                /* The wanted size must be increased so it can contain a BlockLink_t
                 * structure in addition to the requested amount of bytes. Some
                 * additional increment may also be needed for alignment. */
                xAdditionalRequiredSize = _size_of_heap_block_linklist_element + portBYTE_ALIGNMENT - (xWantedSize & portBYTE_ALIGNMENT_MASK);

                if (!heapADD_WILL_OVERFLOW(xWantedSize, xAdditionalRequiredSize))
                {
                    xWantedSize += xAdditionalRequiredSize;
                }
                else
                {
                    xWantedSize = 0;
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            /* Check the block size we are trying to allocate is not so large that the
             * top bit is set.  The top bit of the block size member of the BlockLink_t
             * structure is used to determine who owns the block - the application or
             * the kernel, so it must be free. */
            if (heapBLOCK_SIZE_IS_VALID(xWantedSize))
            {
                if ((xWantedSize > 0) && (xWantedSize <= _heap4.xFreeBytesRemaining))
                {
                    /* Traverse the list from the start (lowest address) block until
                     * one of adequate size is found. */
                    pxPreviousBlock = &_heap4.xStart;
                    pxBlock = _heap4.xStart._next_free_block;

                    while ((pxBlock->_size < xWantedSize) && (pxBlock->_next_free_block != NULL))
                    {
                        pxPreviousBlock = pxBlock;
                        pxBlock = pxBlock->_next_free_block;
                    }

                    /* If the end marker was reached then a block of adequate size
                     * was not found. */
                    if (pxBlock != _heap4.pxEnd)
                    {
                        /* Return the memory space pointed to - jumping over the
                         * BlockLink_t structure at its start. */
                        pvReturn = (void *)(((uint8_t *)pxPreviousBlock->_next_free_block) + _size_of_heap_block_linklist_element);

                        /* This block is being returned for use so must be taken out
                         * of the list of free blocks. */
                        pxPreviousBlock->_next_free_block = pxBlock->_next_free_block;

                        /* If the block is larger than required it can be split into
                         * two. */
                        if ((pxBlock->_size - xWantedSize) > _heap_minimum_block_size)
                        {
                            /* This block is to be split into two.  Create a new
                             * block following the number of bytes requested. The void
                             * cast is used to prevent byte alignment warnings from the
                             * compiler. */
                            pxNewBlockLink = (freertos::BlockLink_t *)(((uint8_t *)pxBlock) + xWantedSize);
                            configASSERT((((size_t)pxNewBlockLink) & portBYTE_ALIGNMENT_MASK) == 0);

                            /* Calculate the sizes of two blocks split from the
                             * single block. */
                            pxNewBlockLink->_size = pxBlock->_size - xWantedSize;
                            pxBlock->_size = xWantedSize;

                            /* Insert the new block into the list of free blocks. */
                            prvInsertBlockIntoFreeList(pxNewBlockLink);
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }

                        _heap4.xFreeBytesRemaining -= pxBlock->_size;

                        if (_heap4.xFreeBytesRemaining < _heap4.xMinimumEverFreeBytesRemaining)
                        {
                            _heap4.xMinimumEverFreeBytesRemaining = _heap4.xFreeBytesRemaining;
                        }
                        else
                        {
                            mtCOVERAGE_TEST_MARKER();
                        }

                        /* The block is being returned - it is allocated and owned
                         * by the application and has no "next" block. */
                        heapALLOCATE_BLOCK(pxBlock);
                        pxBlock->_next_free_block = NULL;
                        _heap4.xNumberOfSuccessfulAllocations++;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            traceMALLOC(pvReturn, xWantedSize);
        }
        (void)xTaskResumeAll();

#if (configUSE_MALLOC_FAILED_HOOK == 1)
        {
            if (pvReturn == NULL)
            {
                vApplicationMallocFailedHook();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
#endif /* if ( configUSE_MALLOC_FAILED_HOOK == 1 ) */

        configASSERT((((size_t)pvReturn) & (size_t)portBYTE_ALIGNMENT_MASK) == 0);
        return pvReturn;
    }

    /*-----------------------------------------------------------*/

    void vPortFree(void *pv)
    {
        uint8_t *puc = (uint8_t *)pv;
        freertos::BlockLink_t *pxLink;

        if (pv != NULL)
        {
            /* The memory being freed will have an BlockLink_t structure immediately
             * before it. */
            puc -= _size_of_heap_block_linklist_element;

            /* This casting is to keep the compiler from issuing warnings. */
            pxLink = (freertos::BlockLink_t *)puc;

            configASSERT(heapBLOCK_IS_ALLOCATED(pxLink));
            configASSERT(pxLink->_next_free_block == NULL);

            if (heapBLOCK_IS_ALLOCATED(pxLink))
            {
                if (pxLink->_next_free_block == NULL)
                {
                    /* The block is being returned to the heap - it is no longer
                     * allocated. */
                    heapFREE_BLOCK(pxLink);
#if (configHEAP_CLEAR_MEMORY_ON_FREE == 1)
                    {
                        (void)memset(puc + _size_of_heap_block_linklist_element, 0, pxLink->_size - _size_of_heap_block_linklist_element);
                    }
#endif

                    vTaskSuspendAll();
                    {
                        /* Add this block to the list of free blocks. */
                        _heap4.xFreeBytesRemaining += pxLink->_size;
                        traceFREE(pv, pxLink->_size);
                        prvInsertBlockIntoFreeList(((freertos::BlockLink_t *)pxLink));
                        _heap4.xNumberOfSuccessfulFrees++;
                    }
                    (void)xTaskResumeAll();
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
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
        void *pv = NULL;

        if (!heapMULTIPLY_WILL_OVERFLOW(xNum, xSize))
        {
            pv = pvPortMalloc(xNum * xSize);

            if (pv != NULL)
            {
                (void)memset(pv, 0, xNum * xSize);
            }
        }

        return pv;
    }

    void prvInsertBlockIntoFreeList(freertos::BlockLink_t *pxBlockToInsert) /* PRIVILEGED_FUNCTION */
    {
        freertos::BlockLink_t *pxIterator;
        uint8_t *puc;

        /* Iterate through the list until a block is found that has a higher address
         * than the block being inserted. */
        for (pxIterator = &_heap4.xStart; pxIterator->_next_free_block < pxBlockToInsert; pxIterator = pxIterator->_next_free_block)
        {
            /* Nothing to do here, just iterate to the right position. */
        }

        /* Do the block being inserted, and the block it is being inserted after
         * make a contiguous block of memory? */
        puc = (uint8_t *)pxIterator;

        if ((puc + pxIterator->_size) == (uint8_t *)pxBlockToInsert)
        {
            pxIterator->_size += pxBlockToInsert->_size;
            pxBlockToInsert = pxIterator;
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        /* Do the block being inserted, and the block it is being inserted before
         * make a contiguous block of memory? */
        puc = (uint8_t *)pxBlockToInsert;

        if ((puc + pxBlockToInsert->_size) == (uint8_t *)pxIterator->_next_free_block)
        {
            if (pxIterator->_next_free_block != _heap4.pxEnd)
            {
                /* Form one big block from the two blocks. */
                pxBlockToInsert->_size += pxIterator->_next_free_block->_size;
                pxBlockToInsert->_next_free_block = pxIterator->_next_free_block->_next_free_block;
            }
            else
            {
                pxBlockToInsert->_next_free_block = _heap4.pxEnd;
            }
        }
        else
        {
            pxBlockToInsert->_next_free_block = pxIterator->_next_free_block;
        }

        /* If the block being inserted plugged a gab, so was merged with the block
         * before and the block after, then it's _next_free_block pointer will have
         * already been set, and should not be set here as that would make it point
         * to itself. */
        if (pxIterator != pxBlockToInsert)
        {
            pxIterator->_next_free_block = pxBlockToInsert;
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }

    /*-----------------------------------------------------------*/

    void vPortGetHeapStats(HeapStats_t *pxHeapStats)
    {
        freertos::BlockLink_t *pxBlock;
        size_t xBlocks = 0, xMaxSize = 0, xMinSize = portMAX_DELAY; /* portMAX_DELAY used as a portable way of getting the maximum value. */

        vTaskSuspendAll();
        {
            pxBlock = _heap4.xStart._next_free_block;

            /* pxBlock will be NULL if the heap has not been initialised.  The heap
             * is initialised automatically when the first allocation is made. */
            if (pxBlock != NULL)
            {
                while (pxBlock != _heap4.pxEnd)
                {
                    /* Increment the number of blocks and record the largest block seen
                     * so far. */
                    xBlocks++;

                    if (pxBlock->_size > xMaxSize)
                    {
                        xMaxSize = pxBlock->_size;
                    }

                    if (pxBlock->_size < xMinSize)
                    {
                        xMinSize = pxBlock->_size;
                    }

                    /* Move to the next block in the chain until the last block is
                     * reached. */
                    pxBlock = pxBlock->_next_free_block;
                }
            }
        }
        (void)xTaskResumeAll();

        pxHeapStats->xSizeOfLargestFreeBlockInBytes = xMaxSize;
        pxHeapStats->xSizeOfSmallestFreeBlockInBytes = xMinSize;
        pxHeapStats->xNumberOfFreeBlocks = xBlocks;

        taskENTER_CRITICAL();
        {
            pxHeapStats->xAvailableHeapSpaceInBytes = _heap4.xFreeBytesRemaining;
            pxHeapStats->xNumberOfSuccessfulAllocations = _heap4.xNumberOfSuccessfulAllocations;
            pxHeapStats->xNumberOfSuccessfulFrees = _heap4.xNumberOfSuccessfulFrees;
            pxHeapStats->xMinimumEverFreeBytesRemaining = _heap4.xMinimumEverFreeBytesRemaining;
        }
        taskEXIT_CRITICAL();
    }
}

freertos::Heap4::Heap4()
{
    BlockLink_t *pxFirstFreeBlock;
    uint8_t *pucAlignedHeap;
    portPOINTER_SIZE_TYPE uxAddress;
    size_t xTotalHeapSize = configTOTAL_HEAP_SIZE;

    /* Ensure the heap starts on a correctly aligned boundary. */
    uxAddress = (portPOINTER_SIZE_TYPE)_ucHeap;

    if ((uxAddress & portBYTE_ALIGNMENT_MASK) != 0)
    {
        uxAddress += (portBYTE_ALIGNMENT - 1);
        uxAddress &= ~((portPOINTER_SIZE_TYPE)portBYTE_ALIGNMENT_MASK);
        xTotalHeapSize -= uxAddress - (portPOINTER_SIZE_TYPE)_ucHeap;
    }

    pucAlignedHeap = (uint8_t *)uxAddress;

    /* xStart is used to hold a pointer to the first item in the list of free
     * blocks.  The void cast is used to prevent compiler warnings. */
    xStart._next_free_block = (BlockLink_t *)pucAlignedHeap;
    xStart._size = (size_t)0;

    /* pxEnd is used to mark the end of the list of free blocks and is inserted
     * at the end of the heap space. */
    uxAddress = ((portPOINTER_SIZE_TYPE)pucAlignedHeap) + xTotalHeapSize;
    uxAddress -= _size_of_heap_block_linklist_element;
    uxAddress &= ~((portPOINTER_SIZE_TYPE)portBYTE_ALIGNMENT_MASK);
    pxEnd = (BlockLink_t *)uxAddress;
    pxEnd->_size = 0;
    pxEnd->_next_free_block = NULL;

    /* To start with there is a single free block that is sized to take up the
     * entire heap space, minus the space taken by pxEnd. */
    pxFirstFreeBlock = (BlockLink_t *)pucAlignedHeap;
    pxFirstFreeBlock->_size = (size_t)(uxAddress - (portPOINTER_SIZE_TYPE)pxFirstFreeBlock);
    pxFirstFreeBlock->_next_free_block = pxEnd;

    /* Only one block exists - and it covers the entire usable heap space. */
    xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->_size;
    xFreeBytesRemaining = pxFirstFreeBlock->_size;
}
