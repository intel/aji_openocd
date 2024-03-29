/***************************************************************************
 *   Copyright (C) 2021 by Intel Corporation                               *
 *   author: Ooi, Cinly                                                    *
 *   author-email: cinly.ooi@intel.com                                     *
 *   SPDX-License-Identifier: GPL-2.0-or-later                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef INC_C_JTAG_CLIENT_GNUAJI_H
#define INC_C_JTAG_CLIENT_GNUAJI_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Protecting the fact that these two files has to be #include-d at the 
// very beginning
#if PORT==WINDOWS
#include <winsock2.h>
#include <windows.h>
#endif 

#include "c_aji.h"

#if PORT==WINDOWS
#include "c_jtag_client_gnuaji_win64.h"
#else
#include "c_jtag_client_gnuaji_lib64.h"
#endif

const char* c_aji_error_decode(AJI_ERROR code);



/**
 * C AJI functions
 * It wraps mangled C++ name in non-manged C function names.
 * Will use the same function name as the original
 * C++ name but with a "c_" prefix
 *
 * For overloaded function that has to be converted to C names,
 * will add suffix '_a','_b', '_c' etc. Not using numeric suffix because
 * AJI might be using it already
 */

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
AJI_ERROR c_aji_get_nodes_bi(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_HIER_ID* hier_ids, DWORD* hier_id_n, AJI_HUB_INFO* hub_infos);
AJI_ERROR c_aji_open_device(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID* open_id, const AJI_CLAIM* claims, DWORD claim_n, const char* application_name);
AJI_ERROR c_aji_open_device_a(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID* open_id, const AJI_CLAIM2* claims, DWORD claim_n, const char* application_name);
AJI_ERROR c_aji_close_device(AJI_OPEN_ID open_id);
AJI_ERROR c_aji_open_entire_device_chain(AJI_CHAIN_ID chain_id, AJI_OPEN_ID* open_id, AJI_CHAIN_TYPE style, const char* application_name);
AJI_ERROR c_aji_open_node(AJI_CHAIN_ID chain_id, DWORD tap_position, DWORD idcode, AJI_OPEN_ID * node_id, const AJI_CLAIM * claims, DWORD claim_n, const char* application_name);
AJI_ERROR c_aji_open_node_a(AJI_CHAIN_ID chain_id, DWORD tap_position, DWORD node_position, DWORD idcode, AJI_OPEN_ID * node_id, const AJI_CLAIM * claims, DWORD claim_n, const char* application_name);
AJI_ERROR c_aji_open_node_b(AJI_CHAIN_ID chain_id, DWORD tap_position, const AJI_HIER_ID * hier_id, AJI_OPEN_ID * node_id, const AJI_CLAIM2 * claims, DWORD claim_n, const char* application_name);

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
AJI_ERROR c_aji_access_dr(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE* write_bits, DWORD read_offset, DWORD read_length, BYTE* read_bits);
AJI_ERROR c_aji_access_dr_a(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE* write_bits, DWORD read_offset, DWORD read_length, BYTE* read_bits, DWORD batch);
AJI_ERROR c_aji_access_overlay(AJI_OPEN_ID node_id, DWORD overlay,  DWORD* captured_overlay);

#endif
