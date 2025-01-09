#pragma once
#include <stddef.h>

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

    class Heap4
    {
    private:
        /*
         * Inserts a block of memory that is being freed into the correct position in
         * the list of free memory blocks.  The block being freed will be merged with
         * the block in front it and/or the block behind it if the memory blocks are
         * adjacent to each other.
         */
        void InsertBlockIntoFreeList(freertos::BlockLink_t *pxBlockToInsert);

    public:
        Heap4();

        /* Create a couple of list links to mark the start and end of the list. */
        BlockLink_t xStart;
        BlockLink_t *pxEnd = NULL;

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

} // namespace freertos
