/* MollenOS
 *
 * Copyright 2011, Philip Meulengracht
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
 * MollenOS - Generic Data Structures (Shared)
 */

#ifndef _DATASTRUCTURES_H_
#define _DATASTRUCTURES_H_

#include <os/osdefs.h>

/* The definition of a key
 * in generic data-structures this can be values or data */
typedef struct _DataKey {
    union {
        int     Integer;
        UUId_t  Id;
        struct {
            const char* Pointer;
            size_t      Length;
        } String;
    } Value;
} DataKey_t;

/* This enumeration denotes
 * the kind of key that is to be interpreted by the data-structure */
typedef enum _KeyType {
    KeyInteger,
    KeyId,
    KeyString
} KeyType_t;

/* SafeMemoryLock_t
 * Custom implementation that is available for the different data-structures in
 * the libds. */
typedef struct _SafeMemoryLock {
    atomic_bool     SyncObject;
    unsigned        Flags;
} SafeMemoryLock_t;

/* dsalloc
 * Seperate portable memory allocator for data-structures */
CRTDECL(void*,
dsalloc(
    _In_ size_t size));

/* dsfree
 * Seperate portable memory freeing for data-structures */
CRTDECL(void,
dsfree(
    _In_ void *p));

/* dslock
 * Acquires the lock given, this is a blocking call and will wait untill
 * the lock is acquired. */
CRTDECL(void,
dslock(
    _In_ SafeMemoryLock_t* MemoryLock));

/* dsunlock
 * Releases the lock given and restores any previous flags. */
CRTDECL(void,
dsunlock(
    _In_ SafeMemoryLock_t* MemoryLock));

/* Helper Function 
 * Matches two keys based on the key type returns 0 if they are equal, or -1 if not */
CRTDECL(int,
dsmatchkey(
    _In_ KeyType_t KeyType, 
    _In_ DataKey_t Key1, 
    _In_ DataKey_t Key2));

/* Helper Function
 * Used by sorting, it compares to values and returns 
 *  - 1 if 1 > 2, 
 *  - 0 if 1 == 2 and
 *  - -1 if 2 > 1 */
CRTDECL(int,
dssortkey(
    _In_ KeyType_t KeyType, 
    _In_ DataKey_t Key1, 
    _In_ DataKey_t Key2));

#endif //!_DATASTRUCTURES_H_
