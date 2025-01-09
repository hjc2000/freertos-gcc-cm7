#pragma once
#include <stddef.h>

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
    };

} // namespace freertos
