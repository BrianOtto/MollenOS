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
 * MollenOS C Library - Standard OS Header
 */

#ifndef __MOLLENOS_CLIB__
#define __MOLLENOS_CLIB__

/* Includes
 * C-Library */
#include <os/osdefs.h>

/* The max path to expect from mollenos 
 * file/path operations */
#ifndef MPATH_MAX
#define MPATH_MAX		512
#endif

/* This describes a window handle 
 * used by UI functions */
#ifndef MWNDHANDLE
#define MWNDHANDLE
typedef void *WndHandle_t;
#endif

/* Enumerators */
typedef enum _MollenOSDeviceType
{
	DeviceUnknown = 0,
	DeviceCpu,
	DeviceCpuCore,
	DeviceController,
	DeviceBus,
	DeviceClock,
	DeviceTimer,
	DevicePerfTimer,
	DeviceInput,
	DeviceStorage,
	DeviceVideo

} OSDeviceType_t;

/* Environment Paths */
typedef enum _EnvironmentPaths
{
	/* The default */
	PathCurrentWorkingDir = 0,

	/* Application Paths */
	PathApplicationBase,
	PathApplicationData,

	/* System Directories */
	PathSystemBase,
	PathSystemDirectory,

	/* Shared Directories */
	PathCommonBin,
	PathCommonDocuments,
	PathCommonInclude,
	PathCommonLib,
	PathCommonMedia,

	/* User Directories */
	PathUserBase,

	/* Special Directory Count */
	PathEnvironmentCount

} EnvironmentPath_t;

/* Environment Queries
 * List of the different options
 * for environment queries */
typedef enum _EnvironmentQueryFunction
{
	EnvironmentQueryOS,
	EnvironmentQueryCpuId,
	EnvironmentQueryMemory

} EnvironmentQueryFunction_t;

/* Structures */
typedef struct _MollenOSVideoDescriptor
{
	/* Framebuffer */
	void *FrameBufferAddr;

	/* Mode Information */
	size_t BytesPerScanline;
	size_t Height;
	size_t Width;
	int Depth;

	/* Pixel Information */
	int RedPosition;
	int BluePosition;
	int GreenPosition;
	int ReservedPosition;

	int RedMask;
	int BlueMask;
	int GreenMask;
	int ReservedMask;

} OSVideoDescriptor_t;

/* This is only hardcoded for now untill
 * we implement support for querying memory
 * options */
#ifndef MOS_PAGE_SIZE
#define MOS_PAGE_SIZE	0x1000
#endif

/* The max-path we support in the OS
 * for file-paths, in MollenOS we support
 * rather long paths */
#define _MAX_PATH 512

/* Cpp Guard */
#ifdef __cplusplus
extern "C" {
#endif

/***********************
 * Memory Prototypes
 ***********************/

/* Memory - Share 
 * This shares a piece of memory with the 
 * target process. The function returns NULL
 * on failure to share the piece of memory
 * otherwise it returns the new buffer handle
 * that can be accessed by the other process */
_MOS_API void *MollenOSMemoryShare(UUId_t Target, void *Buffer, size_t Size);

/* Memory - Unshare 
 * This takes a previous shared memory handle 
 * and unshares it again from the target process
 * The function returns 0 if unshare was succesful, 
 * otherwise -1 */
_MOS_API int MollenOSMemoryUnshare(UUId_t Target, void *MemoryHandle, size_t Size);

/***********************
 * Device Prototypes
 ***********************/
_MOS_API int MollenOSDeviceQuery(OSDeviceType_t Type, int Request, void *Buffer, size_t Length);

/***********************
 * Environment Prototypes
 ***********************/

/* Resolve Environmental Path
 * Resolves the environmental type
 * to an valid absolute path */
_MOS_API void EnvironmentResolve(EnvironmentPath_t SpecialPath, char *StringBuffer);

/* Environment Query
 * Query the system environment 
 * for information, this could be
 * memory, cpu, etc information */
_MOS_API int EnvironmentQuery(EnvironmentQueryFunction_t Function, void *Buffer, size_t Length);

/***********************
 * Screen Prototypes
 ***********************/

/* This function returns screen geomemtry
 * descriped as a rectangle structure, which
 * must be pre-allocated */
_MOS_API void MollenOSGetScreenGeometry(Rect_t *Rectangle);

/***********************
 * System Misc Prototypes
 * - No functions here should
 *   be called manually
 *   they are automatically used
 *   by systems
 ***********************/
_MOS_API int WaitForSignal(size_t Timeout);
_MOS_API int SignalProcess(UUId_t Target);
_MOS_API void MollenOSSystemLog(const char *Format, ...);
_MOS_API int MollenOSEndBoot(void);
_MOS_API int MollenOSRegisterWM(void);

#ifdef __cplusplus
}
#endif

#endif //!__MOLLENOS_CLIB__