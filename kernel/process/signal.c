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
 * MollenOS MCore - Signal Implementation
 */
#define __MODULE "SIG0"
//#define __TRACE

#include <process/process.h>
#include <system/thread.h>
#include <system/utils.h>
#include <threading.h>
#include <scheduler.h>
#include <string.h>
#include <handle.h>
#include <debug.h>
#include <heap.h>

/* Globals 
 * Keep track of signal-consequences */
char GlbSignalIsDeadly[] = {
    0, /* 0? */
    1, /* SIGHUP     */
    1, /* SIGINT     */
    2, /* SIGQUIT    */
    2, /* SIGILL     */
    2, /* SIGTRAP    */
    2, /* SIGABRT    */
    2, /* SIGEMT     */
    2, /* SIGFPE     */
    1, /* SIGKILL    */
    2, /* SIGBUS     */
    2, /* SIGSEGV    */
    2, /* SIGSYS     */
    1, /* SIGPIPE    */
    1, /* SIGALRM    */
    1, /* SIGTERM    */
    0, /* SIGUSR1    */
    0, /* SIGUSR2    */
    0, /* SIGCHLD    */
    0, /* SIGPWR     */
    0, /* SIGWINCH   */
    0, /* SIGURG     */
    0, /* SIGPOLL    */
    3, /* SIGSTOP    */
    3, /* SIGTSTP    */
    0, /* SIGCONT    */
    3, /* SIGTTIN    */
    3, /* SIGTTOUT   */
    1, /* SIGVTALRM  */
    1, /* SIGPROF    */
    2, /* SIGXCPU    */
    2, /* SIGXFSZ    */
    0, /* SIGWAITING */
    1, /* SIGDIAF    */
    0, /* SIGHATE    */
    0, /* SIGWINEVENT*/
    0, /* SIGCAT     */
    0  /* SIGEND     */
};

/* Create Signal 
 * Dispatches a signal to a thread in the system. If the thread is sleeping
 * and the signal is not masked, then it will be woken up. */
OsStatus_t
SignalCreate(
    _In_ UUId_t     ThreadId,
    _In_ int        Signal)
{
    MCoreThread_t*  Target   = ThreadingGetThread(ThreadId);
    SystemSignal_t* Sig;
    DataKey_t       Key;

    TRACE("SignalCreate(Thread %u, Signal %i)", ThreadId, Signal);

    // Sanitize input, and then sanitize if we have a handler
    if (Target == NULL || Signal >= NUMSIGNALS) {
        ERROR("Signal %i was not in range");
        return OsError; // Invalid
    }
    if (Target->SignalInformation[Signal] == 1) {
        ERROR("Signal %i was blocked");
        return OsError; // Ignored
    }
    Sig = (SystemSignal_t*)kmalloc(sizeof(SystemSignal_t));
    Sig->Ignorable      = GlbSignalIsDeadly[Signal];
    Sig->Signal         = Signal;
    Key.Value.Integer   = Signal;
    CollectionAppend(Target->SignalQueue, CollectionCreateNode(Key, Sig));

    // Wake up thread if neccessary
    if (Target->SchedulerFlags & SCHEDULER_FLAG_BLOCKED) {
        SchedulerThreadSignal(Target);
    }
    return OsSuccess;
}

/* SignalReturn
 * Call upon returning from a signal, this will finish
 * the signal-call and enter a new signal if any is queued up */
OsStatus_t
SignalReturn(void)
{
    UUId_t          Cpu     = CpuGetCurrentId();
    MCoreThread_t*  Thread  = ThreadingGetCurrentThread(Cpu);
    Thread->ActiveSignal.Signal = -1;
    return SignalHandle(Thread->Id);
}

/* Handle Signal 
 * This checks if the process has any waiting signals
 * and if it has, it executes the first in list */
OsStatus_t
SignalHandle(
    _In_ UUId_t ThreadId)
{
    CollectionItem_t*   Node;
    MCoreThread_t*      Thread = ThreadingGetThread(ThreadId);;
    SystemSignal_t*     Signal;

    if (Thread == NULL) {
        return OsError;
    }

    // Even if there is a Ash, we might want not
    // to Ash any signals ATM if there is already 
    // one active
    if (Thread->ActiveSignal.Signal != -1) {
        return OsError;
    }
    Node = CollectionPopFront(Thread->SignalQueue);

    // Sanitize the node, no more signals?
    if (Node != NULL) {
        Signal = (SystemSignal_t*)Node->Data;
        CollectionDestroyNode(Thread->SignalQueue, Node);
        SignalExecute(Thread, Signal);
    }
    return OsSuccess;
}

/* SignalExecute
 * This function does preliminary checks before actually
 * dispatching the signal to the process */
void
SignalExecute(
    _In_ MCoreThread_t *Thread,
    _In_ SystemSignal_t *Signal)
{
    SystemProcess_t* Process;
    TRACE("SignalExecute(Thread %u, Signal %i)", Thread->Id, Signal->Signal);

    // Instantiate the process
    Process = (SystemProcess_t*)LookupHandle(Thread->ProcessHandle);
    if (Process == NULL) {
        kfree(Signal);
        return;
    }

    // If there is no handler for the process and we
    // can't ignore signal, we must kill
    if (Process->SignalHandler == 0) {
        char Action = GlbSignalIsDeadly[Signal->Signal];
        if (Action == 1 || Action == 2) {
            ThreadingTerminateThread(Process->MainThreadId, Signal->Signal, 1);
        }
        kfree(Signal);
        return;
    }

    // Update active and dispatch
    memcpy(&Thread->ActiveSignal, Signal, sizeof(SystemSignal_t));
    Thread->ActiveSignal.Context = Thread->ContextActive;

    // Cleanup signal and dispatch
    kfree(Signal);
    TRACE("Signal dispatching..");
    ThreadingSignalDispatch(Thread);
}
