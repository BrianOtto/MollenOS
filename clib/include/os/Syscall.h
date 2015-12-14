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
* MollenOS Syscall Interface
*/

/* Guard */
#ifndef _MOLLENOS_SYSCALLS_H_
#define _MOLLENOS_SYSCALLS_H_

/* Includes */
#include <crtdefs.h>

/* Definitions */
#define MOLLENOS_SYSCALL_TERMINATE			0x1
#define MOLLENOS_SYSCALL_YIELD				0x2

#define MOLLENOS_SYSCALL_MEMALLOC			0x15
#define MOLLENOS_SYSCALL_MEMFREE			0x16

/* Prototypes */
_CRT_EXTERN int Syscall0(int Function);
_CRT_EXTERN int Syscall1(int Function, int Arg0);
_CRT_EXTERN int Syscall2(int Function, int Arg0, int Arg1);
_CRT_EXTERN int Syscall3(int Function, int Arg0, int Arg1, int Arg2);
_CRT_EXTERN int Syscall4(int Function, int Arg0, int Arg1, int Arg2, int Arg3);
_CRT_EXTERN int Syscall5(int Function, int Arg0, int Arg1, int Arg2, int Arg3, int Arg4);

#endif //!_MOLLENOS_SYSCALLS_H_