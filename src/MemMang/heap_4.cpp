#include "heap_4.h"

bool freertos::FreertosHeap4::heapMULTIPLY_WILL_OVERFLOW(size_t a, size_t b)
{
    return (a > 0) && (b > (heapSIZE_MAX / a));
}

bool freertos::FreertosHeap4::heapBLOCK_SIZE_IS_VALID(size_t xBlockSize)
{
    return (xBlockSize & heapBLOCK_ALLOCATED_BITMASK) == 0;
}

bool freertos::FreertosHeap4::heapADD_WILL_OVERFLOW(size_t a, size_t b)
{
    return a > (heapSIZE_MAX - b);
}

bool freertos::FreertosHeap4::heapBLOCK_IS_ALLOCATED(freertos::BlockLink_t *p)
{
    return (p->xBlockSize & heapBLOCK_ALLOCATED_BITMASK) != 0;
}

void freertos::FreertosHeap4::heapALLOCATE_BLOCK(freertos::BlockLink_t *p)
{
    p->xBlockSize |= heapBLOCK_ALLOCATED_BITMASK;
}

void freertos::FreertosHeap4::heapFREE_BLOCK(freertos::BlockLink_t *p)
{
    p->xBlockSize &= ~freertos::FreertosHeap4::heapBLOCK_ALLOCATED_BITMASK;
}

void freertos::FreertosHeap4::prvInsertBlockIntoFreeList(freertos::BlockLink_t *pxBlockToInsert)
{
    freertos::BlockLink_t *pxIterator;
    uint8_t *puc;

    /* Iterate through the list until a block is found that has a higher address
     * than the block being inserted. */
    for (pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock)
    {
        /* Nothing to do here, just iterate to the right position. */
    }

    /* Do the block being inserted, and the block it is being inserted after
     * make a contiguous block of memory? */
    puc = (uint8_t *)pxIterator;

    if ((puc + pxIterator->xBlockSize) == (uint8_t *)pxBlockToInsert)
    {
        pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
        pxBlockToInsert = pxIterator;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    /* Do the block being inserted, and the block it is being inserted before
     * make a contiguous block of memory? */
    puc = (uint8_t *)pxBlockToInsert;

    if ((puc + pxBlockToInsert->xBlockSize) == (uint8_t *)pxIterator->pxNextFreeBlock)
    {
        if (pxIterator->pxNextFreeBlock != pxEnd)
        {
            /* Form one big block from the two blocks. */
            pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
            pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
        }
        else
        {
            pxBlockToInsert->pxNextFreeBlock = pxEnd;
        }
    }
    else
    {
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
    }

    /* If the block being inserted plugged a gab, so was merged with the block
     * before and the block after, then it's pxNextFreeBlock pointer will have
     * already been set, and should not be set here as that would make it point
     * to itself. */
    if (pxIterator != pxBlockToInsert)
    {
        pxIterator->pxNextFreeBlock = pxBlockToInsert;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
}

freertos::FreertosHeap4::FreertosHeap4(uint8_t *buffer, size_t size)
{
    freertos::BlockLink_t *pxFirstFreeBlock;
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

    /* xStart is used to hold a pointer to the first item in the list of free
     * blocks.  The void cast is used to prevent compiler warnings. */
    xStart.pxNextFreeBlock = (freertos::BlockLink_t *)pucAlignedHeap;
    xStart.xBlockSize = (size_t)0;

    /* pxEnd is used to mark the end of the list of free blocks and is inserted
     * at the end of the heap space. */
    uxAddress = ((portPOINTER_SIZE_TYPE)pucAlignedHeap) + xTotalHeapSize;
    uxAddress -= heap_struct_size;
    uxAddress &= ~((portPOINTER_SIZE_TYPE)portBYTE_ALIGNMENT_MASK);
    pxEnd = (freertos::BlockLink_t *)uxAddress;
    pxEnd->xBlockSize = 0;
    pxEnd->pxNextFreeBlock = NULL;

    /* To start with there is a single free block that is sized to take up the
     * entire heap space, minus the space taken by pxEnd. */
    pxFirstFreeBlock = (freertos::BlockLink_t *)pucAlignedHeap;
    pxFirstFreeBlock->xBlockSize = (size_t)(uxAddress - (portPOINTER_SIZE_TYPE)pxFirstFreeBlock);
    pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

    /* Only one block exists - and it covers the entire usable heap space. */
    xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
    xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
}

void *freertos::FreertosHeap4::Malloc(size_t xWantedSize)
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
        if (pxEnd == NULL)
        {
            // prvHeapInit();
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
            xAdditionalRequiredSize = freertos::FreertosHeap4::heap_struct_size + portBYTE_ALIGNMENT - (xWantedSize & portBYTE_ALIGNMENT_MASK);

            if (freertos::FreertosHeap4::heapADD_WILL_OVERFLOW(xWantedSize, xAdditionalRequiredSize) == 0)
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
        if (freertos::FreertosHeap4::heapBLOCK_SIZE_IS_VALID(xWantedSize) != 0)
        {
            if ((xWantedSize > 0) && (xWantedSize <= xFreeBytesRemaining))
            {
                /* Traverse the list from the start (lowest address) block until
                 * one of adequate size is found. */
                pxPreviousBlock = &xStart;
                pxBlock = xStart.pxNextFreeBlock;

                while ((pxBlock->xBlockSize < xWantedSize) && (pxBlock->pxNextFreeBlock != NULL))
                {
                    pxPreviousBlock = pxBlock;
                    pxBlock = pxBlock->pxNextFreeBlock;
                }

                /* If the end marker was reached then a block of adequate size
                 * was not found. */
                if (pxBlock != pxEnd)
                {
                    /* Return the memory space pointed to - jumping over the
                     * BlockLink_t structure at its start. */
                    pvReturn = (void *)(((uint8_t *)pxPreviousBlock->pxNextFreeBlock) + freertos::FreertosHeap4::heap_struct_size);

                    /* This block is being returned for use so must be taken out
                     * of the list of free blocks. */
                    pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                    /* If the block is larger than required it can be split into
                     * two. */
                    if ((pxBlock->xBlockSize - xWantedSize) > freertos::FreertosHeap4::heap_minimum_block_size)
                    {
                        /* This block is to be split into two.  Create a new
                         * block following the number of bytes requested. The void
                         * cast is used to prevent byte alignment warnings from the
                         * compiler. */
                        pxNewBlockLink = (freertos::BlockLink_t *)(((uint8_t *)pxBlock) + xWantedSize);
                        configASSERT((((size_t)pxNewBlockLink) & portBYTE_ALIGNMENT_MASK) == 0);

                        /* Calculate the sizes of two blocks split from the
                         * single block. */
                        pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                        pxBlock->xBlockSize = xWantedSize;

                        /* Insert the new block into the list of free blocks. */
                        prvInsertBlockIntoFreeList(pxNewBlockLink);
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    xFreeBytesRemaining -= pxBlock->xBlockSize;

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
                    heapALLOCATE_BLOCK(pxBlock);
                    pxBlock->pxNextFreeBlock = NULL;
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

void freertos::FreertosHeap4::Free(void *pv)
{
    uint8_t *puc = (uint8_t *)pv;
    freertos::BlockLink_t *pxLink;

    if (pv != NULL)
    {
        /* The memory being freed will have an BlockLink_t structure immediately
         * before it. */
        puc -= freertos::FreertosHeap4::heap_struct_size;

        /* This casting is to keep the compiler from issuing warnings. */
        pxLink = (freertos::BlockLink_t *)puc;

        configASSERT(heapBLOCK_IS_ALLOCATED(pxLink) != 0);
        configASSERT(pxLink->pxNextFreeBlock == NULL);

        if (heapBLOCK_IS_ALLOCATED(pxLink) != 0)
        {
            if (pxLink->pxNextFreeBlock == NULL)
            {
                /* The block is being returned to the heap - it is no longer
                 * allocated. */
                heapFREE_BLOCK(pxLink);

#if (configHEAP_CLEAR_MEMORY_ON_FREE == 1)
                {
                    (void)memset(puc + heap_struct_size, 0, pxLink->xBlockSize - heap_struct_size);
                }
#endif

                vTaskSuspendAll();
                {
                    /* Add this block to the list of free blocks. */
                    xFreeBytesRemaining += pxLink->xBlockSize;
                    traceFREE(pv, pxLink->xBlockSize);
                    prvInsertBlockIntoFreeList(((freertos::BlockLink_t *)pxLink));
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

void *freertos::FreertosHeap4::Calloc(size_t xNum, size_t xSize)
{
    void *pv = NULL;

    if (freertos::FreertosHeap4::heapMULTIPLY_WILL_OVERFLOW(xNum, xSize) == 0)
    {
        pv = pvPortMalloc(xNum * xSize);

        if (pv != NULL)
        {
            (void)memset(pv, 0, xNum * xSize);
        }
    }

    return pv;
}

void freertos::FreertosHeap4::GetHeapStats(HeapStats_t *pxHeapStats)
{
    freertos::BlockLink_t *pxBlock;
    size_t xBlocks = 0, xMaxSize = 0, xMinSize = portMAX_DELAY; /* portMAX_DELAY used as a portable way of getting the maximum value. */

    vTaskSuspendAll();
    {
        pxBlock = xStart.pxNextFreeBlock;

        /* pxBlock will be NULL if the heap has not been initialised.  The heap
         * is initialised automatically when the first allocation is made. */
        if (pxBlock != NULL)
        {
            while (pxBlock != pxEnd)
            {
                /* Increment the number of blocks and record the largest block seen
                 * so far. */
                xBlocks++;

                if (pxBlock->xBlockSize > xMaxSize)
                {
                    xMaxSize = pxBlock->xBlockSize;
                }

                if (pxBlock->xBlockSize < xMinSize)
                {
                    xMinSize = pxBlock->xBlockSize;
                }

                /* Move to the next block in the chain until the last block is
                 * reached. */
                pxBlock = pxBlock->pxNextFreeBlock;
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

namespace
{
/* Allocate the memory for the heap. */
#if (configAPPLICATION_ALLOCATED_HEAP == 1)

    /* The application writer has already defined the array used for the RTOS
     * heap - probably so it can be placed in a special segment or address. */
    extern uint8_t ucHeap[configTOTAL_HEAP_SIZE];
#else
    PRIVILEGED_DATA static uint8_t ucHeap[configTOTAL_HEAP_SIZE];
#endif /* configAPPLICATION_ALLOCATED_HEAP */

    freertos::FreertosHeap4 _heap4{ucHeap, sizeof(ucHeap)};

} // namespace

extern "C"
{
    void *pvPortMalloc(size_t xWantedSize)
    {
        return _heap4.Malloc(xWantedSize);
    }

    void vPortFree(void *pv)
    {
        _heap4.Free(pv);
    }

    size_t xPortGetFreeHeapSize(void)
    {
        return _heap4.xFreeBytesRemaining;
    }

    size_t xPortGetMinimumEverFreeHeapSize(void)
    {
        return _heap4.xMinimumEverFreeBytesRemaining;
    }

    void vPortInitialiseBlocks(void)
    {
        /* This just exists to keep the linker quiet. */
    }

    void *pvPortCalloc(size_t xNum, size_t xSize)
    {
        return _heap4.Calloc(xNum, xSize);
    }

    /*-----------------------------------------------------------*/

    void vPortGetHeapStats(HeapStats_t *pxHeapStats)
    {
        return _heap4.GetHeapStats(pxHeapStats);
    }
}
