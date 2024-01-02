#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/unixlib.h"

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved) {
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(instance);
    if (__wine_init_unix_call()) return FALSE;
    WINE_UNIX_CALL(0, NULL);
    return TRUE;
}
