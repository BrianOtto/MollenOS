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
 * MollenOS MCore - MollenOS Module Manager
 */
#define __MODULE "MODS"
//#define __TRACE

#include <system/interrupts.h>
#include <process/phoenix.h>
#include <modules/modules.h>
#include <ds/collection.h>
#include <interrupts.h>
#include <debug.h>
#include <crc32.h>
#include <heap.h>

#define LIST_MODULE             1
#define LIST_SERVER             2

static Collection_t Modules             = COLLECTION_INIT(KeyInteger);
static int          ModulesAvailable    = 0;

/* ModulesInitialize
 * Loads the ramdisk, iterates all headers and 
 * builds a list of both available servers and 
 * available drivers */
OsStatus_t
ModulesInitialize(
    _In_ Multiboot_t *BootInformation)
{
    MCoreRamDiskHeader_t*   Ramdisk;
    MCoreRamDiskEntry_t*    Entry;
    int                     Counter = 0;
    
    // Debug
    TRACE("ModulesInitialize(Address 0x%x, Size 0x%x)",
        BootInformation->RamdiskAddress, BootInformation->RamdiskSize);

    // Sanitize the boot-parameters 
    // We will consider the possiblity of 0 values to be there is no ramdisk
    if (BootInformation == NULL || BootInformation->RamdiskAddress == 0 || 
        BootInformation->RamdiskSize == 0) {
        TRACE("No ramdisk supplied by the boot-descriptor");
        return OsSuccess;
    }
    
    // Initialize the pointer and read the signature value, must match
    Ramdisk = (MCoreRamDiskHeader_t*)(uintptr_t)BootInformation->RamdiskAddress;
    if (Ramdisk->Magic != RAMDISK_MAGIC) {
        ERROR("Invalid magic in ramdisk - 0x%x", Ramdisk->Magic);
        return OsError;
    }
    if (Ramdisk->Version != RAMDISK_VERSION_1) {
        ERROR("Invalid ramdisk version - 0x%x", Ramdisk->Version);
        return OsError;
    }

    // Store filecount so we can iterate
    Entry = (MCoreRamDiskEntry_t*)
        (BootInformation->RamdiskAddress + sizeof(MCoreRamDiskHeader_t));
    Counter = Ramdisk->FileCount;

    // Keep iterating untill we reach the end of counter
    TRACE("Parsing %i number of files in the ramdisk", Counter);
    while (Counter != 0) {
        if (Entry->Type == RAMDISK_MODULE || Entry->Type == RAMDISK_FILE) {
            MCoreRamDiskModuleHeader_t *Header =
                (MCoreRamDiskModuleHeader_t*)(uintptr_t)(BootInformation->RamdiskAddress + Entry->DataHeaderOffset);
            MCoreModule_t*  Module;
            uint8_t*        ModuleData;
            uint32_t        CrcOfData;
            DataKey_t       Key;

            // Perform CRC validation
            ModuleData = (uint8_t*)(BootInformation->RamdiskAddress 
                + Entry->DataHeaderOffset + sizeof(MCoreRamDiskModuleHeader_t));
            CrcOfData = Crc32Generate(-1, ModuleData, Header->LengthOfData);
            if (CrcOfData != Header->Crc32OfData) {
                ERROR("CRC-Validation(%s): Failed (Calculated 0x%x != Stored 0x%x)",
                    &Entry->Name[0], CrcOfData, Header->Crc32OfData);
            }

            // Allocate a new module header and copy some values 
            Module = (MCoreModule_t*)kmalloc(sizeof(MCoreModule_t));
            Module->Name = MStringCreate(Entry->Name, StrUTF8);
            memcpy(&Module->Header, Header, sizeof(MCoreRamDiskModuleHeader_t));
            Module->Data = (__CONST void*)kmalloc(Header->LengthOfData);
            memcpy((void*)Module->Data, ModuleData, Header->LengthOfData);

            // Update key based on the type of module
            // either its a server or a driver
            if (Header->Flags & RAMDISK_MODULE_SERVER) {
                Key.Value.Integer = LIST_SERVER;
            }
            else {
                Key.Value.Integer = LIST_MODULE;
            }
            CollectionAppend(&Modules, CollectionCreateNode(Key, Module));
        }
        else {
            WARNING("Unknown entry type: %u", Entry->Type);
        }
        Entry++;
        Counter--;
    }

    // Debug
    TRACE("Found %i Modules and Servers", CollectionLength(&Modules));
    ModulesAvailable = 1;
    return CollectionLength(&Modules) == 0 ? OsError : OsSuccess;
}

/* ModulesRunServers
 * Loads all iterated servers in the supplied ramdisk
 * by spawning each one as a new process */
void
ModulesRunServers(void)
{
    IntStatus_t IrqState = 0;
    if (ModulesAvailable != 1) {
        return;
    }

    // Disable interrupts while doing this as
    // we are still the idle thread -> as soon as a new
    // work is spawned we hardly ever return to this
    IrqState = InterruptDisable();

    // Iterate module list and spawn all servers
    // then they will "run" the system for us
    foreach(sNode, &Modules) {
        if (sNode->Key.Value.Integer == LIST_SERVER) {
            MString_t *Path = MStringCreate("rd:/", StrUTF8);
            MStringAppendString(Path, ((MCoreModule_t*)sNode->Data)->Name);
            CreateService(Path, 0, 0, 0, 0);
            MStringDestroy(Path);
        }
    }
    InterruptRestoreState(IrqState);
}

/* ModulesQueryPath
 * Retrieve a pointer to the file-buffer and its length 
 * based on the given <rd:/> path */
OsStatus_t
ModulesQueryPath(
    _In_ MString_t *Path, 
    _Out_ void **Buffer, 
    _Out_ size_t *Length)
{
    MString_t *Token    = Path;
    OsStatus_t Result   = OsError;
    int Index           = -1;
    TRACE("ModulesQueryPath(%s)", MStringRaw(Path));

    if (ModulesAvailable != 1) {
        goto Exit;
    }

    // Build the token we are looking for
    Index = MStringFindReverse(Path, '/', 0);
    if (Index != MSTRING_NOT_FOUND) {
        Token = MStringSubString(Path, Index + 1, -1);
    }
    TRACE("TokenToSearchFor(%s)", MStringRaw(Token));

    // Locate the module
    foreach(sNode, &Modules) {
        MCoreModule_t *Mod = (MCoreModule_t*)sNode->Data;
        TRACE("Comparing(%s)To(%s)", MStringRaw(Token), MStringRaw(Mod->Name));
        if (MStringCompare(Token, Mod->Name, 1) != MSTRING_NO_MATCH) {
            *Buffer = (void*)Mod->Data;
            *Length = Mod->Header.LengthOfData;
            Result = OsSuccess;
            break;
        }
    }
    
Exit:
    // Cleanup the token we created
    // and return the status
    if (Index != MSTRING_NOT_FOUND) {
        MStringDestroy(Token);
    }
    return Result;
}

/* ModulesFindGeneric
 * Resolve a 'generic' driver by its device-type and/or
 * its device sub-type, this is generally used if there is no
 * vendor specific driver available for the device. Returns NULL
 * if none is available */
MCoreModule_t*
ModulesFindGeneric(
    _In_ DevInfo_t DeviceType, 
    _In_ DevInfo_t DeviceSubType)
{
    if (ModulesAvailable != 1) {
        return NULL;
    }

    // Locate the module
    foreach(sNode, &Modules) {
        MCoreModule_t *Mod = (MCoreModule_t*)sNode->Data;
        if (Mod->Header.DeviceType == DeviceType
            && Mod->Header.DeviceSubType == DeviceSubType) {
            return Mod;
        }
    }
    return NULL;
}

/* ModulesFindSpecific
 * Resolve a specific driver by its vendorid and deviceid 
 * this is to ensure optimal module load. Returns NULL 
 * if none is available */
MCoreModule_t*
ModulesFindSpecific(
    _In_ DevInfo_t VendorId, 
    _In_ DevInfo_t DeviceId)
{
    if (ModulesAvailable != 1) {
        return NULL;
    }

    // Sanitize the id's
    if (VendorId == 0) {
        return NULL;
    }

    // Locate the module
    foreach(sNode, &Modules) {
        MCoreModule_t *Mod = (MCoreModule_t*)sNode->Data;
        if (Mod->Header.VendorId == VendorId
            && Mod->Header.DeviceId == DeviceId) {
            return Mod;
        }
    }
    return NULL;
}

/* ModulesFindString
 * Resolve a module by its name. Returns NULL if none
 * is available */
MCoreModule_t*
ModulesFindString(
    _In_ MString_t *Module)
{
    if (ModulesAvailable != 1) {
        return NULL;
    }

    // Locate the module
    foreach(sNode, &Modules) {
        MCoreModule_t *Mod = (MCoreModule_t*)sNode->Data;
        if (MStringCompare(Module, Mod->Name, 1) == MSTRING_FULL_MATCH) {
            return Mod;
        }
    }
    return NULL;
}
