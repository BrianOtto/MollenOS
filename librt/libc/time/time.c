/* MollenOS
*
* Copyright 2011 - 2016, Philip Meulengracht
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
* MollenOS C Library - Get Time
*/

/* Includes */
#include <os/MollenOS.h>
#include <os/Syscall.h>
#include <time.h>
#include <stddef.h>
#include <string.h>

/* Time
 * Gets the time from either a 
 * time seed or returns the current
 * time */
time_t time(time_t *timer)
{
	/* Variables */
	time_t RetTime = 0;
	struct tm TimeStruct;

	/* Null out */
	memset(&TimeStruct, 0, sizeof(struct tm));

	/* Get current time */
	MollenOSDeviceQuery(DeviceClock, 0, &TimeStruct, sizeof(struct tm));

	/* Now convert the sys-time
	 * to time_t */
	RetTime = mktime(&TimeStruct);

	/* Update the user-variable
	 * if he passed a pointer */
	if (timer != NULL)
		*timer = RetTime;

	/* Done! */
	return RetTime;
}
