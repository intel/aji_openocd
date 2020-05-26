#ifndef OPENOCD_C_JTAG_CLIENT_GNUAJI_H
#define OPENOCD_C_JTAG_CLIENT_GNUAJI_H

/**
 * C AJI functions
 * It wraps mangled C++ name in jtag_client_gnu_aji.h with non-manged
 * C function name . Will use the same function name as the original
 * C++ name but with a "c" prefix
 *
 * For overloaded function that has to be converted to C names,
 * will add suffix '_a','_b', '_c' etc. Not using numeric suffix because
 * AJI might be using it already
 */

#include "jtag_client_gnuaji.h"

#include<stdio.h>

inline 
AJI_API AJI_ERROR c_aji_get_hardware(DWORD * hardware_count, AJI_HARDWARE * hardware_list, DWORD timeout) {
    return _Z16aji_get_hardwarePjP12AJI_HARDWAREj(hardware_count, hardware_list, timeout);
}
inline 
AJI_API AJI_ERROR c_aji_get_hardware2(DWORD * hardware_count, AJI_HARDWARE * hardware_list, char **server_version_info_list, DWORD timeout) {
printf("hwlistAA = %p\n", hardware_list);
    return _Z17aji_get_hardware2PjP12AJI_HARDWAREPPcj(hardware_count, hardware_list, server_version_info_list, timeout);
} 

inline
AJI_API AJI_ERROR c_aji_find_hardware(DWORD chain_id, AJI_HARDWARE * hardware, DWORD timeout) {
    return _Z17aji_find_hardwarejP12AJI_HARDWAREj(chain_id, hardware, timeout);
}
inline
AJI_API AJI_ERROR c_aji_find_hardware_a(const char * hw_name, AJI_HARDWARE * hardware, DWORD timeout) {
    return _Z17aji_find_hardwarePKcP12AJI_HARDWAREj(hw_name, hardware, timeout);
}


inline
AJI_API AJI_ERROR c_aji_unlock_chain(AJI_CHAIN_ID chain_id) {
    return _Z16aji_unlock_chainP9AJI_CHAIN(chain_id);
}
inline
AJI_API AJI_ERROR c_aji_unlock_chain_lock(AJI_CHAIN_ID unlock_id, AJI_OPEN_ID lock_id, AJI_PACK_STYLE pack_style) {
    return _Z21aji_unlock_chain_lockP9AJI_CHAINP8AJI_OPEN14AJI_PACK_STYLE(unlock_id, lock_id, pack_style);
}
#endif
