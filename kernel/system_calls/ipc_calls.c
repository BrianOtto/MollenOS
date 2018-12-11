/* MollenOS
 *
 * Copyright 2017, Philip Meulengracht
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
 * MollenOS MCore - System Calls
 */
#define __MODULE "SCIF"
//#define __TRACE

#include <modules/manager.h>
#include <modules/module.h>
#include <system/utils.h>
#include <threading.h>
#include <os/input.h>
#include <machine.h>
#include <handle.h>
#include <debug.h>
#include <pipe.h>

/* ScCreatePipe
 * Creates a new communication pipeline that can be used by seperate threads. Returns a 
 * system-wide unique handle that can referred to. */
OsStatus_t
ScCreatePipe( 
    _In_  int     Type,
    _Out_ UUId_t* Handle)
{
    SystemPipe_t* Pipe;

    if (Type == PIPE_RAW) {
        Pipe = CreateSystemPipe(0, PIPE_DEFAULT_ENTRYCOUNT);
    }
    else if (Type == PIPE_STRUCTURED) {
        Pipe = CreateSystemPipe(PIPE_MPMC | PIPE_STRUCTURED_BUFFER, PIPE_DEFAULT_ENTRYCOUNT);
    }
    else {
        return OsInvalidParameters;
    }
    *Handle = CreateHandle(HandleTypePipe, 0, Pipe);
    return OsSuccess;
}

/* ScDestroyPipe
 * Closes an existing pipe and releases any resources associated. This will then be invalid for
 * communication. */
OsStatus_t
ScDestroyPipe(
    _In_ UUId_t Handle)
{
    SystemPipe_t* Pipe = (SystemPipe_t*)LookupHandle(Handle);
    OsStatus_t    Status = OsInvalidParameters;

    if (Pipe != NULL) {
        Status = DestroyHandle(Handle);
        
        // Disable the window manager input event pipe
        if (Status == OsSuccess && GetMachine()->StdInput == Pipe) {
            GetMachine()->StdInput = NULL;
        }

        // Disable the window manager wm event pipe
        if (Status == OsSuccess && GetMachine()->WmInput == Pipe) {
            GetMachine()->WmInput = NULL;
        }
    }
    return Status;
}

/* ScReadPipe
 * Reads the requested number of bytes from the system-pipe. */
OsStatus_t
ScReadPipe(
    _In_ UUId_t   Handle,
    _In_ uint8_t* Message,
    _In_ size_t   Length)
{
    SystemPipe_t* Pipe = (SystemPipe_t*)LookupHandle(Handle);
    if (Pipe == NULL) {
        ERROR("Thread %s trying to read from non-existing pipe handle %u", 
            ThreadingGetCurrentThread(CpuGetCurrentId())->Name, Handle);
        return OsDoesNotExist;
    }

    if (Length != 0) {
        ReadSystemPipe(Pipe, Message, Length);
    }
    return OsSuccess;
}

/* ScWritePipe
 * Writes the requested number of bytes to the system-pipe. */
OsStatus_t
ScWritePipe(
    _In_ UUId_t   Handle,
    _In_ uint8_t* Message,
    _In_ size_t   Length)
{
    SystemPipe_t* Pipe = (SystemPipe_t*)LookupHandle(Handle);
    if (Message == NULL || Length == 0) {
        return OsError;
    }

    if (Pipe == NULL) {
        ERROR("%s: ScPipeWrite::Invalid pipe %u", 
            ThreadingGetCurrentThread(CpuGetCurrentId())->Name, Handle);
        return OsError;
    }
    WriteSystemPipe(Pipe, Message, Length);
    return OsSuccess;
}

/* ScRpcResponse
 * Waits for rpc request to finish by polling the default pipe for a rpc-response */
OsStatus_t
ScRpcResponse(
    _In_ MRemoteCall_t* RemoteCall)
{
    SystemPipe_t* Pipe = ThreadingGetCurrentThread(CpuGetCurrentId())->Pipe;
    assert(Pipe != NULL);
    assert(RemoteCall != NULL);
    assert(RemoteCall->Result.Length > 0);

    // Read up to <Length> bytes, this results in the next 1 .. Length
    // being read from the raw-pipe.
    ReadSystemPipe(Pipe, (uint8_t*)RemoteCall->Result.Data.Buffer, 
        RemoteCall->Result.Length);
    return OsSuccess;
}

/* ScRpcExecute
 * Executes an IPC RPC request to the given process and optionally waits for
 * a reply/response */
OsStatus_t
ScRpcExecute(
    _In_ MRemoteCall_t* RemoteCall,
    _In_ int            Async)
{
    SystemPipeUserState_t State;
    size_t                TotalLength = sizeof(MRemoteCall_t);
    SystemPipe_t*         Pipe;
    int                   i;

    assert(RemoteCall != NULL);
    TRACE("ScRpcExecute(Message %i, Async %i)", RemoteCall->Function, Async);
    
    Pipe = (SystemPipe_t*)LookupHandle(RemoteCall->Target);
    if (Pipe == NULL) {
        ERROR("Pipe-Handle %u did not exist", RemoteCall->Target);
        return OsError;
    }

    // Calculate how much data to be comitted
    for (i = 0; i < IPC_MAX_ARGUMENTS; i++) {
        if (RemoteCall->Arguments[i].Type == ARGUMENT_BUFFER) {
            TotalLength += RemoteCall->Arguments[i].Length;
        }
    }

    // Setup producer access
    AcquireSystemPipeProduction(Pipe, TotalLength, &State);
    WriteSystemPipeProduction(&State, (const uint8_t*)RemoteCall, sizeof(MRemoteCall_t));
    for (i = 0; i < IPC_MAX_ARGUMENTS; i++) {
        if (RemoteCall->Arguments[i].Type == ARGUMENT_BUFFER) {
            WriteSystemPipeProduction(&State, 
                (const uint8_t*)RemoteCall->Arguments[i].Data.Buffer,
                RemoteCall->Arguments[i].Length);
        }
    }
    if (Async) {
        return OsSuccess;
    }
    return ScRpcResponse(RemoteCall);
}

/* ScRpcListen
 * Listens for a new rpc-message on the default rpc-pipe. */
OsStatus_t
ScRpcListen(
    _In_ MRemoteCall_t* RemoteCall,
    _In_ uint8_t*       ArgumentBuffer)
{
    SystemPipeUserState_t   State;
    uint8_t*                BufferPointer = ArgumentBuffer;
    SystemModule_t*         Module;
    size_t                  Length;
    int                     i;
    
    assert(RemoteCall != NULL);
    TRACE("%s: ScRpcListen(Port %i)", MStringRaw(GetCurrentProcess()->Name), Port);
    
    // Start out by resolving both the process and pipe
    Module = GetCurrentModule();
    if (Module == NULL) {
        return OsInvalidPermissions;
    }

    AcquireSystemPipeConsumption(Module->Rpc, &Length, &State);
    ReadSystemPipeConsumption(&State, (uint8_t*)RemoteCall, sizeof(MRemoteCall_t));
    for (i = 0; i < IPC_MAX_ARGUMENTS; i++) {
        if (RemoteCall->Arguments[i].Type == ARGUMENT_BUFFER) {
            RemoteCall->Arguments[i].Data.Buffer = (const void*)BufferPointer;
            ReadSystemPipeConsumption(&State, BufferPointer, RemoteCall->Arguments[i].Length);
            BufferPointer += RemoteCall->Arguments[i].Length;
        }
    }
    FinalizeSystemPipeConsumption(Module->Rpc, &State);
    return OsSuccess;
}

/* ScRpcRespond
 * A wrapper for sending an RPC response to the calling thread. Each thread has each it's response
 * channel to avoid any concurrency issues. */
OsStatus_t
ScRpcRespond(
    _In_ MRemoteCallAddress_t* RemoteAddress,
    _In_ const uint8_t*        Buffer, 
    _In_ size_t                Length)
{
    MCoreThread_t* Thread = ThreadingGetThread(RemoteAddress->Thread);
    SystemPipe_t*  Pipe   = NULL;

    // Sanitize thread still exists
    if (Thread != NULL) {
        Pipe = Thread->Pipe;
    }
    if (Pipe == NULL) {
        ERROR("Thread %u did not exist", RemoteAddress->Thread);
        return OsError;
    }
    WriteSystemPipe(Pipe, Buffer, Length);
    return OsSuccess;
}