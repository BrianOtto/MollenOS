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
 * MollenOS - Vioarr Window Compositor System
 *  - The window compositor system and general window manager for
 *    MollenOS.
 */
#pragma once
#include "event.hpp"

class CDialogCreateBase : public CVioarrEvent {
public:
    enum EDialogType {
        DialogApplicationSearch
    };

    CDialogCreateBase(EDialogType Type) : CVioarrEvent(EventDialogCreate) {
        m_Type = Type;
    }
    ~CDialogCreateBase() { }

private:
    EDialogType m_Type;
};

class CDialogApplicationSearch : public CDialogCreateBase {
public:
    CDialogApplicationSearch() : CDialogCreateBase(EDialogType::DialogApplicationSearch) { }
    ~CDialogApplicationSearch() { }
};