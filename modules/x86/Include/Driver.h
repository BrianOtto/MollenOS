/* MollenOS Module
* A module in MollenOS is equal to a driver.
* Drivers has access to a lot of functions,
* with information that is passed directly through a device-structure
* along with information about the device
*/
#ifndef __MOLLENOS_DRIVER__
#define __MOLLENOS_DRIVER__

/* Includes */
#include <stdint.h>

/* Definitions */
#define kFuncKernelPanic		0
#define kFuncDebugPrint			1

#define kFuncMemAlloc			2
#define kFuncMemAllocAligned	3
#define kFuncMemFree			4

#define kFuncMemMapDeviceMem	5
#define kFuncMemAllocDma		6
#define kFuncMemGetMapping		7
#define kFuncMemFreeDma			8

#define kFuncStall				9
#define kFuncSleep				10
#define kFuncDelay				11
#define kFuncReadTSC			12

#define kFuncCreateThread		13
#define kFuncYield				14
#define kFuncSleepThread		15
#define kFuncWakeThread			16

#define kFuncInstallIrqPci		17
#define kFuncInstallIrqISA		18
#define kFuncInstallIrqIdt		19
#define kFuncInstallIrqShared	20
#define kFuncAllocateIrqISA		21

#define kFuncRegisterDevice		22
#define kFuncUnregisterDevice	23

#define kFuncReadPciDevice		24
#define kFuncWritePciDevice		25

#define kFuncSemaphoreCreate	26
#define kFuncSemaphoreV			27
#define kFuncSemaphoreP			28
#define kFuncSemaphoreDestroy	29

#define kFuncMutexCreate		30
#define kFuncMutexConstruct		31
#define kFuncMutexDestruct		32
#define kFuncMutexLock			33
#define kFuncMutexUnlock		34

#endif //!__MOLLENOS_DRIVER__