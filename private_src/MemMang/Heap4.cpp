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

namespace
{
	/* The size of the structure placed at the beginning of each allocated memory
	 * block must by correctly byte aligned. */
	size_t constexpr _size_of_heap_block_linklist_element = (sizeof(base::heap::MemoryBlockLinkListNode) + ((size_t)(portBYTE_ALIGNMENT - 1))) & ~((size_t)portBYTE_ALIGNMENT_MASK);

	/// @brief 堆块的最小大小。
	/// @note 将 _size_of_heap_block_linklist_element 左移 1 位，即乘 2，说明最小大小定为
	/// _size_of_heap_block_linklist_element 的 2 倍。这可能是因为如果块太小了，链表的空间利用率
	/// 很低，链表节点本身占用的内存都比指向的内存块大了，那这块内存就不值得用一个链表节点去指向它。
	size_t constexpr _heap_minimum_block_size = ((size_t)(_size_of_heap_block_linklist_element << 1));

} // namespace

void freertos::Heap4::InsertBlockIntoFreeList(base::heap::MemoryBlockLinkListNode *pxBlockToInsert)
{
	base::heap::MemoryBlockLinkListNode *pxIterator;
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
	_buffer = buffer;
	_size = size;

	base::heap::MemoryBlockLinkListNode *pxFirstFreeBlock;
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
	_head_element._next_free_block = (base::heap::MemoryBlockLinkListNode *)pucAlignedHeap;
	_head_element._size = (size_t)0;

	/* _tail_element is used to mark the end of the list of free blocks and is inserted
	 * at the end of the heap space. */
	uxAddress = ((portPOINTER_SIZE_TYPE)pucAlignedHeap) + xTotalHeapSize;
	uxAddress -= _size_of_heap_block_linklist_element;
	uxAddress &= ~((portPOINTER_SIZE_TYPE)portBYTE_ALIGNMENT_MASK);
	_tail_element = (base::heap::MemoryBlockLinkListNode *)uxAddress;
	_tail_element->_size = 0;
	_tail_element->_next_free_block = NULL;

	/* To start with there is a single free block that is sized to take up the
	 * entire heap space, minus the space taken by _tail_element. */
	pxFirstFreeBlock = (base::heap::MemoryBlockLinkListNode *)pucAlignedHeap;
	pxFirstFreeBlock->_size = (size_t)(uxAddress - (portPOINTER_SIZE_TYPE)pxFirstFreeBlock);
	pxFirstFreeBlock->_next_free_block = _tail_element;

	/* Only one block exists - and it covers the entire usable heap space. */
	xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->_size;
	xFreeBytesRemaining = pxFirstFreeBlock->_size;
}

void *freertos::Heap4::Malloc(size_t xWantedSize)
{
	base::heap::MemoryBlockLinkListNode *pxBlock;
	base::heap::MemoryBlockLinkListNode *pxPreviousBlock;
	base::heap::MemoryBlockLinkListNode *pxNewBlockLink;
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
			/* The wanted size must be increased so it can contain a MemoryBlockLinkListNode
			 * structure in addition to the requested amount of bytes. Some
			 * additional increment may also be needed for alignment. */
			xAdditionalRequiredSize = _size_of_heap_block_linklist_element + portBYTE_ALIGNMENT - (xWantedSize & portBYTE_ALIGNMENT_MASK);

			if (!base::heap::HeapAddWillOverflow(xWantedSize, xAdditionalRequiredSize))
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
		 * top bit is set.  The top bit of the block size member of the MemoryBlockLinkListNode
		 * structure is used to determine who owns the block - the application or
		 * the kernel, so it must be free. */
		if (base::heap::HeapBlockSizeIsValid(xWantedSize))
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
					 * MemoryBlockLinkListNode structure at its start. */
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
						pxNewBlockLink = (base::heap::MemoryBlockLinkListNode *)(((uint8_t *)pxBlock) + xWantedSize);
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
	base::heap::MemoryBlockLinkListNode *pxLink;

	if (pv != NULL)
	{
		/* The memory being freed will have an MemoryBlockLinkListNode structure immediately
		 * before it. */
		puc -= _size_of_heap_block_linklist_element;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = (base::heap::MemoryBlockLinkListNode *)puc;

		configASSERT(HeapBlockIsAllocated(pxLink));
		configASSERT(pxLink->_next_free_block == NULL);

		if (HeapBlockIsAllocated(pxLink))
		{
			if (pxLink->_next_free_block == NULL)
			{
				/* The block is being returned to the heap - it is no longer
				 * allocated. */
				HeapFreeBlock(pxLink);

				vTaskSuspendAll();
				{
					/* Add this block to the list of free blocks. */
					xFreeBytesRemaining += pxLink->_size;
					traceFREE(pv, pxLink->_size);
					InsertBlockIntoFreeList(((base::heap::MemoryBlockLinkListNode *)pxLink));
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

uint8_t const *freertos::Heap4::begin() const
{
	return _buffer;
}

uint8_t const *freertos::Heap4::end() const
{
	return _buffer + _size;
}

size_t freertos::Heap4::RemainingFreeSize() const
{
	return xFreeBytesRemaining;
}
