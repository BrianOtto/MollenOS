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
 * MollenOS Keyboard Virtual Key Codes
 */

#ifndef _MCORE_KEYCODES_H_
#define _MCORE_KEYCODES_H_

/* Table */
typedef enum _VKey
{
	/* Mouse Codes */
	VK_INVALID			= 0x00,
	VK_LBUTTON			= 0x01,
	VK_RBUTTON			= 0x02,
	VK_MBUTTON			= 0x04,

	/* Keyboard Codes */
	VK_BACK				= 0x08,
	VK_TAB				= 0x09,
	VK_DOT				= 0x0A,

	VK_CLEAR			= 0x0C,
	VK_ENTER			= 0x0D,
	VK_SHIFT			= 0x10,
	VK_CONTROL			= 0x11,
	VK_ALT				= 0x12,
	VK_PAUSE			= 0x13,
	VK_CAPSLOCK			= 0x14,

	VK_ESCAPE			= 0x1B,
	VK_SPACE			= 0x20,
	VK_PAGEUP			= 0x21,
	VK_PAGEDOWN			= 0x22,
	VK_END				= 0x23,
	VK_HOME				= 0x24,
	VK_LEFT				= 0x25,
	VK_UP				= 0x26,
	VK_RIGHT			= 0x27,
	VK_DOWN				= 0x28,
	VK_SELECT			= 0x29,
	VK_PRINT			= 0x2A,
	VK_EXECUTE			= 0x2B,
	VK_SNAPSHOT			= 0x2C,
	VK_INSERT			= 0x2D,
	VK_DELETE			= 0x2E,
	VK_HELP				= 0x2F,

	/* 0x30 -> 0x39 is 0-9 */

	/* 0x41 -> 0x5A is A-Z */

	VK_LWIN				= 0x5B,
	VK_RWIN				= 0x5C,
	VK_APPS				= 0x5D,

	VK_SLEEP			= 0x5F,

	/* Numpad */
	VK_NUMPAD0			= 0x60,
	VK_NUMPAD1			= 0x61,
	VK_NUMPAD2			= 0x62,
	VK_NUMPAD3			= 0x63,
	VK_NUMPAD4			= 0x64,
	VK_NUMPAD5			= 0x65,
	VK_NUMPAD6			= 0x66,
	VK_NUMPAD7			= 0x67,
	VK_NUMPAD8			= 0x68,
	VK_NUMPAD9			= 0x69,
	VK_MULTIPLY			= 0x6A,
	VK_ADD				= 0x6B,
	VK_SEPARATOR		= 0x6C,
	VK_SUBTRACT			= 0x6D,
	VK_DECIMAL			= 0x6E,
	VK_DIVIDE			= 0x6F,

	/* Function Keys */
	VK_F1				= 0x70,
	VK_F2				= 0x71,
	VK_F3				= 0x72,
	VK_F4				= 0x73,
	VK_F5				= 0x74,
	VK_F6				= 0x75,
	VK_F7				= 0x76,
	VK_F8				= 0x77,
	VK_F9				= 0x78,
	VK_F10				= 0x79,
	VK_F11				= 0x7A,
	VK_F12				= 0x7B,
	VK_F13				= 0x7C,
	VK_F14				= 0x7D,
	VK_F15				= 0x7E,
	VK_F16				= 0x7F,
	VK_F17				= 0x80,
	VK_F18				= 0x81,
	VK_F19				= 0x82,
	VK_F20				= 0x83,
	VK_F21				= 0x84,
	VK_F22				= 0x85,
	VK_F23				= 0x86,
	VK_F24				= 0x87,

	VK_NUMLOCK			= 0x90,
	VK_SCROLL			= 0x91,

	VK_LSHIFT			= 0xA0,
	VK_RSHIFT			= 0xA1,
	VK_LCONTROL			= 0xA2,
	VK_RCONTROL			= 0xA3,
	VK_LALT				= 0xA4,
	VK_RALT				= 0xA5,
	VK_BROWSER_BACK		= 0xA6,
	VK_BROWSER_FORWARD	= 0xA7,
	VK_BROWSER_REFRESH	= 0xA8,
	VK_BROWSER_STOP		= 0xA9,
	VK_BROWSER_SEARCH	= 0xAA,
	VK_BROWSER_FAVORITES = 0xAB,
	VK_BROWSER_HOME		= 0xAC,
	VK_VOLUME_MUTE		= 0xAD,
	VK_VOLUME_DOWN		= 0xAE,
	VK_VOLUME_UP		= 0xAF,
	VK_MEDIA_NEXT_TRACK	= 0xB0,
	VK_MEDIA_PREV_TRACK	= 0xB1,
	VK_MEDIA_STOP		= 0xB2,
	VK_MEDIA_PLAY_PAUSE = 0xB3,
	VK_LAUNCH_MAIL		= 0xB4,
	VK_LAUNCH_MEDIA_SELECT	= 0xB5,
	VK_LAUNCH_APP1		= 0xB6,
	VK_LAUNCH_APP2		= 0xB7,

	/* Special Keys */
	VK_OEM_1			= 0xBA,
	VK_OEM_PLUS			= 0xBB,
	VK_OEM_COMMA		= 0xBC,
	VK_OEM_MINUS		= 0xBD,
	VK_OEM_PERIOD		= 0xBE,
	VK_OEM_2			= 0xBF,
	VK_OEM_3			= 0xC0,

	VK_OEM_4			= 0xDB,
	VK_OEM_5			= 0xDC,
	VK_OEM_6			= 0xDD,
	VK_OEM_7			= 0xDE,
	VK_OEM_8			= 0xDF,

	VK_PLAY				= 0xFA,
	VK_ZOOM				= 0xFB,
	VK_OEM_CLEAR		= 0xFE

} VKey;

#endif