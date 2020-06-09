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

inline 
AJI_API AJI_ERROR c_aji_get_hardware(DWORD * hardware_count, AJI_HARDWARE * hardware_list, DWORD timeout) {
    return _Z16aji_get_hardwarePjP12AJI_HARDWAREj(hardware_count, hardware_list, timeout);
}
inline 
AJI_API AJI_ERROR c_aji_get_hardware2(DWORD * hardware_count, AJI_HARDWARE * hardware_list, char **server_version_info_list, DWORD timeout) {
    return _Z17aji_get_hardware2PjP12AJI_HARDWAREPPcj(hardware_count, hardware_list, server_version_info_list, timeout);
} 

inline
AJI_API AJI_ERROR c_aji_find_hardware(DWORD persistent_id, AJI_HARDWARE * hardware, DWORD timeout) {
    return _Z17aji_find_hardwarejP12AJI_HARDWAREj(persistent_id, hardware, timeout);
}
inline
AJI_API AJI_ERROR c_aji_find_hardware_a(const char * hw_name, AJI_HARDWARE * hardware, DWORD timeout) {
    return _Z17aji_find_hardwarePKcP12AJI_HARDWAREj(hw_name, hardware, timeout);
}

inline 
AJI_API AJI_ERROR c_aji_read_device_chain(AJI_CHAIN_ID chain_id, DWORD * device_count, AJI_DEVICE * device_list, _Bool auto_scan) {
    return _Z21aji_read_device_chainP9AJI_CHAINPjP10AJI_DEVICEb( chain_id, device_count, device_list, auto_scan);
}

inline
AJI_API AJI_ERROR c_aji_lock(AJI_OPEN_ID open_id, DWORD timeout, AJI_PACK_STYLE pack_style) {
    return _Z8aji_lockP8AJI_OPENj14AJI_PACK_STYLE(open_id, timeout, pack_style);
}

inline
AJI_API AJI_ERROR c_aji_unlock(AJI_OPEN_ID open_id) {   
    return _Z10aji_unlockP8AJI_OPEN(open_id);
}

inline
AJI_API AJI_ERROR c_aji_lock_chain(AJI_CHAIN_ID chain_id, DWORD timeout) {
    return _Z14aji_lock_chainP9AJI_CHAINj(chain_id, timeout); 
}

inline
AJI_API AJI_ERROR c_aji_unlock_chain(AJI_CHAIN_ID chain_id) {
    return _Z16aji_unlock_chainP9AJI_CHAIN(chain_id);
}

inline
AJI_API AJI_ERROR c_aji_unlock_chain_lock(AJI_CHAIN_ID unlock_id, AJI_OPEN_ID lock_id, AJI_PACK_STYLE pack_style) {
    return _Z21aji_unlock_chain_lockP9AJI_CHAINP8AJI_OPEN14AJI_PACK_STYLE(unlock_id, lock_id, pack_style);
}

inline
AJI_API AJI_ERROR c_aji_open_device(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID * open_id, const AJI_CLAIM * claims, DWORD claim_n, const char * application_name){
    return _Z15aji_open_deviceP9AJI_CHAINjPP8AJI_OPENPK9AJI_CLAIMjPKc(chain_id, tap_position, open_id, claims, claim_n, application_name);
}

inline
AJI_API AJI_ERROR c_aji_open_device_a(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID * open_id, const AJI_CLAIM2 * claims, DWORD claim_n, const char * application_name) {
    return _Z15aji_open_deviceP9AJI_CHAINjPP8AJI_OPENPK10AJI_CLAIM2jPKc(chain_id, tap_position, open_id, claims, claim_n, application_name);
} 

inline
AJI_API AJI_ERROR c_aji_close_device(AJI_OPEN_ID open_id) {
    return _Z16aji_close_deviceP8AJI_OPEN(open_id);
}

inline
AJI_API AJI_ERROR c_aji_open_entire_device_chain(AJI_CHAIN_ID chain_id, AJI_OPEN_ID * open_id, AJI_CHAIN_TYPE style, const char * application_name) {
    return  _Z28aji_open_entire_device_chainP9AJI_CHAINPP8AJI_OPEN14AJI_CHAIN_TYPEPKc(chain_id, open_id, style, application_name);
}

inline
AJI_API AJI_ERROR c_aji_test_logic_reset(AJI_OPEN_ID open_id) {
    return _Z20aji_test_logic_resetP8AJI_OPEN(open_id);
}

inline
AJI_API AJI_ERROR c_aji_access_ir(AJI_OPEN_ID open_id, DWORD instruction, DWORD * captured_ir, DWORD flags) {
    return _Z13aji_access_irP8AJI_OPENjPjj(open_id, instruction, captured_ir, flags);
}

inline
AJI_API AJI_ERROR c_aji_access_ir_a(AJI_OPEN_ID open_id, DWORD length_ir, const BYTE * write_bits, BYTE * read_bits, DWORD flags) {
    return _Z13aji_access_irP8AJI_OPENjPKhPhj(open_id, length_ir, write_bits, read_bits, flags);
}

inline
AJI_API AJI_ERROR c_aji_access_ir_multiple(DWORD num_devices, const AJI_OPEN_ID * open_id, const DWORD * instructions, DWORD * captured_irs) {
    return _Z22aji_access_ir_multiplejPKP8AJI_OPENPKjPj(num_devices, open_id, instructions, captured_irs);
}
inline
AJI_API AJI_ERROR c_aji_access_dr(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD read_offset, DWORD read_length, BYTE * read_bits) {
    return  _Z13aji_access_drP8AJI_OPENjjjjPKhjjPh(open_id, length_dr, flags, write_offset, write_length, write_bits, read_offset, read_length, read_bits);
}

inline
AJI_API AJI_ERROR c_aji_access_dr_a(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD read_offset, DWORD read_length, BYTE * read_bits, DWORD batch) {
    return _Z13aji_access_drP8AJI_OPENjjjjPKhjjPhj(open_id, length_dr, flags, write_offset, write_length, write_bits, read_offset, read_length, read_bits, batch);
}
#endif
