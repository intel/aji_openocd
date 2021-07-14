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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//There is no #define for Linux build, so use not win32 as proxy
#if ! IS_WIN32

#include <dlfcn.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "replacements.h"
#include "log.h"
#include "c_aji.h"
#include "c_jtag_client_gnuaji_lib64.h"

/**
 * C AJI functions
 * It wraps mangled C++ name in non-manged C function names.
 * Will use the same function name as the original
 * C++ name but with a "c_" prefix
 *
 * For overloaded function that has to be converted to C names,
 * will add suffix '_a','_b', '_c' etc. Not using numeric suffix because
 * AJI might be using it already
 *
 * Functions here are mainly from jtag_client.dll library
 */



AJI_ERROR c_jtag_client_gnuaji_init(void) {
    return AJI_NO_ERROR;
}

AJI_ERROR c_jtag_client_gnuaji_free(void) {
    return AJI_NO_ERROR;
}


AJI_ERROR c_aji_get_hardware(DWORD * hardware_count, AJI_HARDWARE * hardware_list, DWORD timeout) {
	AJI_ERROR _Z16aji_get_hardwarePjP12AJI_HARDWAREj(DWORD*, AJI_HARDWARE*, DWORD);
    return _Z16aji_get_hardwarePjP12AJI_HARDWAREj(hardware_count, hardware_list, timeout);
}

AJI_ERROR c_aji_get_hardware2(DWORD * hardware_count, AJI_HARDWARE * hardware_list, char **server_version_info_list, DWORD timeout) {
	AJI_ERROR _Z17aji_get_hardware2PjP12AJI_HARDWAREPPcj(DWORD*, AJI_HARDWARE*, char**, DWORD);
    return _Z17aji_get_hardware2PjP12AJI_HARDWAREPPcj(hardware_count, hardware_list, server_version_info_list, timeout);
} 

AJI_ERROR c_aji_find_hardware(DWORD persistent_id, AJI_HARDWARE * hardware, DWORD timeout) {
	AJI_ERROR _Z17aji_find_hardwarejP12AJI_HARDWAREj(DWORD, AJI_HARDWARE*, DWORD);
    return _Z17aji_find_hardwarejP12AJI_HARDWAREj(persistent_id, hardware, timeout);
}

AJI_ERROR c_aji_find_hardware_a(const char * hw_name, AJI_HARDWARE * hardware, DWORD timeout) {
    AJI_ERROR _Z17aji_find_hardwarePKcP12AJI_HARDWAREj(const char*, AJI_HARDWARE*, DWORD);
    return _Z17aji_find_hardwarePKcP12AJI_HARDWAREj(hw_name, hardware, timeout);
}

AJI_ERROR c_aji_read_device_chain(AJI_CHAIN_ID chain_id, DWORD * device_count, AJI_DEVICE * device_list, _Bool auto_scan) {
	AJI_ERROR _Z21aji_read_device_chainP9AJI_CHAINPjP10AJI_DEVICEb(AJI_CHAIN_ID, DWORD*, AJI_DEVICE*, _Bool);
    return _Z21aji_read_device_chainP9AJI_CHAINPjP10AJI_DEVICEb(chain_id, device_count, device_list, auto_scan);
}

AJI_ERROR AJI_API c_aji_get_nodes(
    AJI_CHAIN_ID         chain_id,
    DWORD                tap_position,
    DWORD* idcodes,
    DWORD* idcode_n) {
	AJI_ERROR _Z13aji_get_nodesP9AJI_CHAINjPjS1_(AJI_CHAIN_ID, DWORD, DWORD*, DWORD*);
    return _Z13aji_get_nodesP9AJI_CHAINjPjS1_(chain_id, tap_position, idcodes, idcode_n);
}

AJI_ERROR AJI_API c_aji_get_nodes_a(
    AJI_CHAIN_ID         chain_id,
    DWORD                tap_position,
    DWORD* idcodes,
    DWORD* idcode_n,
    DWORD* hub_info) {
	AJI_ERROR _Z13aji_get_nodesP9AJI_CHAINjPjS1_S1_(AJI_CHAIN_ID, DWORD, DWORD*, DWORD*, DWORD*);
    return _Z13aji_get_nodesP9AJI_CHAINjPjS1_S1_(chain_id, tap_position, idcodes, idcode_n, hub_info);
}

AJI_ERROR AJI_API c_aji_get_nodes_b(
    AJI_CHAIN_ID chain_id,
    DWORD  tap_position,
    AJI_HIER_ID* hier_ids,
    DWORD* hier_id_n,
    AJI_HUB_INFO* hub_infos) {
    AJI_ERROR _Z13aji_get_nodesP9AJI_CHAINjP11AJI_HIER_IDPjP12AJI_HUB_INFO(AJI_CHAIN_ID, DWORD, AJI_HIER_ID*, DWORD*, AJI_HUB_INFO*);
    return _Z13aji_get_nodesP9AJI_CHAINjP11AJI_HIER_IDPjP12AJI_HUB_INFO(chain_id, tap_position, hier_ids, hier_id_n, hub_infos);
}

AJI_ERROR c_aji_lock(AJI_OPEN_ID open_id, DWORD timeout, AJI_PACK_STYLE pack_style) {
	AJI_ERROR _Z8aji_lockP8AJI_OPENj14AJI_PACK_STYLE(AJI_OPEN_ID, DWORD, AJI_PACK_STYLE);
    return _Z8aji_lockP8AJI_OPENj14AJI_PACK_STYLE(open_id, timeout, pack_style);
}

AJI_ERROR c_aji_unlock_lock_chain(AJI_OPEN_ID unlock_id, AJI_CHAIN_ID lock_id) {
	AJI_ERROR _Z21aji_unlock_lock_chainP8AJI_OPENP9AJI_CHAIN(AJI_OPEN_ID, AJI_CHAIN_ID);
    return _Z21aji_unlock_lock_chainP8AJI_OPENP9AJI_CHAIN(unlock_id, lock_id);
}

AJI_ERROR c_aji_unlock(AJI_OPEN_ID open_id) {   
    AJI_ERROR _Z10aji_unlockP8AJI_OPEN(AJI_OPEN_ID);
    return _Z10aji_unlockP8AJI_OPEN(open_id);
}

AJI_ERROR c_aji_lock_chain(AJI_CHAIN_ID chain_id, DWORD timeout) {
    AJI_ERROR _Z14aji_lock_chainP9AJI_CHAINj(AJI_CHAIN_ID, DWORD);
    return _Z14aji_lock_chainP9AJI_CHAINj(chain_id, timeout);
}

AJI_ERROR c_aji_unlock_chain(AJI_CHAIN_ID chain_id) {
    AJI_ERROR _Z16aji_unlock_chainP9AJI_CHAIN(AJI_CHAIN_ID);
    return _Z16aji_unlock_chainP9AJI_CHAIN(chain_id);
}

AJI_ERROR c_aji_unlock_chain_lock(AJI_CHAIN_ID unlock_id, AJI_OPEN_ID lock_id, AJI_PACK_STYLE pack_style) {
    AJI_ERROR _Z21aji_unlock_chain_lockP9AJI_CHAINP8AJI_OPEN14AJI_PACK_STYLE(AJI_CHAIN_ID, AJI_OPEN_ID, AJI_PACK_STYLE);
    return _Z21aji_unlock_chain_lockP9AJI_CHAINP8AJI_OPEN14AJI_PACK_STYLE(unlock_id, lock_id, pack_style);
}

AJI_API AJI_ERROR c_aji_flush(AJI_OPEN_ID open_id) {
    AJI_ERROR _Z9aji_flushP8AJI_OPEN(AJI_OPEN_ID);
    return _Z9aji_flushP8AJI_OPEN(open_id);
}

AJI_ERROR c_aji_open_device(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID * open_id, const AJI_CLAIM * claims, DWORD claim_n, const char * application_name){
    AJI_ERROR _Z15aji_open_deviceP9AJI_CHAINjPP8AJI_OPENPK9AJI_CLAIMjPKc(AJI_CHAIN_ID, DWORD, AJI_OPEN_ID*, const AJI_CLAIM*, DWORD, const char*);
    return _Z15aji_open_deviceP9AJI_CHAINjPP8AJI_OPENPK9AJI_CLAIMjPKc(chain_id, tap_position, open_id, claims, claim_n, application_name);
}

AJI_ERROR c_aji_open_device_a(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID * open_id, const AJI_CLAIM2 * claims, DWORD claim_n, const char * application_name) {
    AJI_ERROR _Z15aji_open_deviceP9AJI_CHAINjPP8AJI_OPENPK10AJI_CLAIM2jPKc(AJI_CHAIN_ID, DWORD, AJI_OPEN_ID*, const AJI_CLAIM2*, DWORD, const char*);
    return _Z15aji_open_deviceP9AJI_CHAINjPP8AJI_OPENPK10AJI_CLAIM2jPKc(chain_id, tap_position, open_id, claims, claim_n, application_name);
} 

AJI_ERROR c_aji_close_device(AJI_OPEN_ID open_id) {
    AJI_ERROR _Z16aji_close_deviceP8AJI_OPEN(AJI_OPEN_ID);
    
    return _Z16aji_close_deviceP8AJI_OPEN(open_id);
}

AJI_ERROR c_aji_open_entire_device_chain(AJI_CHAIN_ID chain_id, AJI_OPEN_ID * open_id, AJI_CHAIN_TYPE style, const char * application_name) {
    AJI_ERROR _Z28aji_open_entire_device_chainP9AJI_CHAINPP8AJI_OPEN14AJI_CHAIN_TYPEPKc(AJI_CHAIN_ID, AJI_OPEN_ID*, const AJI_CHAIN_TYPE, const char*);
    return _Z28aji_open_entire_device_chainP9AJI_CHAINPP8AJI_OPEN14AJI_CHAIN_TYPEPKc(chain_id, open_id, style, application_name);
}

AJI_ERROR AJI_API c_aji_open_node             (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position,
                                               DWORD                idcode,
                                               AJI_OPEN_ID        * node_id,
                                               const AJI_CLAIM    * claims,
                                               DWORD                claim_n,
                                               const char         * application_name){
    AJI_ERROR _Z13aji_open_nodeP9AJI_CHAINjjPP8AJI_OPENPK9AJI_CLAIMjPKc(AJI_CHAIN_ID, DWORD, DWORD, AJI_OPEN_ID*, const AJI_CLAIM*, DWORD, const char*);
    return _Z13aji_open_nodeP9AJI_CHAINjjPP8AJI_OPENPK9AJI_CLAIMjPKc(chain_id, tap_position, idcode, node_id, claims, claim_n, application_name);
}

AJI_ERROR AJI_API c_aji_open_node_a           (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position,
                                               DWORD                node_position,
                                               DWORD                idcode,
                                               AJI_OPEN_ID        * node_id,
                                               const AJI_CLAIM    * claims,
                                               DWORD                claim_n,
                                               const char         * application_name){
    AJI_ERROR _Z13aji_open_nodeP9AJI_CHAINjjjPP8AJI_OPENPK9AJI_CLAIMjPKc(AJI_CHAIN_ID, DWORD, DWORD, DWORD, AJI_OPEN_ID*, const AJI_CLAIM*, DWORD, const char*);
    return _Z13aji_open_nodeP9AJI_CHAINjjjPP8AJI_OPENPK9AJI_CLAIMjPKc(chain_id, tap_position, node_position, idcode, node_id, claims, claim_n, application_name);
}

AJI_ERROR AJI_API c_aji_open_node_b           (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position,
                                               const AJI_HIER_ID  * hier_id,
                                               AJI_OPEN_ID        * node_id,
                                               const AJI_CLAIM2   * claims,
                                               DWORD                claim_n,
                                               const char         * application_name){
    AJI_ERROR _Z13aji_open_nodeP9AJI_CHAINjPK11AJI_HIER_IDPP8AJI_OPENPK10AJI_CLAIM2jPKc(AJI_CHAIN_ID, DWORD, const AJI_HIER_ID*, AJI_OPEN_ID*, const AJI_CLAIM2*, DWORD, const char*);
    return _Z13aji_open_nodeP9AJI_CHAINjPK11AJI_HIER_IDPP8AJI_OPENPK10AJI_CLAIM2jPKc(chain_id, tap_position, hier_id, node_id, claims, claim_n, application_name);
}

AJI_ERROR c_aji_test_logic_reset(AJI_OPEN_ID open_id) {
    AJI_ERROR _Z20aji_test_logic_resetP8AJI_OPEN(AJI_OPEN_ID*);
    return _Z20aji_test_logic_resetP8AJI_OPEN(open_id);
}

AJI_ERROR c_aji_delay(AJI_OPEN_ID open_id, DWORD timeout_microseconds) {
    AJI_ERROR _Z9aji_delayP8AJI_OPENj(AJI_OPEN_ID, DWORD);
    return _Z9aji_delayP8AJI_OPENj(open_id, timeout_microseconds);
}

AJI_ERROR c_aji_run_test_idle(AJI_OPEN_ID open_id, DWORD num_clocks) {
    AJI_ERROR _Z17aji_run_test_idleP8AJI_OPENj(AJI_OPEN_ID, DWORD);
    return _Z17aji_run_test_idleP8AJI_OPENj(open_id, num_clocks);
}

AJI_ERROR c_aji_run_test_idle_a(AJI_OPEN_ID open_id, DWORD num_clocks, DWORD flags) {
    AJI_ERROR _Z17aji_run_test_idleP8AJI_OPENjj(AJI_OPEN_ID, DWORD, DWORD);
    return _Z17aji_run_test_idleP8AJI_OPENjj(open_id, num_clocks, flags);
}

AJI_ERROR c_aji_access_ir(AJI_OPEN_ID open_id, DWORD instruction, DWORD * captured_ir, DWORD flags) {
	AJI_ERROR _Z13aji_access_irP8AJI_OPENjPjj(AJI_OPEN_ID, DWORD, DWORD*, DWORD);
    return _Z13aji_access_irP8AJI_OPENjPjj(open_id, instruction, captured_ir, flags);
}

AJI_ERROR c_aji_access_ir_a(AJI_OPEN_ID open_id, DWORD length_ir, const BYTE * write_bits, BYTE * read_bits, DWORD flags) {
    AJI_ERROR _Z13aji_access_irP8AJI_OPENjPKhPhj(AJI_OPEN_ID, DWORD, const BYTE*, BYTE*, DWORD);
    return _Z13aji_access_irP8AJI_OPENjPKhPhj(open_id, length_ir, write_bits, read_bits, flags);
}

AJI_ERROR c_aji_access_dr(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD read_offset, DWORD read_length, BYTE * read_bits) {
    AJI_ERROR _Z13aji_access_drP8AJI_OPENjjjjPKhjjPh(AJI_OPEN_ID, DWORD, DWORD, DWORD, DWORD, const BYTE*, DWORD, DWORD, BYTE*);
    return _Z13aji_access_drP8AJI_OPENjjjjPKhjjPh(open_id, length_dr, flags, write_offset, write_length, write_bits, read_offset, read_length, read_bits);
}

AJI_ERROR c_aji_access_dr_a(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD read_offset, DWORD read_length, BYTE * read_bits, DWORD batch) {
	AJI_ERROR _Z13aji_access_drP8AJI_OPENjjjjPKhjjPhj(AJI_OPEN_ID, DWORD, DWORD, DWORD, DWORD, const BYTE*, DWORD, DWORD, BYTE*, DWORD);
    return _Z13aji_access_drP8AJI_OPENjjjjPKhjjPhj(open_id, length_dr, flags, write_offset, write_length, write_bits, read_offset, read_length, read_bits, batch);
}

AJI_API AJI_ERROR c_aji_access_overlay(AJI_OPEN_ID node_id, DWORD overlay, DWORD* captured_overlay){
    AJI_ERROR _Z18aji_access_overlayP8AJI_OPENjPj(AJI_OPEN_ID, DWORD, DWORD*);
    return _Z18aji_access_overlayP8AJI_OPENjPj(node_id, overlay, captured_overlay);

}
#endif //IS_WIN32
