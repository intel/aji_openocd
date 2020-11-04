#ifndef OPENOCD_C_JTAG_CLIENT_GNUAJI_WIN64_H
#define OPENOCD_C_JTAG_CLIENT_GNUAJI_WIN64_H


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

#include <winsock2.h>
#include <windows.h>
#include "c_aji.h"

#define LIBRARY_NAME_JTAG_CLIENT "jtag_client.dll"

AJI_ERROR c_jtag_client_gnuaji_init(void);
AJI_ERROR c_jtag_client_gnuaji_free(void);

AJI_ERROR c_aji_get_hardware(DWORD* hardware_count, AJI_HARDWARE* hardware_list, DWORD timeout);
AJI_ERROR c_aji_get_hardware2(DWORD* hardware_count, AJI_HARDWARE* hardware_list, char** server_version_info_list, DWORD timeout);
AJI_ERROR c_aji_find_hardware(DWORD persistent_id, AJI_HARDWARE* hardware, DWORD timeout);
AJI_ERROR c_aji_find_hardware_a(const char* hw_name, AJI_HARDWARE* hardware, DWORD timeout);
AJI_ERROR c_aji_read_device_chain(AJI_CHAIN_ID chain_id, DWORD* device_count, AJI_DEVICE* device_list, _Bool auto_scan);
AJI_ERROR c_aji_get_nodes(AJI_CHAIN_ID chain_id, DWORD tap_position, DWORD* idcodes, DWORD* idcode_n);
AJI_ERROR c_aji_get_nodes_a(AJI_CHAIN_ID chain_id, DWORD tap_position, DWORD* idcodes, DWORD* idcode_n, DWORD* hub_info);
AJI_ERROR c_aji_get_nodes_b(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_HIER_ID* hier_ids, DWORD* hier_id_n, AJI_HUB_INFO* hub_infos);
AJI_ERROR c_aji_open_device(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID* open_id, const AJI_CLAIM* claims, DWORD claim_n, const char* application_name);
AJI_ERROR c_aji_open_device_a(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID* open_id, const AJI_CLAIM2* claims, DWORD claim_n, const char* application_name);
AJI_ERROR c_aji_close_device(AJI_OPEN_ID open_id);
AJI_ERROR c_aji_open_entire_device_chain(AJI_CHAIN_ID chain_id, AJI_OPEN_ID* open_id, AJI_CHAIN_TYPE style, const char* application_name);
AJI_ERROR c_aji_open_node(AJI_CHAIN_ID chain_id, DWORD tap_position, DWORD idcode, AJI_OPEN_ID* node_id, const AJI_CLAIM* claims, DWORD claim_n, const char* application_name);
AJI_ERROR c_aji_open_node_a(AJI_CHAIN_ID chain_id, DWORD tap_position, DWORD node_position, DWORD idcode, AJI_OPEN_ID* node_id, const AJI_CLAIM* claims, DWORD claim_n, const char* application_name);
AJI_ERROR c_aji_open_node_b(AJI_CHAIN_ID chain_id, DWORD tap_position, const AJI_HIER_ID* hier_id, AJI_OPEN_ID* node_id, const AJI_CLAIM2* claims, DWORD claim_n, const char* application_name);


AJI_ERROR c_aji_lock(AJI_OPEN_ID open_id, DWORD timeout, AJI_PACK_STYLE pack_style);
AJI_ERROR c_aji_unlock_lock_chain(AJI_OPEN_ID unlock_id, AJI_CHAIN_ID lock_id);
AJI_ERROR c_aji_unlock(AJI_OPEN_ID open_id);
AJI_ERROR c_aji_lock_chain(AJI_CHAIN_ID chain_id, DWORD timeout);
AJI_ERROR c_aji_unlock_chain(AJI_CHAIN_ID chain_id);
AJI_ERROR c_aji_unlock_chain_lock(AJI_CHAIN_ID unlock_id, AJI_OPEN_ID lock_id, AJI_PACK_STYLE pack_style);
AJI_ERROR c_aji_flush(AJI_OPEN_ID openid);

AJI_ERROR c_aji_test_logic_reset(AJI_OPEN_ID open_id);
AJI_ERROR c_aji_delay(AJI_OPEN_ID open_id, DWORD timeout_microseconds);
AJI_ERROR c_aji_run_test_idle(AJI_OPEN_ID open_id, DWORD num_clocks);
AJI_ERROR c_aji_run_test_idle_a(AJI_OPEN_ID open_id, DWORD num_clocks, DWORD flags);

AJI_ERROR c_aji_access_ir(AJI_OPEN_ID open_id, DWORD instruction, DWORD* captured_ir, DWORD flags);
AJI_ERROR c_aji_access_ir_a(AJI_OPEN_ID open_id, DWORD length_ir, const BYTE* write_bits, BYTE* read_bits, DWORD flags);
AJI_ERROR c_aji_access_ir_multiple(DWORD num_devices, const AJI_OPEN_ID* open_id, const DWORD* instructions, DWORD* captured_irs);
AJI_ERROR c_aji_access_dr(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE* write_bits, DWORD read_offset, DWORD read_length, BYTE* read_bits);
AJI_ERROR c_aji_access_dr_a(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE* write_bits, DWORD read_offset, DWORD read_length, BYTE* read_bits, DWORD batch);
AJI_ERROR c_aji_access_overlay(AJI_OPEN_ID node_id, DWORD overlay,  DWORD* captured_overlay);
#endif
