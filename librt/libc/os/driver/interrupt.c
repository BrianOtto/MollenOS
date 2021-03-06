/* MollenOS
 *
 * Copyright 2011 - 2017, Philip Meulengracht
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
 * MollenOS MCore - Interrupt Support Definitions & Structures
 * - This header describes the base interrupt-structures, prototypes
 *   and functionality, refer to the individual things for descriptions
 */

#include <os/driver.h>
#include <os/syscall.h>

/* RegisterFastInterruptHandler
 * Registers a fast interrupt handler associated with the interrupt. */
void
RegisterFastInterruptHandler(
    _In_ DeviceInterrupt_t* Interrupt,
    _In_ InterruptHandler_t Handler)
{
    Interrupt->FastInterrupt.Handler = Handler;
}

/* RegisterFastInterruptIoResource
 * Registers the given device io resource with the fast-interrupt. */
void
RegisterFastInterruptIoResource(
    _In_ DeviceInterrupt_t* Interrupt,
    _In_ DeviceIo_t*        IoSpace)
{
    for (int i = 0; i < INTERRUPT_MAX_IO_RESOURCES; i++) {
        if (Interrupt->FastInterrupt.IoResources[i] == NULL) {
            Interrupt->FastInterrupt.IoResources[i] = IoSpace;
            break;
        }
    }
}

/* RegisterFastInterruptMemoryResource
 * Registers the given memory resource with the fast-interrupt. */
void
RegisterFastInterruptMemoryResource(
    _In_ DeviceInterrupt_t* Interrupt,
    _In_ uintptr_t          Address,
    _In_ size_t             Length,
    _In_ Flags_t            Flags)
{
    for (int i = 0; i < INTERRUPT_MAX_MEMORY_RESOURCES; i++) {
        if (Interrupt->FastInterrupt.MemoryResources[i].Address == 0) {
            Interrupt->FastInterrupt.MemoryResources[i].Address = Address;
            Interrupt->FastInterrupt.MemoryResources[i].Length  = Length;
            Interrupt->FastInterrupt.MemoryResources[i].Flags   = Flags;
            break;
        }
    }
}

/* RegisterInterruptContext
 * Sets the context pointer that should be attached to any OnInterrupt events. */
void
RegisterInterruptContext(
    _In_ DeviceInterrupt_t* Interrupt,
    _In_ void*              Context)
{
    Interrupt->Context = Context;
}

/* RegisterInterruptSource 
 * Allocates the given interrupt source for use by
 * the requesting driver, an id for the interrupt source
 * is returned. After a succesful register, OnInterrupt
 * can be called by the event-system */
UUId_t
RegisterInterruptSource(
    _In_ DeviceInterrupt_t* Interrupt,
    _In_ Flags_t            Flags)
{
	// Sanitize input
	if (Interrupt == NULL) {
		return UUID_INVALID;
	}
	return Syscall_InterruptAdd(Interrupt, Flags);
}

/* UnregisterInterruptSource 
 * Unallocates the given interrupt source and disables
 * all events of OnInterrupt */
OsStatus_t
UnregisterInterruptSource(
    _In_ UUId_t             Source)
{
	// Sanitize input
	if (Source == UUID_INVALID) {
		return OsError;
	}
	return Syscall_InterruptRemove(Source);
}
