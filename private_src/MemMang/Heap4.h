#pragma once
#include "base/bit/bit.h"
#include "base/embedded/heap/FreeRtosHeap4.h"
#include "base/embedded/heap/IHeap.h"
#include "base/embedded/heap/MemoryBlockLinkListNode.h"
#include <cstddef>

struct xHeapStats;

namespace freertos
{
	/// @brief 基于 freertos 的 heap4 的堆管理器。
	class Heap4 :
		public base::heap::IHeap
	{
	private:
#pragma region constexpr
		/* Max value that fits in a size_t type. */
		static size_t constexpr _heap_size_max = ~((size_t)0);

		/* Check if multiplying a and b will result in overflow. */
		bool constexpr HeapMultiplyWillOverflow(size_t a, size_t b)
		{
			return ((a) > 0) && ((b) > (_heap_size_max / (a)));
		}

		/* Check if adding a and b will result in overflow. */
		bool constexpr HeapAddWillOverflow(size_t a, size_t b)
		{
			return (a) > (_heap_size_max - (b));
		}

		/* MSB of the _size member of an MemoryBlockLinkListNode structure is used to track
		 * the allocation status of a block.  When MSB of the _size member of
		 * an MemoryBlockLinkListNode structure is set then the block belongs to the application.
		 * When the bit is free the block is still part of the free heap space. */
		static size_t constexpr _heap_block_allocated_bitmask = static_cast<size_t>(base::bit::Bit((sizeof(size_t) * 8) - 1));

		bool constexpr HeapBlockSizeIsValid(size_t _size)
		{
			return ((_size)&_heap_block_allocated_bitmask) == 0;
		}

		bool constexpr HeapBlockIsAllocated(base::heap::MemoryBlockLinkListNode *pxBlock)
		{
			return ((pxBlock->_size) & _heap_block_allocated_bitmask) != 0;
		}

		void constexpr HeapAllocateBlock(base::heap::MemoryBlockLinkListNode *pxBlock)
		{
			(pxBlock->_size) |= _heap_block_allocated_bitmask;
		}

		void constexpr HeapFreeBlock(base::heap::MemoryBlockLinkListNode *pxBlock)
		{
			(pxBlock->_size) &= ~_heap_block_allocated_bitmask;
		}

#pragma endregion

		uint8_t *_buffer{};
		size_t _size{};

		/// @brief 将被释放的内存插入链表。
		/// @note 如果发现与链表中要插入位置的前一个节点和后一个节点指向的内存是连续的，会合并这些节点。
		/// @param pxBlockToInsert
		void InsertBlockIntoFreeList(base::heap::MemoryBlockLinkListNode *pxBlockToInsert);

	public:
		/// @brief 构造一个基于 freertos 的 heap4 的堆管理器。
		/// @param buffer 要被作为堆的缓冲区。
		/// @param size 缓冲区大小。
		Heap4(uint8_t *buffer, size_t size);

		/* Create a couple of list links to mark the start and end of the list. */
		base::heap::MemoryBlockLinkListNode _head_element;
		base::heap::MemoryBlockLinkListNode *_tail_element = nullptr;

		/* Keeps track of the number of calls to allocate and free memory as well as the
		 * number of free bytes remaining, but says nothing about fragmentation. */
		size_t xFreeBytesRemaining = 0U;
		size_t xMinimumEverFreeBytesRemaining = 0U;
		size_t xNumberOfSuccessfulAllocations = 0;
		size_t xNumberOfSuccessfulFrees = 0;

		/// @brief 分配内存。
		/// @param size 要分配的内存块大小。单位：字节。
		/// @return
		virtual void *Malloc(size_t xWantedSize) override;

		/// @brief 要释放的由 Malloc 方法分配的内存块。
		/// @param p Malloc 方法返回的指针。
		virtual void Free(void *pv) override;

		/// @brief 堆的起点。
		/// @return
		virtual uint8_t const *begin() const override;

		/// @brief 堆的最后一个字节再 +1.
		/// @return
		virtual uint8_t const *end() const override;

		/// @brief 剩余的自由空间。单位：字节。
		/// @note 不一定是一整块连续的，有可能是碎片化的。
		/// @return
		virtual size_t RemainingFreeSize() const override;
	};
} // namespace freertos
