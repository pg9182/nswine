
#if 0
#pragma makedep unix
#endif

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ntgdi.h"
#include "wine/debug.h"
#include "wine/gdi_driver.h"
#include "wine/unixlib.h"

static const struct user_driver_funcs headlessdrv_funcs = {
};

static NTSTATUS headlessdrv_unix_init(void *arg) {
    __wine_set_user_driver(&headlessdrv_funcs, WINE_GDI_DRIVER_VERSION);
    return 0;
}

const unixlib_entry_t __wine_unix_call_funcs[] = { headlessdrv_unix_init };
#ifdef _WIN64
const unixlib_entry_t __wine_unix_call_wow64_funcs[] = { headlessdrv_unix_init };
#endif /* _WIN64 */
