/*
 * DUMMYDRV initialization code
 *
 * Copyright 2023 pg9182
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/gdi_driver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dummydrv);

static BOOL CDECL DUMMY_RegisterHotKey(HWND hwnd, UINT modifiers, UINT vk)
{
    return TRUE;
}

static BOOL CDECL DUMMY_GetCursorPos(LPPOINT pt)
{
    return TRUE;
}

static BOOL CDECL DUMMY_SetCursorPos(INT x, INT y)
{
    return TRUE;
}

static BOOL CDECL DUMMY_CreateDesktopWindow(HWND hwnd)
{
    return TRUE;
}

static BOOL CDECL DUMMY_CreateWindow(HWND hwnd)
{
    ERR("Ignoring stubbed window creation.\n");
    return TRUE;
}

static DWORD CDECL DUMMY_MsgWaitForMultipleObjectsEx(DWORD count, const HANDLE *handles, DWORD timeout, DWORD mask, DWORD flags)
{
    if (!count && !timeout) return WAIT_TIMEOUT;
    return WaitForMultipleObjectsEx(count, handles, flags & MWMO_WAITALL, timeout, flags & MWMO_ALERTABLE);
}

static void CDECL DUMMY_SetWindowText(HWND hwnd, LPCWSTR text)
{
    TRACE("hwnd=%p text=%s\n", hwnd, wine_dbgstr_w(text));
}

static UINT CDECL DUMMY_ShowWindow(HWND hwnd, INT cmd, RECT *rect, UINT swp)
{
    return swp;
}

static LRESULT CDECL DUMMY_SysCommand(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    return -1;
}

static BOOL CDECL DUMMY_UpdateLayeredWindow(HWND hwnd, const UPDATELAYEREDWINDOWINFO *info, const RECT *window_rect)
{
    return TRUE;
}

static LRESULT CDECL DUMMY_WindowMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    default:
        FIXME("got window msg %x hwnd %p wp %lx lp %lx\n", msg, hwnd, wp, lp);
    }
    return 0;
}

static BOOL CDECL DUMMY_WindowPosChanging(HWND hwnd, HWND insert_after, UINT swp_flags, const RECT *window_rect, const RECT *client_rect, RECT *visible_rect, struct window_surface **surface)
{
    return TRUE;
}

static const struct user_driver_funcs dummydrv_funcs = {
    // we don't need graphics support
    .dc_funcs.priority = GDI_PRIORITY_NULL_DRV,

    // see dlls/user32/driver.c
    .pRegisterHotKey = DUMMY_RegisterHotKey,
    .pGetCursorPos = DUMMY_GetCursorPos,
    .pSetCursorPos = DUMMY_SetCursorPos,
    .pCreateDesktopWindow = DUMMY_CreateDesktopWindow,
    .pCreateWindow = DUMMY_CreateWindow,
    .pMsgWaitForMultipleObjectsEx = DUMMY_MsgWaitForMultipleObjectsEx,
    .pSetWindowText = DUMMY_SetWindowText,
    .pShowWindow = DUMMY_ShowWindow,
    .pSysCommand = DUMMY_SysCommand,
    .pUpdateLayeredWindow = DUMMY_UpdateLayeredWindow,
    .pWindowMessage = DUMMY_WindowMessage,
    .pWindowPosChanging = DUMMY_WindowPosChanging,
};

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
        __wine_set_user_driver(&dummydrv_funcs, WINE_GDI_DRIVER_VERSION);
    return TRUE;
}
