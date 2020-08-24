#include "c_jtag_client_gnu_aji_win64.h"

HINSTANCE c_jtag_client_lib;
AJI_ERROR c_jtag_client_gnuaji_init(void) {
    c_jtag_client_lib = LoadLibrary(TEXT(LIBRARY_NAME_JTAG_CLIENT));
    if (c_jtag_client_lib == NULL) {
        LOG_ERROR("Cannot find %s.", LIBRARY_NAME_JTAG_CLIENT);
        return AJI_FAILURE;
    }
    return AJI_NO_ERROR;
}

void c_jtag_client_gnuaji_free(void) {
    if (c_jtag_client_lib != NULL) {
        FreeLibrary(c_jtag_client_lib);
    }
    c_jtag_client_lib = NULL;
}

