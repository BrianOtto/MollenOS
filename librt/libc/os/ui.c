/* MollenOS
 *
 * Copyright 2018, Philip Meulengracht
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
 * MollenOS User Visual Interface
 *  - Provides functionality to create and manage windows used by the program
 */

#include <os/window.h>
#include <assert.h>
#include <stdlib.h>

// Globals
// State keeping for a single window
static DmaBuffer_t* ProgramWindowBuffer = NULL;
static long         ProgramWindowHandle = -1;

/* UiParametersSetDefault
 * Set(s) default window parameters for the given window param structure. */
void
UiParametersSetDefault(
    _In_  UIWindowParameters_t* Descriptor)
{
    // Sanitize parameters
    assert(Descriptor != NULL);
    Descriptor->Flags           = 0;
    Descriptor->Surface.Format  = SurfaceRGBA;
    
    Descriptor->Surface.Dimensions.x    = -1;
    Descriptor->Surface.Dimensions.y    = -1;
    Descriptor->Surface.Dimensions.w    = 450;
    Descriptor->Surface.Dimensions.h    = 300;
}

/* UiUnregisterWindow
 * Unregisters and destroys the current active window for the application.
 * If none are registered, OsError is returned. */
void
UiUnregisterWindow(void)
{
    if (ProgramWindowHandle != -1) {
        DestroyWindow(ProgramWindowHandle);
    }
}

/* UiRegisterWindow
 * Registers a new window with the window manage with the given 
 * configuration. If the configuration is invalid, OsError is returned. */
OsStatus_t
UiRegisterWindow(
    _In_  UIWindowParameters_t* Descriptor,
    _Out_ DmaBuffer_t**         WindowBuffer)
{
    OsStatus_t  Status;
    size_t      BytesNeccessary = 0;

    // Sanitize parameters
    assert(Descriptor != NULL);
    assert(WindowBuffer != NULL);
    
    // Calculate how many bytes are needed by checking sizes requested.
    if (Descriptor->Surface.Dimensions.w <= 100)    { Descriptor->Surface.Dimensions.w = 450; }
    if (Descriptor->Surface.Dimensions.h <= 100)    { Descriptor->Surface.Dimensions.h = 300; }
    BytesNeccessary = Descriptor->Surface.Dimensions.w * Descriptor->Surface.Dimensions.h * 4;

    // Create the buffer object
    *WindowBuffer = CreateBuffer(UUID_INVALID, BytesNeccessary);
    if (*WindowBuffer == NULL) {
        return OsError;
    }
    ProgramWindowBuffer = *WindowBuffer;

    // Create the window
    Status = CreateWindow(Descriptor, GetBufferHandle(ProgramWindowBuffer), &ProgramWindowHandle);
    if (Status == OsSuccess) {
        atexit(UiUnregisterWindow);
    }
    return Status;
}

/* UiSwapBackbuffer
 * Presents the current backbuffer and rendering all changes made to the window. */
OsStatus_t
UiSwapBackbuffer(void)
{
    if (ProgramWindowHandle == -1) {
        return OsError;
    }
    return SwapWindowBackbuffer(ProgramWindowHandle);
}
