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
 * MollenOS Interrupt Interface
 * - Contains the shared kernel interrupt interface
 *   that all sub-layers must conform to
 */

#ifndef _MCORE_INTERRUPTS_H_
#define _MCORE_INTERRUPTS_H_

#include <os/interrupt.h>
#include <os/driver.h>
#include <os/ipc/ipc.h>
#include <os/osdefs.h>
#include <os/context.h>

/* Special flags that are available only
 * in kernel context for special interrupts */
#define INTERRUPT_KERNEL                0x10000000
#define INTERRUPT_CONTEXT               0x20000000

/* SystemInterrupt
 * The kernel interrupt descriptor structure. Contains
 * all information neccessary to store registered interrupts. */
typedef struct _SystemInterrupt {
    DeviceInterrupt_t               Interrupt;
    FastInterruptResourceTable_t    KernelResources;
    UUId_t                          Id;
    UUId_t                          ProcessHandle;
    UUId_t                          Thread;
    Flags_t                         Flags;
    int                             Source;
    struct _SystemInterrupt*        Link;
} SystemInterrupt_t;

/* InitializeInterruptTable
 * Initializes the static system interrupt table. This must be done before any driver interrupts
 * as they will rely on the system function table that gets passed along. */
KERNELAPI void KERNELABI
InitializeInterruptTable(void);

/* FastInterruptResources_t
 * Retrieves the system fast interrupt resource table to pass to process interrupt handlers. */
KERNELAPI FastInterruptResources_t* KERNELABI
GetFastInterruptTable(void);

/* InterruptRegister
 * Tries to allocate the given interrupt source by the given descriptor and flags. On success
 * it returns the id of the irq, and on failure it returns UUID_INVALID */
KERNELAPI UUId_t KERNELABI
InterruptRegister(
    _In_ DeviceInterrupt_t* Interrupt,
    _In_ Flags_t            Flags);

/* InterruptUnregister 
 * Unregisters the interrupt from the system and removes any resources that was associated 
 * with that interrupt also masks the interrupt if it was the only user */
KERNELAPI OsStatus_t KERNELABI
InterruptUnregister(
    _In_ UUId_t             Source);

/* InterruptGet
 * Retrieves the given interrupt source information as a SystemInterrupt_t */
KERNELAPI SystemInterrupt_t* KERNELABI
InterruptGet(
   _In_ UUId_t              Source);

/* InterruptGetIndex
 * Retrieves the given interrupt source information as a SystemInterrupt_t */
KERNELAPI SystemInterrupt_t* KERNELABI
InterruptGetIndex(
   _In_ UUId_t              TableIndex);

/* InterruptSetActiveStatus
 * Set's the current status for the calling cpu to interrupt-active state */
KERNELAPI void KERNELABI
InterruptSetActiveStatus(
    _In_ int                Active);

/* InterruptGetActiveStatus
 * Get's the current status for the calling cpu to interrupt-active state */
KERNELAPI int KERNELABI
InterruptGetActiveStatus(void);

/* InterruptHandle
 * Handles an interrupt by invoking the registered handlers
 * on the given table-index. */
KERNELAPI InterruptStatus_t KERNELABI
InterruptHandle(
    _In_  Context_t*        Context,
    _In_  int               TableIndex,
    _Out_ int*              Source);

/* InterruptIncreasePenalty 
 * Increases the penalty for an interrupt source. This affects how the system allocates
 * interrupts when load balancing */
KERNELAPI OsStatus_t KERNELABI
InterruptIncreasePenalty(
    _In_ int                Source);

/* InterruptDecreasePenalty 
 * Decreases the penalty for an interrupt source. This affects how the system allocates
 * interrupts when load balancing */
KERNELAPI OsStatus_t KERNELABI
InterruptDecreasePenalty(
    _In_ int                Source);

/* InterruptGetPenalty
 * Retrieves the penalty for an interrupt source. If INTERRUPT_NONE is returned the source is unavailable. */
KERNELAPI int KERNELABI
InterruptGetPenalty(
    _In_ int                Source);

/* InterruptGetLeastLoaded
 * Allocates the least used sharable irq
 * most useful for MSI devices */
KERNELAPI int KERNELABI
InterruptGetLeastLoaded(
    _In_ int                Irqs[],
    _In_ int                Count);

/* AcpiGetPolarityMode
 * Returns whether or not the polarity is Active Low or Active High.
 * For Active Low = 1, Active High = 0 */
KERNELAPI int KERNELABI
AcpiGetPolarityMode(
    _In_ uint16_t           IntiFlags,
    _In_ int                Source);

/* AcpiGetTriggerMode
 * Returns whether or not the trigger mode of the interrup is level or edge.
 * For Level = 1, Edge = 0 */
KERNELAPI int KERNELABI
AcpiGetTriggerMode(
    _In_ uint16_t           IntiFlags,
    _In_ int                Source);

/* ConvertAcpiFlagsToConformFlags
 * Converts acpi interrupt flags to the system interrupt conform flags. */
KERNELAPI Flags_t KERNELABI
ConvertAcpiFlagsToConformFlags(
    _In_ uint16_t           IntiFlags,
    _In_ int                Source);

/* AcpiDeriveInterrupt
 * Derives an interrupt by consulting the bus of the device, and spits out flags in
 * AcpiConform and returns irq */
KERNELAPI int KERNELABI
AcpiDeriveInterrupt(
    _In_  DevInfo_t         Bus, 
    _In_  DevInfo_t         Device,
    _In_  int               Pin,
    _Out_ Flags_t*          AcpiConform);

/* __KernelInterruptDriver
 * Call this to send an interrupt into user-space
 * the driver must acknowledge the interrupt once its handled
 * to unmask the interrupt-line again */
__EXTERN OsStatus_t
ScRpcExecute(
    _In_ MRemoteCall_t*     Rpc,
    _In_ int                Async);

SERVICEAPI OsStatus_t SERVICEABI
__KernelInterruptDriver(
    _In_ UUId_t             Ash, 
    _In_ UUId_t             Id,
    _In_ void*              Data)
{
    MRemoteCall_t Request;
    size_t Zero = 0;

    RPCInitialize(&Request, Ash, 1, __DRIVER_INTERRUPT);
    RPCSetArgument(&Request, 0, (const void*)&Id, sizeof(UUId_t));
    RPCSetArgument(&Request, 1, (const void*)&Data, sizeof(void*));
    RPCSetArgument(&Request, 2, (const void*)&Zero, sizeof(size_t));
    RPCSetArgument(&Request, 3, (const void*)&Zero, sizeof(size_t));
    RPCSetArgument(&Request, 4, (const void*)&Zero, sizeof(size_t));
    return ScRpcExecute(&Request, 1);
}

/* __KernelTimeoutDriver
 * Call this to send an timeout into userspace. The driver is
 * then informed about a timer-interval that elapsed. */
SERVICEAPI OsStatus_t SERVICEABI
__KernelTimeoutDriver(
    _In_ UUId_t             Ash, 
    _In_ UUId_t             TimerId,
    _In_ void*              TimerData)
{
    MRemoteCall_t Request;
    size_t Zero = 0;

    RPCInitialize(&Request, Ash, 1, __DRIVER_TIMEOUT);
    RPCSetArgument(&Request, 0, (const void*)&TimerId, sizeof(UUId_t));
    RPCSetArgument(&Request, 1, (const void*)&TimerData, sizeof(void*));
    RPCSetArgument(&Request, 2, (const void*)&Zero, sizeof(size_t));
    RPCSetArgument(&Request, 3, (const void*)&Zero, sizeof(size_t));
    RPCSetArgument(&Request, 4, (const void*)&Zero, sizeof(size_t));
    return ScRpcExecute(&Request, 1);
}

#endif //!_MCORE_INTERRUPTS_H_
