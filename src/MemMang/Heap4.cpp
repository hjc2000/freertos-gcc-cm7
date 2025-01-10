/*
 * A sample implementation of pvPortMalloc() and vPortFree() that combines
 * (coalescences) adjacent memory blocks as they are freed, and in so doing
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the
 * memory management pages of https://www.FreeRTOS.org for more information.
 */
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
    /* The size of the structure placed at the beginning of each allocated memory
     * block must by correctly byte aligned. */
    size_t constexpr _size_of_heap_block_linklist_element = (sizeof(freertos::BlockLink_t) + ((size_t)(portBYTE_ALIGNMENT - 1))) & ~((size_t)portBYTE_ALIGNMENT_MASK);

    /// @brief 堆块的最小大小。
    /// @note 将 _size_of_heap_block_linklist_element 左移 1 位，即乘 2，说明最小大小定为
    /// _size_of_heap_block_linklist_element 的 2 倍。这可能是因为如果块太小了，链表的空间利用率
    /// 很低，链表节点本身占用的内存都比指向的内存块大了，那这块内存就不值得用一个链表节点去指向它。
    size_t constexpr _heap_minimum_block_size = ((size_t)(_size_of_heap_block_linklist_element << 1));

    uint8_t _buffer[configTOTAL_HEAP_SIZE];

} // namespace

freertos::Heap4 freertos::_heap4{_buffer, sizeof(_buffer)};

void freertos::Heap4::InsertBlockIntoFreeList(freertos::BlockLink_t *pxBlockToInsert)
{
    freertos::BlockLink_t *pxIterator;
    uint8_t *puc;

    /* Iterate through the list until a block is found that has a higher address
     * than the block being inserted. */
    for (pxIterator = &_head_element; pxIterator->_next_free_block < pxBlockToInsert; pxIterator = pxIterator->_next_free_block)
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
        if (pxIterator->_next_free_block != _tail_element)
        {
            /* Form one big block from the two blocks. */
            pxBlockToInsert->_size += pxIterator->_next_free_block->_size;
            pxBlockToInsert->_next_free_block = pxIterator->_next_free_block->_next_free_block;
        }
        else
        {
            pxBlockToInsert->_next_free_block = _tail_element;
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

freertos::Heap4::Heap4(uint8_t *buffer, size_t size)
{
    BlockLink_t *pxFirstFreeBlock;
    uint8_t *pucAlignedHeap;
    portPOINTER_SIZE_TYPE uxAddress;
    size_t xTotalHeapSize = size;

    /* Ensure the heap starts on a correctly aligned boundary. */
    uxAddress = (portPOINTER_SIZE_TYPE)buffer;

    if ((uxAddress & portBYTE_ALIGNMENT_MASK) != 0)
    {
        uxAddress += (portBYTE_ALIGNMENT - 1);
        uxAddress &= ~((portPOINTER_SIZE_TYPE)portBYTE_ALIGNMENT_MASK);
        xTotalHeapSize -= uxAddress - (portPOINTER_SIZE_TYPE)buffer;
    }

    pucAlignedHeap = (uint8_t *)uxAddress;

    /* _head_element is used to hold a pointer to the first item in the list of free
     * blocks.  The void cast is used to prevent compiler warnings. */
    _head_element._next_free_block = (BlockLink_t *)pucAlignedHeap;
    _head_element._size = (size_t)0;

    /* _tail_element is used to mark the end of the list of free blocks and is inserted
     * at the end of the heap space. */
    uxAddress = ((portPOINTER_SIZE_TYPE)pucAlignedHeap) + xTotalHeapSize;
    uxAddress -= _size_of_heap_block_linklist_element;
    uxAddress &= ~((portPOINTER_SIZE_TYPE)portBYTE_ALIGNMENT_MASK);
    _tail_element = (BlockLink_t *)uxAddress;
    _tail_element->_size = 0;
    _tail_element->_next_free_block = NULL;

    /* To start with there is a single free block that is sized to take up the
     * entire heap space, minus the space taken by _tail_element. */
    pxFirstFreeBlock = (BlockLink_t *)pucAlignedHeap;
    pxFirstFreeBlock->_size = (size_t)(uxAddress - (portPOINTER_SIZE_TYPE)pxFirstFreeBlock);
    pxFirstFreeBlock->_next_free_block = _tail_element;

    /* Only one block exists - and it covers the entire usable heap space. */
    xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->_size;
    xFreeBytesRemaining = pxFirstFreeBlock->_size;
}

void *freertos::Heap4::Malloc(size_t xWantedSize)
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
        if (_tail_element == NULL)
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

            if (!HeapAddWillOverflow(xWantedSize, xAdditionalRequiredSize))
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
        if (HeapBlockSizeIsValid(xWantedSize))
        {
            if ((xWantedSize > 0) && (xWantedSize <= xFreeBytesRemaining))
            {
                /* Traverse the list from the start (lowest address) block until
                 * one of adequate size is found. */
                pxPreviousBlock = &_head_element;
                pxBlock = _head_element._next_free_block;

                while ((pxBlock->_size < xWantedSize) && (pxBlock->_next_free_block != NULL))
                {
                    pxPreviousBlock = pxBlock;
                    pxBlock = pxBlock->_next_free_block;
                }

                /* If the end marker was reached then a block of adequate size
                 * was not found. */
                if (pxBlock != _tail_element)
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
                        InsertBlockIntoFreeList(pxNewBlockLink);
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    xFreeBytesRemaining -= pxBlock->_size;

                    if (xFreeBytesRemaining < xMinimumEverFreeBytesRemaining)
                    {
                        xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /* The block is being returned - it is allocated and owned
                     * by the application and has no "next" block. */
                    HeapAllocateBlock(pxBlock);
                    pxBlock->_next_free_block = NULL;
                    xNumberOfSuccessfulAllocations++;
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

void freertos::Heap4::Free(void *pv)
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

        configASSERT(HeapBlockIsAllocated(pxLink));
        configASSERT(pxLink->_next_free_block == NULL);

        if (HeapBlockIsAllocated(pxLink))
        {
            if (pxLink->_next_free_block == NULL)
            {
                /* The block is being returned to the heap - it is no longer
                 * allocated. */
                HeapFreeBlock(pxLink);
#if (configHEAP_CLEAR_MEMORY_ON_FREE == 1)
                {
                    (void)memset(puc + _size_of_heap_block_linklist_element, 0, pxLink->_size - _size_of_heap_block_linklist_element);
                }
#endif

                vTaskSuspendAll();
                {
                    /* Add this block to the list of free blocks. */
                    xFreeBytesRemaining += pxLink->_size;
                    traceFREE(pv, pxLink->_size);
                    InsertBlockIntoFreeList(((freertos::BlockLink_t *)pxLink));
                    xNumberOfSuccessfulFrees++;
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

void *freertos::Heap4::Calloc(size_t xNum, size_t xSize)
{
    void *pv = NULL;

    if (!HeapMultiplyWillOverflow(xNum, xSize))
    {
        pv = pvPortMalloc(xNum * xSize);

        if (pv != NULL)
        {
            (void)memset(pv, 0, xNum * xSize);
        }
    }

    return pv;
}

void freertos::Heap4::GetHeapStats(xHeapStats *pxHeapStats)
{
    freertos::BlockLink_t *pxBlock;
    size_t xBlocks = 0, xMaxSize = 0, xMinSize = portMAX_DELAY; /* portMAX_DELAY used as a portable way of getting the maximum value. */

    vTaskSuspendAll();
    {
        pxBlock = _head_element._next_free_block;

        /* pxBlock will be NULL if the heap has not been initialised.  The heap
         * is initialised automatically when the first allocation is made. */
        if (pxBlock != NULL)
        {
            while (pxBlock != _tail_element)
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
        pxHeapStats->xAvailableHeapSpaceInBytes = xFreeBytesRemaining;
        pxHeapStats->xNumberOfSuccessfulAllocations = xNumberOfSuccessfulAllocations;
        pxHeapStats->xNumberOfSuccessfulFrees = xNumberOfSuccessfulFrees;
        pxHeapStats->xMinimumEverFreeBytesRemaining = xMinimumEverFreeBytesRemaining;
    }
    taskEXIT_CRITICAL();
}
