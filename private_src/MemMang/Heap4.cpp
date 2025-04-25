/*
 * A sample implementation of pvPortMalloc() and vPortFree() that combines
 * (coalescences) adjacent memory blocks as they are freed, and in so doing
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the
 * memory management pages of https://www.FreeRTOS.org for more information.
 */
#include "Heap4.h"
#include "base/bit/bit.h"
#include "base/embedded/heap/MemoryBlockLinkListNode.h"
#include "base/task/task.h"
#include "FreeRTOS.h"
#include "task.h"
#include <cstddef>
#include <cstdint>
#include <stdlib.h>
#include <string.h>

void freertos::Heap4::InsertBlockIntoFreeList(base::heap::MemoryBlockLinkListNode *pxBlockToInsert)
{
	base::heap::MemoryBlockLinkListNode *pxIterator{};
	uint8_t *puc{};

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
}

freertos::Heap4::Heap4(uint8_t *buffer, size_t size)
{
	_buffer = buffer;
	_size = size;

	/* Ensure the heap starts on a correctly aligned boundary. */
	_buffer = base::bit::AlignUp(buffer, 8);
	_size = size - static_cast<size_t>(_buffer - buffer);

	/* _head_element is used to hold a pointer to the first item in the list of free
	 * blocks.  The void cast is used to prevent compiler warnings. */
	_head_element._next_free_block = reinterpret_cast<base::heap::MemoryBlockLinkListNode *>(_buffer);
	_head_element._size = 0;

	/* _tail_element is used to mark the end of the list of free blocks and is inserted
	 * at the end of the heap space. */
	uint8_t *tail_addr = base::bit::AlignDown(_buffer + _size - base::bit::GetAlignedSize<base::heap::MemoryBlockLinkListNode>());
	_tail_element = reinterpret_cast<base::heap::MemoryBlockLinkListNode *>(tail_addr);
	_tail_element->_next_free_block = nullptr;
	_tail_element->_size = 0;

	/* To start with there is a single free block that is sized to take up the
	 * entire heap space, minus the space taken by _tail_element. */
	base::heap::MemoryBlockLinkListNode *pxFirstFreeBlock = reinterpret_cast<base::heap::MemoryBlockLinkListNode *>(_buffer);
	pxFirstFreeBlock->_next_free_block = _tail_element;
	pxFirstFreeBlock->_size = static_cast<size_t>(tail_addr - _buffer);

	/* Only one block exists - and it covers the entire usable heap space. */
	xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->_size;
	xFreeBytesRemaining = pxFirstFreeBlock->_size;
}

void *freertos::Heap4::Malloc(size_t xWantedSize)
{
	base::heap::MemoryBlockLinkListNode *pxBlock;
	base::heap::MemoryBlockLinkListNode *pxPreviousBlock;
	base::heap::MemoryBlockLinkListNode *pxNewBlockLink;
	void *pvReturn = nullptr;

	vTaskSuspendAll();
	{
		/* If this is the first call to malloc then the heap will require
		 * initialisation to setup the list of free blocks. */
		if (_tail_element == nullptr)
		{
			// 已被构造函数取代。
			//  prvHeapInit();
		}

		if (xWantedSize > 0)
		{
			/* The wanted size must be increased so it can contain a MemoryBlockLinkListNode
			 * structure in addition to the requested amount of bytes. Some
			 * additional increment may also be needed for alignment. */
			size_t xAdditionalRequiredSize = base::bit::GetAlignedSize<base::heap::MemoryBlockLinkListNode>() +
											 portBYTE_ALIGNMENT -
											 (xWantedSize & portBYTE_ALIGNMENT_MASK);

			if (!base::heap::HeapAddWillOverflow(xWantedSize, xAdditionalRequiredSize))
			{
				xWantedSize += xAdditionalRequiredSize;
			}
			else
			{
				xWantedSize = 0;
			}
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

				while ((pxBlock->_size < xWantedSize) && (pxBlock->_next_free_block != nullptr))
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
					pvReturn = (void *)(((uint8_t *)pxPreviousBlock->_next_free_block) + base::bit::GetAlignedSize<base::heap::MemoryBlockLinkListNode>());

					/* This block is being returned for use so must be taken out
					 * of the list of free blocks. */
					pxPreviousBlock->_next_free_block = pxBlock->_next_free_block;

					/* If the block is larger than required it can be split into
					 * two. */
					if ((pxBlock->_size - xWantedSize) > base::heap::MinimumBlockSize())
					{
						/* This block is to be split into two.  Create a new
						 * block following the number of bytes requested. The void
						 * cast is used to prevent byte alignment warnings from the
						 * compiler. */
						pxNewBlockLink = (base::heap::MemoryBlockLinkListNode *)(((uint8_t *)pxBlock) + xWantedSize);

						/* Calculate the sizes of two blocks split from the
						 * single block. */
						pxNewBlockLink->_size = pxBlock->_size - xWantedSize;
						pxBlock->_size = xWantedSize;

						/* Insert the new block into the list of free blocks. */
						InsertBlockIntoFreeList(pxNewBlockLink);
					}

					xFreeBytesRemaining -= pxBlock->_size;

					if (xFreeBytesRemaining < xMinimumEverFreeBytesRemaining)
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}

					/* The block is being returned - it is allocated and owned
					 * by the application and has no "next" block. */
					HeapAllocateBlock(pxBlock);
					pxBlock->_next_free_block = nullptr;
					xNumberOfSuccessfulAllocations++;
				}
			}
		}
	}
	(void)xTaskResumeAll();

	return pvReturn;
}

void freertos::Heap4::Free(void *pv)
{
	uint8_t *puc = (uint8_t *)pv;
	base::heap::MemoryBlockLinkListNode *pxLink;

	if (pv != nullptr)
	{
		/* The memory being freed will have an MemoryBlockLinkListNode structure immediately
		 * before it. */
		puc -= base::bit::GetAlignedSize<base::heap::MemoryBlockLinkListNode>();

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = (base::heap::MemoryBlockLinkListNode *)puc;

		if (HeapBlockIsAllocated(pxLink))
		{
			if (pxLink->_next_free_block == nullptr)
			{
				/* The block is being returned to the heap - it is no longer
				 * allocated. */
				HeapFreeBlock(pxLink);

				vTaskSuspendAll();
				{
					/* Add this block to the list of free blocks. */
					xFreeBytesRemaining += pxLink->_size;
					InsertBlockIntoFreeList(((base::heap::MemoryBlockLinkListNode *)pxLink));
					xNumberOfSuccessfulFrees++;
				}
				(void)xTaskResumeAll();
			}
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
