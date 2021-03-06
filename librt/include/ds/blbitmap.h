/* MollenOS
 *
 * Copyright 2017, Philip Meulengracht
 *
 * This program is free software : you can redistribute it and / or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation ? , either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * MollenOS MCore - Generic Block Bitmap Implementation
 */

#ifndef __BLOCK_BITMAP__
#define __BLOCK_BITMAP__

#include <os/osdefs.h>
#include <ds/ds.h>
#include <ds/bitmap.h>

#define BLOCKMAP_ALLRESERVED            (1 << 0) // Set all to allocated initially

/* BlockBitmap_t
 * A specialized bitmap that returns memory blocks instead of frames.
 * This way it can be used for keeping track of memory regions in <BlockSize> */
typedef struct _BlockBitmap {
    Bitmap_t            Base;
    SafeMemoryLock_t    SyncObject;
   
    // Block Information
    uintptr_t           BlockStart;
    uintptr_t           BlockEnd;
    size_t              BlockSize;
    size_t              BlockCount;

    // Statistics
    size_t              BlocksAllocated;
    size_t              NumAllocations;
    size_t              NumFrees;
} BlockBitmap_t;

typedef struct _BlockBitmapSegment {
    BlockBitmap_t*      Source;
    uintptr_t           SegmentStart;
    uintptr_t           SegmentSize;
} BlockBitmapSegment_t;

/* CreateBlockmap
 * Creates a new blockmap of with the given configuration and returns a pointer to a newly
 * allocated blockmap. Also returns an error code. */
CRTDECL(
OsStatus_t,
CreateBlockmap(
    _In_  Flags_t           Configuration,
    _In_  uintptr_t         BlockStart, 
    _In_  uintptr_t         BlockEnd, 
    _In_  size_t            BlockSize,
    _Out_ BlockBitmap_t**   Blockmap));

/* ConstructBlockmap
 * Instantiates a static instance of a block bitmap. The buffer used for the bit storage
 * must also be provided and should be of at-least GetBytesNeccessaryForBlockmap(<Params>). */
CRTDECL(
OsStatus_t,
ConstructBlockmap(
    _In_ BlockBitmap_t*     Blockmap,
    _In_ void*              Buffer,
    _In_ Flags_t            Configuration,
    _In_ uintptr_t          BlockStart, 
    _In_ uintptr_t          BlockEnd, 
    _In_ size_t             BlockSize));

/* DestroyBlockmap
 * Destroys a block bitmap, and releases all resources associated with the bitmap */
CRTDECL(
OsStatus_t,
DestroyBlockmap(
    _In_ BlockBitmap_t* Blockmap));

/* GetBytesNeccessaryForBlockmap
 * Calculates the number of bytes neccessary for the allocation parameters. */
CRTDECL(
size_t,
GetBytesNeccessaryForBlockmap(
    _In_ uintptr_t          BlockStart, 
    _In_ uintptr_t          BlockEnd, 
    _In_ size_t             BlockSize));

/* AllocateBlocksInBlockmap
 * Allocates a number of bytes in the bitmap (rounded up in blocks)
 * and returns the calculated block of the start of block allocated (continously) */
CRTDECL(
uintptr_t,
AllocateBlocksInBlockmap(
    _In_ BlockBitmap_t* Blockmap,
    _In_ size_t         AllocationMask,
    _In_ size_t         Size));

/* ReserveBlockmapRegion
 * Reserves a region of the blockmap. This sets the given region to allocated. The
 * region and size must be within boundaries of the blockmap. */
CRTDECL(
OsStatus_t,
ReserveBlockmapRegion(
    _In_ BlockBitmap_t* Blockmap,
    _In_ uintptr_t      Block,
    _In_ size_t         Size));

/* ReleaseBlockmapRegion
 * Deallocates a given block translated into offsets 
 * into the given bitmap, and frees them in the bitmap */
CRTDECL(
OsStatus_t,
ReleaseBlockmapRegion(
    _In_ BlockBitmap_t* Blockmap,
    _In_ uintptr_t      Block,
    _In_ size_t         Size));

/* BlockBitmapValidate
 * Validates the given block that it's within
 * range of our bitmap and that it is either set or clear */
CRTDECL(
OsStatus_t,
BlockBitmapValidateState(
    _In_ BlockBitmap_t* Blockmap,
    _In_ uintptr_t      Block,
    _In_ int            Set));

#endif //!__BLOCK_BITMAP__
