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
 * MollenOS MCore - File Definitions & Structures
 * - This header describes the base file-structure, prototypes
 *   and functionality, refer to the individual things for descriptions
 */

#ifndef _FILE_INTERFACE_H_
#define _FILE_INTERFACE_H_

#include <os/mollenos.h>
#include <os/service.h>
#include <os/buffer.h>
#include <os/ipc/ipc.h>

/* This is the options structure used exclusively
 * for multi-params readback from the ipc operations */
PACKED_TYPESTRUCT(OpenFilePackage, {
    FileSystemCode_t    Code;
    UUId_t              Handle;
});
PACKED_TYPESTRUCT(RWFilePackage, {
    FileSystemCode_t    Code;
    size_t              Index;
    size_t              ActualSize;
});
PACKED_TYPESTRUCT(QueryFileValuePackage, {
    FileSystemCode_t    Code;
    union {
        struct {
            uint32_t    Lo;
            uint32_t    Hi;
        } Parts;
        uint64_t        Full;
    } Value;
});
PACKED_TYPESTRUCT(QueryFileOptionsPackage, {
    FileSystemCode_t    Code;
    Flags_t             Options;
    Flags_t             Access;
});
PACKED_TYPESTRUCT(QueryFileStatsPackage, {
    FileSystemCode_t    Code;
    OsFileDescriptor_t  Descriptor;
});

/* These definitions are in-place to allow a custom
 * setting of the file-manager, these are set to values
 * where in theory it should never be needed to have more */
#define __FILEMANAGER_INTERFACE_VERSION        1

/* These are the different IPC functions supported
 * by the filemanager, note that some of them might
 * be changed in the different versions, and/or new functions will be added */
#define __FILEMANAGER_REGISTERDISK              IPC_DECL_FUNCTION(0)
#define __FILEMANAGER_UNREGISTERDISK            IPC_DECL_FUNCTION(1)
#define __FILEMANAGER_QUERYDISKS                IPC_DECL_FUNCTION(2)
#define __FILEMANAGER_QUERYDISK                 IPC_DECL_FUNCTION(3)
#define __FILEMANAGER_QUERYDISKBYPATH           IPC_DECL_FUNCTION(4)
#define __FILEMANAGER_QUERYDISKBYHANDLE         IPC_DECL_FUNCTION(5)

#define __FILEMANAGER_OPEN                      IPC_DECL_FUNCTION(6)
#define __FILEMANAGER_CLOSE                     IPC_DECL_FUNCTION(7)
#define __FILEMANAGER_READ                      IPC_DECL_FUNCTION(8)
#define __FILEMANAGER_WRITE                     IPC_DECL_FUNCTION(9)
#define __FILEMANAGER_SEEK                      IPC_DECL_FUNCTION(10)
#define __FILEMANAGER_FLUSH                     IPC_DECL_FUNCTION(11)
#define __FILEMANAGER_MOVE                      IPC_DECL_FUNCTION(12)

#define __FILEMANAGER_GETPOSITION               IPC_DECL_FUNCTION(13)
#define __FILEMANAGER_GETOPTIONS                IPC_DECL_FUNCTION(14)
#define __FILEMANAGER_SETOPTIONS                IPC_DECL_FUNCTION(15)
#define __FILEMANAGER_GETSIZE                   IPC_DECL_FUNCTION(16)
#define __FILEMANAGER_GETPATH                   IPC_DECL_FUNCTION(17)
#define __FILEMANAGER_GETSTATSBYPATH            IPC_DECL_FUNCTION(18)
#define __FILEMANAGER_GETSTATSBYHANDLE          IPC_DECL_FUNCTION(19)
#define __FILEMANAGER_DELETEPATH                IPC_DECL_FUNCTION(20)

#define __FILEMANAGER_PATHRESOLVE               IPC_DECL_FUNCTION(21)
#define __FILEMANAGER_PATHCANONICALIZE          IPC_DECL_FUNCTION(22)

/* Bit flag defintions for operations such as 
 * registering / unregistering of disks, open flags
 * for OpenFile and access flags */
#define __DISK_REMOVABLE                        0x00000001

#define __DISK_FORCED_REMOVE                    0x00000001

// Access flags
#define __FILE_READ_ACCESS                      0x00000001
#define __FILE_WRITE_ACCESS                     0x00000002
#define __FILE_READ_SHARE                       0x00000100
#define __FILE_WRITE_SHARE                      0x00000200

// Options flag
#define __FILE_CREATE                           0x00000001
#define __FILE_CREATE_RECURSIVE                 0x00000002
#define __FILE_TRUNCATE                         0x00000004
#define __FILE_FAILONEXIST                      0x00000008
#define __FILE_APPEND                           0x00000100
#define __FILE_BINARY                           0x00000200
#define __FILE_VOLATILE                         0x00000400
#define __FILE_TEMPORARY                        0x00000800
#define __FILE_DIRECTORY                        0x00001000
#define __FILE_LINK                             0x00002000

/* RegisterDisk
 * Registers a disk with the file-manager and it will
 * automatically be parsed (MBR, GPT, etc), and all filesystems
 * on the disk will be brought online */
SERVICEAPI OsStatus_t SERVICEABI
RegisterDisk( 
    _In_ UUId_t  Device, 
    _In_ Flags_t Flags)
{
    // Variables
    MRemoteCall_t Request;

    // Initialize RPC
    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_REGISTERDISK);
    RPCSetArgument(&Request, 0, (const void*)&Device, sizeof(UUId_t));
    RPCSetArgument(&Request, 1, (const void*)&Flags, sizeof(Flags_t));
    return RPCEvent(&Request);
}

/* UnregisterDisk
 * Unregisters a disk from the system, and brings any filesystems
 * registered on this disk offline */
SERVICEAPI OsStatus_t SERVICEABI
UnregisterDisk(
    _In_ UUId_t  Device,
    _In_ Flags_t Flags)
{
    // Variables
    MRemoteCall_t Request;

    // Initialize RPC
    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_UNREGISTERDISK);
    RPCSetArgument(&Request, 0, (const void*)&Device, sizeof(UUId_t));
    RPCSetArgument(&Request, 1, (const void*)&Flags, sizeof(Flags_t));
    return RPCEvent(&Request);
}

/* QueryDiskByPath
 * Queries information about a storage medium that belong to the given file path. */
SERVICEAPI OsStatus_t SERVICEABI
QueryDiskByPath(
    _In_  const char*           Path,
    _Out_ vStorageDescriptor_t* StorageDescriptor)
{
    // Variables
    MRemoteCall_t Request;

    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_QUERYDISKBYPATH);
    RPCSetArgument(&Request, 0, (const void*)Path, strlen(Path) + 1);
    RPCSetResult(&Request, (const void*)StorageDescriptor, sizeof(vStorageDescriptor_t));
    return RPCExecute(&Request);
}

/* QueryDiskByHandle
 * Queries information about a storage medium that belong to the given file handle. */
SERVICEAPI OsStatus_t SERVICEABI
QueryDiskByHandle(
    _In_  UUId_t                Handle,
    _Out_ vStorageDescriptor_t* StorageDescriptor)
{
    // Variables
    MRemoteCall_t Request;
    
    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_QUERYDISKBYHANDLE);
    RPCSetArgument(&Request, 0, (const void*)&Handle, sizeof(UUId_t));
    RPCSetResult(&Request, (const void*)StorageDescriptor, sizeof(vStorageDescriptor_t));
    return RPCExecute(&Request);
}

/* OpenFile
 * Opens or creates the given file path based on
 * the given <Access> and <Options> flags. See the top of this file */
SERVICEAPI FileSystemCode_t SERVICEABI
OpenFile(
    _In_  const char*   Path, 
    _In_  Flags_t       Options, 
    _In_  Flags_t       Access,
    _Out_ UUId_t*       Handle)
{
    // Variables
    OpenFilePackage_t Package;
    MRemoteCall_t Request;

    // Initialize the request
    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_OPEN);
    RPCSetArgument(&Request, 0, (const void*)Path, strlen(Path) + 1);
    RPCSetArgument(&Request, 1, (const void*)&Options, sizeof(Flags_t));
    RPCSetArgument(&Request, 2, (const void*)&Access, sizeof(Flags_t));
    RPCSetResult(&Request, (const void*)&Package, sizeof(OpenFilePackage_t));

    // Execute the rpc request
    if (RPCExecute(&Request) != OsSuccess) {
        *Handle = UUID_INVALID;
        return FsInvalidParameters;
    }
    *Handle = Package.Handle;
    return Package.Code;
}

/* CloseFile
 * Closes the given file-handle, but does not necessarily
 * close the link to the file. Returns the result */
SERVICEAPI FileSystemCode_t SERVICEABI
CloseFile(
    _In_ UUId_t Handle)
{
    // Variables
    FileSystemCode_t Result = FsOk;
    MRemoteCall_t Request;

    // Initialize the request
    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_CLOSE);
    RPCSetArgument(&Request, 0, (const void*)&Handle, sizeof(UUId_t));
    RPCSetResult(&Request, (const void*)&Result, sizeof(FileSystemCode_t));

    // Execute the request 
    if (RPCExecute(&Request) != OsSuccess) {
        return FsInvalidParameters;
    }
    return Result;
}

/* DeletePath
 * Deletes the given path. The caller must make sure there is no other references
 * to the file or directory - otherwise delete fails */
SERVICEAPI FileSystemCode_t SERVICEABI
DeletePath(
    _In_ const char*    Path,
    _In_ Flags_t        Flags)
{
    // Variables
    FileSystemCode_t Result = FsOk;
    MRemoteCall_t Request;

    // Initialize the request
    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_DELETEPATH);
    RPCSetArgument(&Request, 0, (const void*)Path, strlen(Path) + 1);
    RPCSetArgument(&Request, 1, (const void*)&Flags, sizeof(Flags_t));
    RPCSetResult(&Request, (const void*)&Result, sizeof(FileSystemCode_t));

    // Execute the request 
    if (RPCExecute(&Request) != OsSuccess) {
        return FsInvalidParameters;
    }
    return Result;
}

/* ReadFile
 * Reads the requested number of bytes into the given buffer
 * from the current position in the file-handle */
SERVICEAPI FileSystemCode_t SERVICEABI
ReadFile(
    _In_      UUId_t            Handle,
    _In_      UUId_t            BufferHandle,
    _In_      size_t            Length,
    _Out_Opt_ size_t*           BytesIndex,
    _Out_Opt_ size_t*           BytesRead)
{
    // Variables
    RWFilePackage_t Package;
    MRemoteCall_t Request;

    // Initialize the request
    RPCInitialize(&Request, __FILEMANAGER_TARGET, __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_READ);
    RPCSetArgument(&Request, 0, (const void*)&Handle,       sizeof(UUId_t));
    RPCSetArgument(&Request, 1, (const void*)&BufferHandle, sizeof(UUId_t));
    RPCSetArgument(&Request, 2, (const void*)&Length,       sizeof(size_t));
    RPCSetResult(&Request,      (const void*)&Package,      sizeof(RWFilePackage_t));

    // Execute the request 
    if (RPCExecute(&Request) != OsSuccess) {
        Package.ActualSize  = 0;
        Package.Index       = 0;
        Package.Code        = FsInvalidParameters;
    }

    // Update out's
    if (BytesRead != NULL) {
        *BytesRead = Package.ActualSize;
    }
    if (BytesIndex != NULL) {
        *BytesIndex = Package.Index;
    }
    return Package.Code;
}

/* WriteFile
 * Writes the requested number of bytes from the given buffer
 * into the current position in the file-handle */
SERVICEAPI FileSystemCode_t SERVICEABI
WriteFile(
    _In_      UUId_t            Handle,
    _In_      UUId_t            BufferHandle,
    _In_      size_t            Length,
    _Out_Opt_ size_t*           BytesWritten)
{
    // Variables
    RWFilePackage_t Package;
    MRemoteCall_t Request;

    // Initialize the request
    RPCInitialize(&Request, __FILEMANAGER_TARGET, __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_WRITE);
    RPCSetArgument(&Request, 0, (const void*)&Handle,       sizeof(UUId_t));
    RPCSetArgument(&Request, 1, (const void*)&BufferHandle, sizeof(UUId_t));
    RPCSetArgument(&Request, 2, (const void*)&Length,       sizeof(size_t));
    RPCSetResult(&Request,      (const void*)&Package,      sizeof(RWFilePackage_t));

    // Execute the request
    if (RPCExecute(&Request) != OsSuccess) {
        Package.ActualSize  = 0;
        Package.Index       = 0;
        Package.Code        = FsInvalidParameters;
    }

    // Update out's
    if (BytesWritten != NULL) {
        *BytesWritten = Package.ActualSize;
    }
    return Package.Code;
}

/* SeekFile
 * Sets the file-pointer for the given handle to the
 * values given, the position is absolute and must
 * be within range of the file size */
SERVICEAPI FileSystemCode_t SERVICEABI
SeekFile(
    _In_ UUId_t     Handle, 
    _In_ uint32_t   SeekLo, 
    _In_ uint32_t   SeekHi)
{
    // Variables
    FileSystemCode_t Result = FsOk;
    MRemoteCall_t Request;

    // Initialize the request
    RPCInitialize(&Request, __FILEMANAGER_TARGET, __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_SEEK);
    RPCSetArgument(&Request, 0, (const void*)&Handle, sizeof(UUId_t));
    RPCSetArgument(&Request, 1, (const void*)&SeekLo, sizeof(uint32_t));
    RPCSetArgument(&Request, 2, (const void*)&SeekHi, sizeof(uint32_t));
    RPCSetResult(&Request,      (const void*)&Result, sizeof(FileSystemCode_t));

    // Execute the request
    if (RPCExecute(&Request) != OsSuccess) {
        return FsInvalidParameters;
    }
    return Result;
}

/* FlushFile
 * Flushes the internal file buffers and ensures there are
 * no pending file operations for the given file handle */
SERVICEAPI FileSystemCode_t SERVICEABI
FlushFile(
    _In_ UUId_t Handle)
{
    // Variables
    FileSystemCode_t Result = FsOk;
    MRemoteCall_t Request;

    // Initialize the request
    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_FLUSH);
    RPCSetArgument(&Request, 0, (const void*)&Handle, sizeof(UUId_t));
    RPCSetResult(&Request, (const void*)&Result, sizeof(FileSystemCode_t));

    // Execute the request
    if (RPCExecute(&Request) != OsSuccess) {
        return FsInvalidParameters;
    }
    return Result;
}

/* MoveFile
 * Moves or copies a given file path to the destination path
 * this can also be used for renamining if the dest/source paths
 * match (except for filename/directoryname) */
SERVICEAPI FileSystemCode_t SERVICEABI
MoveFile(
    _In_ const char*    Source,
    _In_ const char*    Destination,
    _In_ int            Copy)
{
    // Variables
    FileSystemCode_t Result = FsOk;
    MRemoteCall_t Request;

    // Initialize the request
    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_MOVE);
    RPCSetArgument(&Request, 0, (const void*)Source, strlen(Source) + 1);
    RPCSetArgument(&Request, 1, (const void*)Destination, strlen(Destination) + 1);
    RPCSetArgument(&Request, 2, (const void*)&Copy, sizeof(int));
    RPCSetResult(&Request, (const void*)&Result, sizeof(FileSystemCode_t));
    
    // Execute the request
    if (RPCExecute(&Request) != OsSuccess) {
        return FsInvalidParameters;
    }
    return Result;
}

/* GetFilePosition 
 * Queries the current file position that the given handle
 * is at, it returns as two separate unsigned values, the upper
 * value is optional and should only be checked for large files */
SERVICEAPI OsStatus_t SERVICEABI
GetFilePosition(
    _In_      UUId_t    Handle,
    _Out_     uint32_t* PositionLo,
    _Out_Opt_ uint32_t* PositionHi)
{
    // Variables
    QueryFileValuePackage_t Package;
    MRemoteCall_t Request;

    // Initialize the request
    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_GETPOSITION);
    RPCSetArgument(&Request, 0, (const void*)&Handle, sizeof(UUId_t));
    RPCSetResult(&Request, (const void*)&Package, sizeof(QueryFileValuePackage_t));
    
    // Execute the request
    if (RPCExecute(&Request) != OsSuccess) {
        *PositionLo = 0;
        if (PositionHi != NULL) {
            *PositionHi = 0;
        }
        return OsError;
    }

    // Update out's
    *PositionLo = Package.Value.Parts.Lo;
    if (PositionHi != NULL) {
        *PositionHi = Package.Value.Parts.Hi;
    }
    return OsSuccess;
}

/* GetFileOptions 
 * Queries the current file options and file access flags
 * for the given file handle */
SERVICEAPI OsStatus_t SERVICEABI
GetFileOptions(
    _In_  UUId_t    Handle,
    _Out_ Flags_t*  Options,
    _Out_ Flags_t*  Access)
{
    // Variables
    QueryFileOptionsPackage_t Package;
    MRemoteCall_t Request;

    // Initialize the request
    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_GETOPTIONS);
    RPCSetArgument(&Request, 0, (const void*)&Handle, sizeof(UUId_t));
    RPCSetResult(&Request, (const void*)&Package, sizeof(QueryFileOptionsPackage_t));
    
    // Execute the request
    if (RPCExecute(&Request) != OsSuccess) {
        *Options = 0;
        *Access = 0;
        return OsError;
    }

    // Update out's
    *Options = Package.Options;
    *Access = Package.Access;
    return OsSuccess;
}

/* SetFileOptions 
 * Attempts to modify the current option and or access flags
 * for the given file handle as specified by <Options> and <Access> */
SERVICEAPI OsStatus_t SERVICEABI
SetFileOptions(
    _In_ UUId_t  Handle,
    _In_ Flags_t Options,
    _In_ Flags_t Access)
{
    // Variables
    MRemoteCall_t Request;
    OsStatus_t Result = OsSuccess;

    // Initialize the request
    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_SETOPTIONS);
    RPCSetArgument(&Request, 0, (const void*)&Handle, sizeof(UUId_t));
    RPCSetArgument(&Request, 1, (const void*)&Options, sizeof(Flags_t));
    RPCSetArgument(&Request, 2, (const void*)&Access, sizeof(Flags_t));
    RPCSetResult(&Request, (const void*)&Result, sizeof(OsStatus_t));
    
    // Execute the request
    if (RPCExecute(&Request) != OsSuccess) {
        return OsError;
    }
    return Result;
}

/* GetFileSize 
 * Queries the current file size that the given handle
 * has, it returns as two separate unsigned values, the upper
 * value is optional and should only be checked for large files */
SERVICEAPI OsStatus_t SERVICEABI
GetFileSize(
    _In_ UUId_t          Handle,
    _Out_ uint32_t      *SizeLo,
    _Out_Opt_ uint32_t  *SizeHi)
{
    // Variables
    QueryFileValuePackage_t Package;
    MRemoteCall_t Request;

    // Initialize the request
    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_GETSIZE);
    RPCSetArgument(&Request, 0, (const void*)&Handle, sizeof(UUId_t));
    RPCSetResult(&Request, (const void*)&Package, sizeof(QueryFileValuePackage_t));

    // Execute the request
    if (RPCExecute(&Request) != OsSuccess) {
        *SizeLo = 0;
        if (SizeHi != NULL) {
            *SizeHi = 0;
        }
        return OsError;
    }

    // Update out's
    *SizeLo = Package.Value.Parts.Lo;
    if (SizeHi != NULL) {
        *SizeHi = Package.Value.Parts.Hi;
    }
    return OsSuccess;
}

/* GetFilePath 
 * Queries the full path of a file that the given handle
 * has, it returns it as a UTF8 string with max length of _MAXPATH */
SERVICEAPI OsStatus_t SERVICEABI
GetFilePath(
    _In_  UUId_t    Handle,
    _Out_ char*     Path,
    _Out_ size_t    MaxLength)
{
    // Variables
    MRemoteCall_t Request;
    char Buffer[_MAXPATH];

    // Reset local buffer
    memset(&Buffer[0], 0, _MAXPATH);

    // Initialize the request
    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_GETPATH);
    RPCSetArgument(&Request, 0, (const void*)&Handle, sizeof(UUId_t));
    RPCSetResult(&Request, (const void*)&Buffer[0], _MAXPATH);

    // Execute the request
    if (RPCExecute(&Request) != OsSuccess) {
        return OsError;
    }

    // Copy the result into the given buffer
    memcpy(Path, &Buffer[0], MIN(MaxLength, strlen(&Buffer[0])));
    return OsSuccess;
}

/* GetFileStatsByPath 
 * Queries file information by the given path. If the path is invalid
 * or doesn't exist the information is zeroed out. */
SERVICEAPI FileSystemCode_t SERVICEABI
GetFileStatsByPath(
    _In_ const char*            Path,
    _In_ OsFileDescriptor_t*    FileDescriptor)
{
    QueryFileStatsPackage_t Package;
    MRemoteCall_t           Request;
    FileSystemCode_t        Status = FsInvalidParameters;

    RPCInitialize(&Request, __FILEMANAGER_TARGET, __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_GETSTATSBYPATH);
    RPCSetArgument(&Request, 0, (const void*)Path, strlen(Path) + 1);
    RPCSetResult(&Request, (const void*)&Package, sizeof(QueryFileStatsPackage_t));
    
    if (RPCExecute(&Request) == OsSuccess) {
        Status = Package.Code;
        memcpy((void*)FileDescriptor, &Package.Descriptor, sizeof(OsFileDescriptor_t));
    }
    return Status;
}

/* GetFileStatsByHandle 
 * Queries file information by the given file handle. If the handle is invalid
 * or doesn't exist the information is zeroed out. */
SERVICEAPI FileSystemCode_t SERVICEABI
GetFileStatsByHandle(
    _In_ UUId_t                 Handle,
    _In_ OsFileDescriptor_t*    FileDescriptor)
{
    QueryFileStatsPackage_t Package;
    MRemoteCall_t           Request;
    FileSystemCode_t        Status = FsInvalidParameters;

    RPCInitialize(&Request, __FILEMANAGER_TARGET, __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_GETSTATSBYHANDLE);
    RPCSetArgument(&Request, 0, (const void*)&Handle, sizeof(UUId_t));
    RPCSetResult(&Request, (const void*)&FileDescriptor, sizeof(OsFileDescriptor_t));
    
    if (RPCExecute(&Request) == OsSuccess) {
        Status = Package.Code;
        memcpy((void*)FileDescriptor, &Package.Descriptor, sizeof(OsFileDescriptor_t));
    }
    return Status;
}

/* PathResolveEnvironment
 * Resolves the given env-path identifier to a string that can be used to locate files. */
SERVICEAPI OsStatus_t SERVICEABI
PathResolveEnvironment(
    _In_  EnvironmentPath_t Base,
    _Out_ char*             Buffer,
    _In_  size_t            MaxLength)
{
    MRemoteCall_t Request;

    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_PATHRESOLVE);
    RPCSetArgument(&Request, 0, (const void*)&Base, sizeof(EnvironmentPath_t));
    RPCSetResult(&Request, (const void*)Buffer, MaxLength);
    return RPCExecute(&Request);
}

/* PathCanonicalize
 * Canonicalizes the path by removing extra characters and resolving all identifiers in path */
SERVICEAPI OsStatus_t SERVICEABI
PathCanonicalize(
    _In_  const char*       Path,
    _Out_ char*             Buffer,
    _In_  size_t            MaxLength)
{
    MRemoteCall_t Request;

    RPCInitialize(&Request, __FILEMANAGER_TARGET, 
        __FILEMANAGER_INTERFACE_VERSION, __FILEMANAGER_PATHCANONICALIZE);
    RPCSetArgument(&Request, 0, (const void*)Path, strlen(Path) + 1);
    RPCSetResult(&Request, (const void*)Buffer, MaxLength);
    return RPCExecute(&Request);
}

#endif //!_FILE_INTERFACE_H_
