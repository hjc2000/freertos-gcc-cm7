#pragma once
#include <stddef.h>

namespace freertos
{
    class FreertosHeap4
    {
    public:
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
