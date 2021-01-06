#ifndef INC_C_JTAG_CLIENT_GNUAJI_H
#define INC_C_JTAG_CLIENT_GNUAJI_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if PORT==WINDOWS
#include "c_jtag_client_gnuaji_win64.h"
#else
#include "c_jtag_client_gnuaji_lib64.h"
#endif

const char* c_aji_error_decode(AJI_ERROR code);
#endif
