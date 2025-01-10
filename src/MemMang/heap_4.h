#pragma once
#include <stddef.h>
#include <stdint.h>

struct xHeapStats;

namespace freertos
{
    /// @brief 空闲内存块链表节点。
    struct BlockLink_t
    {
        /// @brief 下一个空闲内存块。
        BlockLink_t *_next_free_block{};

        /// @brief 空闲内存块大小。
        size_t _size{};
    };

    /// @brief 基于 freertos 的 heap4 的堆管理器。
    class Heap4
    {
    private:
#pragma region 常量
        /// @brief 假设 1 个字节有 8 位。
        static size_t constexpr _bit_count_per_byte = (size_t)8;

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

        /* MSB of the _size member of an BlockLink_t structure is used to track
         * the allocation status of a block.  When MSB of the _size member of
         * an BlockLink_t structure is set then the block belongs to the application.
         * When the bit is free the block is still part of the free heap space. */
        static size_t constexpr _heap_block_allocated_bitmask = ((size_t)1) << ((sizeof(size_t) * _bit_count_per_byte) - 1);

        bool constexpr HeapBlockSizeIsValid(size_t _size)
        {
            return ((_size)&_heap_block_allocated_bitmask) == 0;
        }

        bool constexpr HeapBlockIsAllocated(freertos::BlockLink_t *pxBlock)
        {
            return ((pxBlock->_size) & _heap_block_allocated_bitmask) != 0;
        }

        void constexpr HeapAllocateBlock(freertos::BlockLink_t *pxBlock)
        {
            (pxBlock->_size) |= _heap_block_allocated_bitmask;
        }

        void constexpr HeapFreeBlock(freertos::BlockLink_t *pxBlock)
        {
            (pxBlock->_size) &= ~_heap_block_allocated_bitmask;
        }

#pragma endregion

        /// @brief 将被释放的内存插入链表。
        /// @note 如果发现与链表中要插入位置的前一个节点和后一个节点指向的内存是连续的，会合并这些节点。
        /// @param pxBlockToInsert
        void InsertBlockIntoFreeList(freertos::BlockLink_t *pxBlockToInsert);

    public:
        /// @brief 构造一个基于 freertos 的 heap4 的堆管理器。
        /// @param buffer 要被作为堆的缓冲区。
        /// @param size 缓冲区大小。
        Heap4(uint8_t *buffer, size_t size);

        /* Create a couple of list links to mark the start and end of the list. */
        BlockLink_t _head_element;
        BlockLink_t *_tail_element = nullptr;

        /* Keeps track of the number of calls to allocate and free memory as well as the
         * number of free bytes remaining, but says nothing about fragmentation. */
        size_t xFreeBytesRemaining = 0U;
        size_t xMinimumEverFreeBytesRemaining = 0U;
        size_t xNumberOfSuccessfulAllocations = 0;
        size_t xNumberOfSuccessfulFrees = 0;

        void *Malloc(size_t xWantedSize);
        void Free(void *pv);
        void *Calloc(size_t xNum, size_t xSize);
        void GetHeapStats(xHeapStats *pxHeapStats);
    };

    /// @brief 全局的 Heap4 对象。
    extern freertos::Heap4 _heap4;
} // namespace freertos
