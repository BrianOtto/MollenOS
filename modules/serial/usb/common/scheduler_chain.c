/* MollenOS
 *
 * Copyright 2018, Philip Meulengracht
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
 * MollenOS MCore - USB Controller Scheduler
 * - Contains the implementation of a shared controller scheduker
 *   for all the usb drivers
 */
//#define __TRACE

/* Includes
 * - System */
#include <os/mollenos.h>
#include <os/utils.h>
#include "scheduler.h"

/* Includes
 * - Library */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

/* UsbSchedulerChainElement
 * Chains up a new element to the given element chain. The root element
 * must be specified and the element to append to the chain. Also the
 * chain direction must be specified. */
OsStatus_t
UsbSchedulerChainElement(
    _In_ UsbScheduler_t*    Scheduler,
    _In_ uint8_t*           ElementRoot,
    _In_ uint8_t*           Element,
    _In_ uint16_t           Marker,
    _In_ int                Direction)
{
    // Variables
    UsbSchedulerObject_t *RootObject= NULL;
    UsbSchedulerObject_t *Object    = NULL;
    UsbSchedulerPool_t *RootPool    = NULL;
    UsbSchedulerPool_t *Pool        = NULL;
    uintptr_t PhysicalAddress       = 0;
    OsStatus_t Result               = OsSuccess;
    uint16_t RootIndex              = 0;
    uint16_t LinkIndex              = 0;

    // Debug
    TRACE("UsbSchedulerChainElement(Marker is 0x%x)", Marker);
    
    // Validate element and lookup pool
    Result                  = UsbSchedulerGetPoolFromElement(Scheduler, ElementRoot, &RootPool);
    assert(Result == OsSuccess);
    Result                  = UsbSchedulerGetPoolFromElement(Scheduler, Element, &Pool);
    assert(Result == OsSuccess);
    RootObject              = USB_ELEMENT_OBJECT(RootPool, ElementRoot);
    Object                  = USB_ELEMENT_OBJECT(Pool, Element);
    PhysicalAddress         = USB_ELEMENT_PHYSICAL(Pool, Object->Index);

    // Get indices
    RootIndex               = RootObject->Index;
    LinkIndex               = (Direction == USB_CHAIN_BREATH) ? RootObject->BreathIndex : RootObject->DepthIndex;
    TRACE(" > Root 0x%x, initial link of root is 0x%x. Index of new element is 0x%x", RootIndex, LinkIndex, Object->Index);

    // Iterate to marker, support cyclic queues
    while (LinkIndex != Marker && LinkIndex != RootIndex) {
        // Move to next object
        RootPool        = USB_ELEMENT_GET_POOL(Scheduler, LinkIndex);
        ElementRoot     = USB_ELEMENT_INDEX(RootPool, LinkIndex);
        RootObject      = USB_ELEMENT_OBJECT(RootPool, ElementRoot);
        LinkIndex       = (Direction == USB_CHAIN_BREATH) ? RootObject->BreathIndex : RootObject->DepthIndex;
        TRACE(" > Current index is 0x%x, next index is 0x%x", RootObject->Index, LinkIndex);
    }

    // Append object
    // Set link of <Element> to be the link of <ElementRoot>
    // Set link of <ElementRoot> to be <Element>
    if (Direction == USB_CHAIN_BREATH)  Object->BreathIndex = RootObject->BreathIndex;
    else                                Object->DepthIndex  = RootObject->DepthIndex;
    USB_ELEMENT_LINK(Pool, Element, Direction) = USB_ELEMENT_LINK(RootPool, ElementRoot, Direction);
    MemoryBarrier();
    if (Direction == USB_CHAIN_BREATH)  RootObject->BreathIndex = Object->Index;
    else                                RootObject->DepthIndex = Object->Index;
    USB_ELEMENT_LINK(RootPool, ElementRoot, Direction) = LODWORD(PhysicalAddress) | USB_ELEMENT_LINKFLAGS(Object->Flags);
    TRACE("Indices of existing element is (0x%x:0x%x), indices of new element are now (0x%x:0x%x)",
        RootObject->BreathIndex, RootObject->DepthIndex, Object->BreathIndex, Object->DepthIndex);

    return OsSuccess;
}

/* UsbSchedulerUnchainElement
 * Removes an existing element from the given element chain. The root element
 * must be specified and the element to remove from the chain. Also the
 * chain direction must be specified. */
OsStatus_t
UsbSchedulerUnchainElement(
    _In_ UsbScheduler_t*    Scheduler,
    _In_ uint8_t*           ElementRoot,
    _In_ uint8_t*           Element,
    _In_ int                Direction)
{
    // Variables
    UsbSchedulerObject_t *RootObject= NULL;
    UsbSchedulerObject_t *sObject   = NULL;
    UsbSchedulerPool_t *RootPool    = NULL;
    UsbSchedulerPool_t *sPool       = NULL;
    OsStatus_t Result               = OsSuccess;
    uint16_t RootIndex              = 0;
    uint16_t LinkIndex              = 0;
    
    // Validate element and lookup pool
    Result                  = UsbSchedulerGetPoolFromElement(Scheduler, ElementRoot, &RootPool);
    assert(Result == OsSuccess);
    Result                  = UsbSchedulerGetPoolFromElement(Scheduler, Element, &sPool);
    assert(Result == OsSuccess);
    RootObject              = USB_ELEMENT_OBJECT(RootPool, ElementRoot);
    sObject                 = USB_ELEMENT_OBJECT(sPool, Element);

    // Get indices
    RootIndex               = RootObject->Index;
    LinkIndex               = (Direction == USB_CHAIN_BREATH) ? RootObject->BreathIndex : RootObject->DepthIndex;

    // Iterate to end/start/object, support cyclic queues
    while (LinkIndex != USB_ELEMENT_NO_INDEX && LinkIndex != RootIndex && LinkIndex != sObject->Index) {
        // Move to next object
        RootPool        = USB_ELEMENT_GET_POOL(Scheduler, LinkIndex);
        ElementRoot     = USB_ELEMENT_INDEX(RootPool, LinkIndex);
        RootObject      = USB_ELEMENT_OBJECT(RootPool, ElementRoot);
        LinkIndex       = (Direction == USB_CHAIN_BREATH) ? RootObject->BreathIndex : RootObject->DepthIndex;
    }

    // Only one accepted case, LinkIndex == sObject->Index
    if (LinkIndex == sObject->Index) {
        // Set link of <ElementRoot> to be the link of <Element>
        if (Direction == USB_CHAIN_BREATH)  RootObject->BreathIndex = sObject->BreathIndex;
        else                                RootObject->DepthIndex = sObject->DepthIndex;
        USB_ELEMENT_LINK(RootPool, ElementRoot, Direction) = USB_ELEMENT_LINK(sPool, Element, Direction);
        return OsSuccess;
    }
    return OsError;
}
