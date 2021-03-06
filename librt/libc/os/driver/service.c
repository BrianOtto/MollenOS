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
 * MollenOS MCore - Service Definitions & Structures
 * - This header describes the base service-structure, prototypes
 *   and functionality, refer to the individual things for descriptions
 */

/* Includes
 * - System */
#include <os/device.h>
#include <os/syscall.h>

/* RegisterService 
 * Registers a service on the current alias, allowing
 * other applications and frameworks to send commands
 * and function requests */
OsStatus_t
RegisterService(
    _In_ UUId_t Alias) {
	return Syscall_RegisterService(Alias);
}

/* InstallDriver 
 * Tries to find a suitable driver for the given device
 * by searching storage-medias for the vendorid/deviceid 
 * combination or the class/subclass combination if specific
 * is not found */
OsStatus_t
InstallDriver(
    _In_ MCoreDevice_t* Device, 
    _In_ size_t         Length) {
	return Syscall_LoadDriver(Device, Length);
}
