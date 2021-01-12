#ifndef INC_JTAG_CLIENT_GNUAJI_H
#define INC_JTAG_CLIENT_GNUAJI_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if PORT==WINDOWS
#include "jtag_client_gnuaji_win64.h"
#else
#include "jtag_client_gnuaji_lib64.h"
#endif

#endif //INC_JTAG_CLIENT_GNUAJI_H
