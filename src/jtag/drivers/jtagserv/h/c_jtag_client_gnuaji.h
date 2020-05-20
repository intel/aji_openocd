/**
 * C AJI functions
 * It wraps mangled C++ name in jtag_client_gnu_aji.h with non-manged
 * C function name . Will use the same function name as the original
 * C++ name but with a "c" prefix
 *
 * For overloaded function that has to be converted to C names,
 * will add suffix 'A','B', 'C' etc. Not using numeric suffix because
 * AJI might be using it already
 */

#include "jtag_client_gnuaji.h"


inline 
AJI_API AJI_ERROR c_aji_get_hardware(DWORD * hardware_count, AJI_HARDWARE * hardware_list, DWORD timeout) {
    return _Z16aji_get_hardwarePjP12AJI_HARDWAREj(hardware_count, hardware_list, timeout);
}
inline 
AJI_API AJI_ERROR c_aji_get_hardware2(DWORD * hardware_count, AJI_HARDWARE * hardware_list, char **server_version_info_list, DWORD timeout) {
    return _Z17aji_get_hardware2PjP12AJI_HARDWAREPPcj(hardware_count, hardware_list, server_version_info_list, timeout);
} 
