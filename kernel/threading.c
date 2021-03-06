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
 * MollenOS Threading Interface
 * - Common routines that are platform independant to provide
 *   a flexible and generic threading platfrom
 */
#define __MODULE "MTIF"
//#define __TRACE

#include <garbagecollector.h>
#include <component/cpu.h>
//#include <system/interrupts.h>
#include <system/thread.h>
#include <system/utils.h>
#include <threading.h>
#include <timers.h>
#include <handle.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <debug.h>
#include <heap.h>

OsStatus_t ThreadingReap(void *Context);

static Collection_t     Threads         = COLLECTION_INIT(KeyId);
static UUId_t           GlbThreadGcId   = UUID_INVALID;
static _Atomic(UUId_t)  GlbThreadId     = ATOMIC_VAR_INIT(1);

/* ThreadingInitialize
 * Initializes static data and allocates resources. */
OsStatus_t
ThreadingInitialize(void)
{
    GlbThreadGcId = GcRegister(ThreadingReap);
    return OsSuccess;
}

/* ThreadingEnable
 * Enables the threading system for the given cpu calling the function. */
OsStatus_t
ThreadingEnable(void)
{
    MCoreThread_t* Thread = &GetCurrentProcessorCore()->IdleThread;
    DataKey_t Key;

    memset(Thread, 0, sizeof(MCoreThread_t));
    
    Thread->Id              = atomic_fetch_add(&GlbThreadId, 1);
    Thread->Name            = strdup("idle");
    Thread->ParentThreadId  = UUID_INVALID;
    Thread->ProcessHandle   = UUID_INVALID;
    Thread->Flags           = THREADING_KERNELMODE | THREADING_IDLE;

    SchedulerThreadInitialize(Thread, Thread->Flags);
    Thread->Pipe        = CreateSystemPipe(0, 6); // 64 entries, 4kb
    Thread->SignalQueue = CollectionCreate(KeyInteger);
    Key.Value.Id        = Thread->Id;
    COLLECTION_NODE_INIT(&Thread->CollectionHeader, Key);

    // Initialize arch-dependant members
    Thread->MemorySpace = GetCurrentSystemMemorySpace();
    if (ThreadingRegister(Thread) != OsSuccess) {
        ERROR("Failed to register thread with system. Threading is not enabled.");
        CpuHalt();
    }
    GetCurrentProcessorCore()->CurrentThread = Thread;
    return CollectionAppend(&Threads, &Thread->CollectionHeader);
}

/* ThreadingEntryPoint
 * Initializes and handles finish of the thread
 * all threads should use this entry point. No Return */
void
ThreadingEntryPoint(void)
{
    MCoreThread_t*  Thread;
    UUId_t          CoreId;

    TRACE("ThreadingEntryPoint()");

    CoreId  = CpuGetCurrentId();
    Thread  = ThreadingGetCurrentThread(CoreId);
    if (THREADING_RUNMODE(Thread->Flags) == THREADING_KERNELMODE || (Thread->Flags & THREADING_SWITCHMODE)) {
        Thread->Function(Thread->Arguments);
        ThreadingTerminateThread(Thread->Id, 0, 1);
    }
    else {
        ThreadingSwitchLevel();
    }
    for (;;);
}

/* ThreadingCreateThread
 * Creates a new thread with the given paramaters and it is immediately
 * queued up for execution. */
UUId_t
ThreadingCreateThread(
    _In_ const char*    Name,
    _In_ ThreadEntry_t  Function,
    _In_ void*          Arguments,
    _In_ Flags_t        Flags)
{
    MCoreThread_t*  Thread;
    MCoreThread_t*  Parent;
    UUId_t          Cpu;
    char            NameBuffer[16];
    DataKey_t       Key;

    TRACE("ThreadingCreateThread(%s, 0x%x)", Name, Flags);

    Key.Value.Id    = atomic_fetch_add(&GlbThreadId, 1);
    Cpu             = CpuGetCurrentId();
    Parent          = ThreadingGetCurrentThread(Cpu);

    Thread = (MCoreThread_t*)kmalloc(sizeof(MCoreThread_t));
    memset(Thread, 0, sizeof(MCoreThread_t));
    if (ThreadingRegister(Thread) != OsSuccess) {
        ERROR("Failed to register a new thread with system.");
        kfree(Thread);
        return UUID_INVALID;
    }

    // Sanitize name, if NULL generate a new thread name of format 'Thread X'
    if (Name == NULL) {
        memset(&NameBuffer[0], 0, sizeof(NameBuffer));
        sprintf(&NameBuffer[0], "thread %i", Key.Value);
        Thread->Name = strdup(&NameBuffer[0]);
    }
    else {
        Thread->Name = strdup(Name);
    }

    // Initialize some basic thread information 
    // The only flags we want to copy for now are the running-mode
    Thread->Id              = Key.Value.Id;
    Thread->ParentThreadId  = Parent->Id;
    Thread->ProcessHandle   = Parent->ProcessHandle;
    Thread->Function        = Function;
    Thread->Arguments       = Arguments;
    Thread->Flags           = Flags;
    COLLECTION_NODE_INIT(&Thread->CollectionHeader, Key);
    TimersGetSystemTick(&Thread->StartedAt);

    // Setup initial scheduler information
    SchedulerThreadInitialize(Thread, Flags);

    // Create communication members
    Thread->Pipe        = CreateSystemPipe(0, 6); // 64 entries, 4kb
    Thread->SignalQueue = CollectionCreate(KeyInteger);
    Thread->ActiveSignal.Signal = -1;

    // Flag-Special-Case
    // If it's NOT a kernel thread
    // we specify transition-mode
    if (THREADING_RUNMODE(Flags) != THREADING_KERNELMODE && !(Flags & THREADING_INHERIT)) {
        Thread->Flags |= THREADING_SWITCHMODE;
    }

    // Flag-Special-Case
    // Determine the address space we want
    // to initialize for this thread
    if (THREADING_RUNMODE(Flags) == THREADING_KERNELMODE) {
        Thread->MemorySpace = CreateSystemMemorySpace(MEMORY_SPACE_INHERIT);
    }
    else {
        Flags_t ASFlags = 0;

        if (THREADING_RUNMODE(Flags) == THREADING_DRIVERMODE) {
            ASFlags |= MEMORY_SPACE_SERVICE;
        }
        else {
            ASFlags |= MEMORY_SPACE_APPLICATION;
        }
        if (Flags & THREADING_INHERIT) {
            ASFlags |= MEMORY_SPACE_INHERIT;
        }
        Thread->MemorySpace = CreateSystemMemorySpace(ASFlags);
    }

    // Create context's neccessary
    Thread->Contexts[THREADING_CONTEXT_LEVEL0] = 
        ContextCreate(Thread->Flags, THREADING_CONTEXT_LEVEL0, (uintptr_t)&ThreadingEntryPoint, 0, 0, 0);
    Thread->Contexts[THREADING_CONTEXT_SIGNAL0] = 
        ContextCreate(Thread->Flags, THREADING_CONTEXT_SIGNAL0, 0, 0, 0, 0);
    if (Thread->ProcessHandle != UUID_INVALID) {
        AcquireHandle(Thread->ProcessHandle);
    }
    CollectionAppend(&Threads, &Thread->CollectionHeader);
    SchedulerThreadQueue(Thread, 0);
    return Thread->Id;
}

/* ThreadingCleanupThread
 * Cleans up the thread resources allocated both during creation and runtime. */
void
ThreadingCleanupThread(
    _In_ MCoreThread_t* Thread)
{
    CollectionItem_t*   Node;
    int                 i;

    assert(Thread != NULL);
    assert(atomic_load_explicit(&Thread->Cleanup, memory_order_relaxed) == 1);

    // Make sure we are completely removed as reference
    // from the entire system. We also signal all waiters for this
    // thread again before continueing just in case
    SchedulerHandleSignalAll((uintptr_t*)&Thread->Cleanup);
    ThreadingUnregister(Thread);

    _foreach(Node, Thread->SignalQueue) {
        kfree(Node->Data);
    }
    CollectionDestroy(Thread->SignalQueue);
    
    for (i = 0; i < THREADING_NUMCONTEXTS; i++) {
        if (Thread->Contexts[i] != NULL) {
            kfree(Thread->Contexts[i]);
        }
    }
    ReleaseSystemMemorySpace(Thread->MemorySpace);
    DestroySystemPipe(Thread->Pipe);

    // Remove a reference to the process
    if (Thread->ProcessHandle != UUID_INVALID) {
        DestroyHandle(Thread->ProcessHandle);
    }
    kfree((void*)Thread->Name);
    kfree(Thread);
}

/* ThreadingDetachThread
 * Detaches a running thread by marking it without parent, this will make
 * sure it runs untill it kills itself. */
OsStatus_t
ThreadingDetachThread(
    _In_  UUId_t        ThreadId)
{
    MCoreThread_t*  Thread  = ThreadingGetCurrentThread(CpuGetCurrentId());
    MCoreThread_t*  Target  = ThreadingGetThread(ThreadId);
    // Detach is allowed if the caller is the spawner or the caller
    // is in same process
    if (Target != NULL) {
        if (Target->ParentThreadId == Thread->Id || 
            Target->ProcessHandle == Thread->ProcessHandle) {
            Target->ParentThreadId = UUID_INVALID;
            return OsSuccess;
        }
        WARNING(" > invalid permissions to perform thread detach");
    }
    return OsError;
}

/* ThreadingTerminateThread
 * Marks the thread with the given id for finished, and it will be cleaned up
 * on next switch unless specified. The given exitcode will be stored. */
OsStatus_t
ThreadingTerminateThread(
    _In_ UUId_t         ThreadId,
    _In_ int            ExitCode,
    _In_ int            TerminateChildren)
{
    CollectionItem_t*   Node;
    MCoreThread_t*      Target = ThreadingGetThread(ThreadId);

    if (Target == NULL || (Target->Flags & THREADING_IDLE)) {
        return OsError; // Never, ever kill system idle threads
    }

    if (TerminateChildren) {
        _foreach(Node, &Threads) {
            MCoreThread_t *Child = (MCoreThread_t*)Node;
            if (Child->ParentThreadId == Target->Id) {
                ThreadingTerminateThread(Child->Id, ExitCode, 1);
            }
        }
    }
    Target->RetCode = ExitCode;
    atomic_store(&Target->Cleanup, 1);

    // If the thread we are trying to kill is not this one, and is sleeping
    // we must wake it up, it will be cleaned on next schedule
    if (ThreadId != ThreadingGetCurrentThreadId()) {
        SchedulerThreadSignal(Target);
    }
    SchedulerHandleSignalAll((uintptr_t*)&Target->Cleanup);
    return OsSuccess;
}

/* ThreadingJoinThread
 * Can be used to wait for a thread the return value of this function is the exit code of the thread */
int
ThreadingJoinThread(
    _In_ UUId_t         ThreadId)
{
    MCoreThread_t* Target = ThreadingGetThread(ThreadId);
    if (Target != NULL && Target->ParentThreadId != UUID_INVALID) {
        int Finished = atomic_load(&Target->Cleanup);
        if (Finished != 1) {
            SchedulerAtomicThreadSleep((atomic_int*)&Target->Cleanup, &Finished, 0);
        }
        atomic_store(&Target->Cleanup, 1);
        return Target->RetCode;
    }
    return -1;
}

/* ThreadingSwitchLevel
 * Initializes non-kernel mode and marks the thread
 * for transitioning, there is no return from this function */
void
ThreadingSwitchLevel(void)
{
    MCoreThread_t* Thread = ThreadingGetCurrentThread(CpuGetCurrentId());

    Thread->Contexts[THREADING_CONTEXT_LEVEL1]  = ContextCreate(Thread->Flags,
        THREADING_CONTEXT_LEVEL1, (uintptr_t)Thread->Function, 0, (uintptr_t)Thread->Arguments, 0);
    Thread->Contexts[THREADING_CONTEXT_SIGNAL1] = ContextCreate(Thread->Flags,
        THREADING_CONTEXT_SIGNAL1, 0, 0, 0, 0);
    Thread->Flags |= THREADING_TRANSITION_USERMODE;

    ThreadingYield();
    for (;;);
}

/* ThreadingGetCurrentThread
 * Retrieves the current thread on the given cpu
 * if there is any issues it returns NULL */
MCoreThread_t*
ThreadingGetCurrentThread(
    _In_ UUId_t         CoreId)
{
    return GetProcessorCore(CoreId)->CurrentThread;
}

/* ThreadingGetCurrentThreadId
 * Retrives the current thread id on the current cpu
 * from the callers perspective */
UUId_t
ThreadingGetCurrentThreadId(void)
{
    if (GetCurrentProcessorCore()->CurrentThread == NULL) {
        return 0;
    }
    return GetCurrentProcessorCore()->CurrentThread->Id;
}

/* ThreadingGetThread
 * Lookup thread by the given thread-id, returns NULL if invalid */
MCoreThread_t*
ThreadingGetThread(
    _In_ UUId_t         ThreadId)
{
    foreach(Node, &Threads) {
        MCoreThread_t *Thread = (MCoreThread_t*)Node;
        if (Thread->Id == ThreadId) {
            return Thread;
        }
    }
    return NULL;
}

/* ThreadingIsCurrentTaskIdle
 * Is the given cpu running it's idle task? */
int
ThreadingIsCurrentTaskIdle(
    _In_ UUId_t         CoreId)
{
    SystemCpuCore_t *Core = GetProcessorCore(CoreId);
    return (Core->CurrentThread == &Core->IdleThread) ? 1 : 0;
}

/* ThreadingGetCurrentMode
 * Returns the current run-mode for the current
 * thread on the current cpu */
Flags_t
ThreadingGetCurrentMode(void)
{
    if (ThreadingGetCurrentThread(CpuGetCurrentId()) == NULL) {
        return THREADING_KERNELMODE;
    }
    return ThreadingGetCurrentThread(CpuGetCurrentId())->Flags & THREADING_MODEMASK;
}

/* ThreadingReapZombies
 * Garbage-Collector function, it reaps and cleans up all threads */
OsStatus_t
ThreadingReap(
    _In_ void*          Context)
{
    foreach(i, &Threads) {
        if ((void*)i == Context) {
            MCoreThread_t* Thread = (MCoreThread_t*)i;
            CollectionRemoveByNode(&Threads, &Thread->CollectionHeader);
            ThreadingCleanupThread(Thread);
            break;
        }
    }
    return OsSuccess;
}

/* ThreadingDebugPrint
 * Prints out debugging information about each thread
 * in the system, only active threads */
void
ThreadingDebugPrint(void)
{
    foreach(i, &Threads) {
        MCoreThread_t *Thread = (MCoreThread_t*)i;
        if (atomic_load_explicit(&Thread->Cleanup, memory_order_relaxed) == 0) {
            WRITELINE("Thread %u (%s) - Flags 0x%x, Queue %i, Timeslice %u, Cpu: %u",
                Thread->Id, Thread->Name, Thread->Flags, Thread->Queue, 
                Thread->TimeSlice, Thread->CoreId);
        }
    }
}

/* ThreadingSwitch
 * This is the thread-switch function and must be 
 * be called from the below architecture to get the
 * next thread to run */
MCoreThread_t*
ThreadingSwitch(
    _In_ MCoreThread_t* Current, 
    _In_ int            PreEmptive,
    _InOut_ Context_t** Context)
{
    SystemCpuCore_t*    Core;
    MCoreThread_t*      NextThread;
    int                 Cleanup;

    Core                    = GetCurrentProcessorCore();
    Current->ContextActive  = *Context;

    // Perform thread stuck detection
#if 0
    if (PreEmptive && Current->ContextActive != NULL && !(Current->Flags & THREADING_IDLE)) {
        if (Current->LastInstructionPointer == CONTEXT_IP(Current->ContextActive)) {
            FATAL(FATAL_SCOPE_KERNEL, " > detected thread %s stuck at instruction 0x%x\n", 
                Current->Name, CONTEXT_IP(Current->ContextActive));
        }
        Current->LastInstructionPointer = CONTEXT_IP(Current->ContextActive);
    }
#endif
    TRACE(" > current thread: %s", Current->Name);

    Cleanup = atomic_load_explicit(&Current->Cleanup, memory_order_relaxed);
GetNextThread:
    if ((Current->Flags & THREADING_IDLE) || Cleanup == 1) {
        // If the thread is finished then add it to garbagecollector
        if (Cleanup == 1) {
            GcSignal(GlbThreadGcId, Current);
        }
        NextThread = SchedulerThreadSchedule(NULL, PreEmptive);
        TRACE(" > (null-schedule) initial next thread: %s", (NextThread) ? NextThread->Name : "null");
    }
    else {
        NextThread = SchedulerThreadSchedule(Current, PreEmptive);
        TRACE(" > initial next thread: %s", (NextThread) ? NextThread->Name : "null");
    }

    // Sanitize if we need to active our idle thread, otherwise
    // do a final check that we haven't just gotten ahold of a thread
    // marked for finish
    if (NextThread == NULL) {
        NextThread = &Core->IdleThread;
    }
    else {
        Cleanup = atomic_load_explicit(&NextThread->Cleanup, memory_order_relaxed);
        if (Cleanup == 1) {
            Current = NextThread;
            goto GetNextThread;
        }
    }
    TRACE(" > next thread: %s", NextThread->Name);

    // Handle level switch // thread startup
    if (NextThread->Flags & THREADING_TRANSITION_USERMODE) {
        NextThread->Flags           &= ~(THREADING_SWITCHMODE | THREADING_TRANSITION_USERMODE);
        NextThread->ContextActive   = NextThread->Contexts[THREADING_CONTEXT_LEVEL1];
    }
    if (NextThread->ContextActive == NULL) {
        NextThread->ContextActive   = NextThread->Contexts[THREADING_CONTEXT_LEVEL0];
    }

    Core->CurrentThread = NextThread;
    *Context            = NextThread->ContextActive;
    return NextThread;
}
