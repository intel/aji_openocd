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

#if IS_WIN32


#include "c_aji.h"
#include "c_jtag_client_gnuaji_win64.h"

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

/*
 * On mingw64 command line:
 * ##To find the find decorated names in a DLL
 * $  /c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio/2019/Professional/VC/Tools/MSVC/14.26.28801/bin/Hostx64/x64/dumpbin.exe -EXPORTS jtag_client.dll
 * ...
 *  ordinal hint RVA      name
 *  1    0 0000BE10 ?aji_access_dr@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@KKKKPEBEKKPEAE@Z
 *  2    1 0000BF20 ?aji_access_dr@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@KKKKPEBEKKPEAEK@Z
 *  3    2 0000C030 ?aji_access_dr@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@KKKKPEBEKKPEAEKPEAK@Z
 * ...
 *
 * ## To undecorate a name, e.g. to find out how to undecorate/demangle 
 * $ /c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio/2019/Professional/VC/Tools/MSVC/14.26.28801/bin/Hostx64/x64/undname.exe \
 *       '?aji_access_dr@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@KKKKPEBEKKPEAEKPEAK@Z'
 * Microsoft (R) C++ Name Undecorator
 * Copyright (C) Microsoft Corporation. All rights reserved.

 * Undecoration of :- "?aji_access_dr@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@KKKKPEBEKKPEAEKPEAK@Z" * is :
 * - "enum AJI_ERROR __cdecl aji_access_dr(class AJI_OPEN * __ptr64,unsigned long,unsigned long,unsigned long,unsigned long, 
 * unsigned char const * __ptr64,unsigned long,unsigned long,unsigned char * __ptr64,unsigned long,unsigned long * __ptr64)"
 *
 * 
 * The other news is ...
 *   The mangled name above is only correct if it is compiled by MSVC. For libjtag_client.dll compiled by gcc in mingw64,
 *   another mangled name scheme is used. 
 */

#include <winsock2.h>
#include <windows.h>
#include <shlwapi.h>

#include "log.h"
#include "c_aji.h"


AJI_ERROR c_jtag_client_gnuaji_init(void) {
    return AJI_NO_ERROR;
}

AJI_ERROR c_jtag_client_gnuaji_free(void) {
    return AJI_NO_ERROR;
}


AJI_ERROR c_aji_get_hardware(DWORD* hardware_count, AJI_HARDWARE* hardware_list, DWORD timeout) {
    AJI_ERROR _Z16aji_get_hardwarePmP12AJI_HARDWAREm(DWORD * hardware_count, AJI_HARDWARE * hardware_list, DWORD timeout);
    return _Z16aji_get_hardwarePmP12AJI_HARDWAREm(hardware_count, hardware_list, timeout);
}

AJI_ERROR c_aji_get_hardware2(DWORD* hardware_count, AJI_HARDWARE* hardware_list, char** server_version_info_list, DWORD timeout) {
    AJI_ERROR _Z17aji_get_hardware2PmP12AJI_HARDWAREPPcm(DWORD*, AJI_HARDWARE*, char**, DWORD);
    return _Z17aji_get_hardware2PmP12AJI_HARDWAREPPcm(hardware_count, hardware_list, server_version_info_list, timeout);
}

AJI_ERROR c_aji_find_hardware(DWORD persistent_id, AJI_HARDWARE* hardware, DWORD timeout) {
    AJI_ERROR _Z17aji_find_hardwaremP12AJI_HARDWAREm(DWORD, AJI_HARDWARE*, DWORD);
    return _Z17aji_find_hardwaremP12AJI_HARDWAREm(persistent_id, hardware, timeout);
}

AJI_ERROR c_aji_find_hardware_a(const char* hw_name, AJI_HARDWARE* hardware, DWORD timeout) {
    AJI_ERROR _Z17aji_find_hardwarePKcP12AJI_HARDWAREm(const char*, AJI_HARDWARE*, DWORD);
    return _Z17aji_find_hardwarePKcP12AJI_HARDWAREm(hw_name, hardware, timeout);
}

AJI_ERROR c_aji_read_device_chain(AJI_CHAIN_ID chain_id, DWORD* device_count, AJI_DEVICE* device_list, _Bool auto_scan) {
    AJI_ERROR _Z21aji_read_device_chainP9AJI_CHAINPmP10AJI_DEVICEb(AJI_CHAIN_ID, DWORD*, AJI_DEVICE*, _Bool);
    return _Z21aji_read_device_chainP9AJI_CHAINPmP10AJI_DEVICEb(chain_id, device_count, device_list, auto_scan);
}

AJI_ERROR AJI_API c_aji_get_nodes(
    AJI_CHAIN_ID         chain_id,
    DWORD                tap_position,
    DWORD* idcodes,
    DWORD* idcode_n) {
    AJI_ERROR _Z13aji_get_nodesP9AJI_CHAINmPmS1_(AJI_CHAIN_ID, DWORD, DWORD*, DWORD*);
    return _Z13aji_get_nodesP9AJI_CHAINmPmS1_(chain_id, tap_position, idcodes, idcode_n);
}

AJI_ERROR AJI_API c_aji_get_nodes_a(
    AJI_CHAIN_ID         chain_id,
    DWORD                tap_position,
    DWORD* idcodes,
    DWORD* idcode_n,
    DWORD* hub_info) {
    AJI_ERROR _Z13aji_get_nodesP9AJI_CHAINmPmS1_S1_(AJI_CHAIN_ID, DWORD, DWORD*, DWORD*, DWORD*);
    return _Z13aji_get_nodesP9AJI_CHAINmPmS1_S1_(chain_id, tap_position, idcodes, idcode_n, hub_info);
}

AJI_ERROR AJI_API c_aji_get_nodes_b(
    AJI_CHAIN_ID chain_id,
    DWORD  tap_position,
    AJI_HIER_ID* hier_ids,
    DWORD* hier_id_n,
    AJI_HUB_INFO* hub_infos) {
    AJI_ERROR _Z13aji_get_nodesP9AJI_CHAINmP11AJI_HIER_IDPmP12AJI_HUB_INFO(AJI_CHAIN_ID, DWORD, AJI_HIER_ID*, DWORD*, AJI_HUB_INFO*);
    return _Z13aji_get_nodesP9AJI_CHAINmP11AJI_HIER_IDPmP12AJI_HUB_INFO(chain_id, tap_position, hier_ids, hier_id_n, hub_infos);
}

AJI_ERROR AJI_API c_aji_get_nodes_bi(
    AJI_CHAIN_ID chain_id,
    DWORD  tap_position,
    AJI_HIER_ID* hier_ids,
    DWORD* hier_id_n,
    AJI_HUB_INFO* hub_infos) {
    AJI_ERROR _Z13aji_get_nodesP9AJI_CHAINmP11AJI_HIER_IDPmP12AJI_HUB_INFOi(AJI_CHAIN_ID, DWORD, AJI_HIER_ID*, DWORD*, AJI_HUB_INFO*, int);
    return _Z13aji_get_nodesP9AJI_CHAINmP11AJI_HIER_IDPmP12AJI_HUB_INFOi(chain_id, tap_position, hier_ids, hier_id_n, hub_infos, 1);
}

AJI_ERROR c_aji_lock(AJI_OPEN_ID open_id, DWORD timeout, AJI_PACK_STYLE pack_style) {
    AJI_ERROR _Z8aji_lockP8AJI_OPENm14AJI_PACK_STYLE(AJI_OPEN_ID, DWORD, AJI_PACK_STYLE);
    return _Z8aji_lockP8AJI_OPENm14AJI_PACK_STYLE(open_id, timeout, pack_style);
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
    AJI_ERROR _Z14aji_lock_chainP9AJI_CHAINm(AJI_CHAIN_ID, DWORD);
    return _Z14aji_lock_chainP9AJI_CHAINm(chain_id, timeout);
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

AJI_ERROR c_aji_open_device(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID* open_id, const AJI_CLAIM* claims, DWORD claim_n, const char* application_name) {
    AJI_ERROR _Z15aji_open_deviceP9AJI_CHAINmPP8AJI_OPENPK9AJI_CLAIMmPKc(AJI_CHAIN_ID, DWORD, AJI_OPEN_ID*, const AJI_CLAIM*, DWORD, const char*);
    return _Z15aji_open_deviceP9AJI_CHAINmPP8AJI_OPENPK9AJI_CLAIMmPKc(chain_id, tap_position, open_id, claims, claim_n, application_name);
}

AJI_ERROR c_aji_open_device_a(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID* open_id, const AJI_CLAIM2* claims, DWORD claim_n, const char* application_name) {
    AJI_ERROR _Z15aji_open_deviceP9AJI_CHAINmPP8AJI_OPENPK10AJI_CLAIM2mPKc(AJI_CHAIN_ID, DWORD, AJI_OPEN_ID*, const AJI_CLAIM2*, DWORD, const char*);
    return _Z15aji_open_deviceP9AJI_CHAINmPP8AJI_OPENPK10AJI_CLAIM2mPKc(chain_id, tap_position, open_id, claims, claim_n, application_name);
}

AJI_ERROR c_aji_close_device(AJI_OPEN_ID open_id) {
    AJI_ERROR _Z16aji_close_deviceP8AJI_OPEN(AJI_OPEN_ID);
    return _Z16aji_close_deviceP8AJI_OPEN(open_id);
}

AJI_ERROR c_aji_open_entire_device_chain(AJI_CHAIN_ID chain_id, AJI_OPEN_ID* open_id, AJI_CHAIN_TYPE style, const char* application_name) {
    AJI_ERROR _Z28aji_open_entire_device_chainP9AJI_CHAINPP8AJI_OPEN14AJI_CHAIN_TYPEPKc(AJI_CHAIN_ID, AJI_OPEN_ID*, const AJI_CHAIN_TYPE, const char*);
    return _Z28aji_open_entire_device_chainP9AJI_CHAINPP8AJI_OPEN14AJI_CHAIN_TYPEPKc(chain_id, open_id, style, application_name);
}

AJI_ERROR c_aji_open_node(AJI_CHAIN_ID chain_id,
    DWORD tap_position,
    DWORD idcode,
    AJI_OPEN_ID* node_id,
    const AJI_CLAIM* claims,
    DWORD claim_n,
    const char* application_name) {
    AJI_ERROR _Z13aji_open_nodeP9AJI_CHAINmmPP8AJI_OPENPK9AJI_CLAIMmPKc(AJI_CHAIN_ID, DWORD, DWORD, AJI_OPEN_ID*, const AJI_CLAIM*, DWORD, const char*);
    return _Z13aji_open_nodeP9AJI_CHAINmmPP8AJI_OPENPK9AJI_CLAIMmPKc(chain_id, tap_position, idcode, node_id, claims, claim_n, application_name);
}

AJI_ERROR c_aji_open_node_a(AJI_CHAIN_ID chain_id,
    DWORD tap_position,
    DWORD node_position,
    DWORD idcode,
    AJI_OPEN_ID* node_id,
    const AJI_CLAIM* claims,
    DWORD claim_n,
    const char* application_name) {
    AJI_ERROR _Z13aji_open_nodeP9AJI_CHAINmmmPP8AJI_OPENPK9AJI_CLAIMmPKc(AJI_CHAIN_ID, DWORD, DWORD, DWORD, AJI_OPEN_ID*, const AJI_CLAIM*, DWORD, const char*);
    return _Z13aji_open_nodeP9AJI_CHAINmmmPP8AJI_OPENPK9AJI_CLAIMmPKc(chain_id, tap_position, node_position, idcode, node_id, claims, claim_n, application_name);
}

AJI_ERROR c_aji_open_node_b(AJI_CHAIN_ID chain_id,
    DWORD tap_position,
    const AJI_HIER_ID* hier_id,
    AJI_OPEN_ID* node_id,
    const AJI_CLAIM2* claims,
    DWORD claim_n,
    const char* application_name) {
    AJI_ERROR _Z13aji_open_nodeP9AJI_CHAINmPK11AJI_HIER_IDPP8AJI_OPENPK10AJI_CLAIM2mPKc(AJI_CHAIN_ID, DWORD, const AJI_HIER_ID*, AJI_OPEN_ID*, const AJI_CLAIM2*, DWORD, const char*);
    return _Z13aji_open_nodeP9AJI_CHAINmPK11AJI_HIER_IDPP8AJI_OPENPK10AJI_CLAIM2mPKc(chain_id, tap_position, hier_id, node_id, claims, claim_n, application_name);
}

AJI_ERROR c_aji_test_logic_reset(AJI_OPEN_ID open_id) {
    AJI_ERROR _Z20aji_test_logic_resetP8AJI_OPEN(AJI_OPEN_ID*);
    return _Z20aji_test_logic_resetP8AJI_OPEN(open_id);
}

AJI_ERROR c_aji_delay(AJI_OPEN_ID open_id, DWORD timeout_microseconds) {
    AJI_ERROR _Z9aji_delayP8AJI_OPENm(AJI_OPEN_ID, DWORD);
    return _Z9aji_delayP8AJI_OPENm(open_id, timeout_microseconds);
}

AJI_ERROR c_aji_run_test_idle(AJI_OPEN_ID open_id, DWORD num_clocks) {
    AJI_ERROR _Z17aji_run_test_idleP8AJI_OPENm(AJI_OPEN_ID, DWORD);
    return _Z17aji_run_test_idleP8AJI_OPENm(open_id, num_clocks);
}

AJI_ERROR c_aji_run_test_idle_a(AJI_OPEN_ID open_id, DWORD num_clocks, DWORD flags) {
    AJI_ERROR _Z17aji_run_test_idleP8AJI_OPENmm(AJI_OPEN_ID, DWORD, DWORD);
    return _Z17aji_run_test_idleP8AJI_OPENmm(open_id, num_clocks, flags);
}

AJI_ERROR c_aji_access_ir(AJI_OPEN_ID open_id, DWORD instruction, DWORD* captured_ir, DWORD flags) {
    AJI_ERROR _Z13aji_access_irP8AJI_OPENmPmm(AJI_OPEN_ID, DWORD, DWORD*, DWORD);
    return _Z13aji_access_irP8AJI_OPENmPmm(open_id, instruction, captured_ir, flags);
}

AJI_ERROR c_aji_access_ir_a(AJI_OPEN_ID open_id, DWORD length_ir, const BYTE* write_bits, BYTE* read_bits, DWORD flags) {
    AJI_ERROR _Z13aji_access_irP8AJI_OPENmPKhPhm(AJI_OPEN_ID, DWORD, const BYTE*, BYTE*, DWORD);
    return _Z13aji_access_irP8AJI_OPENmPKhPhm(open_id, length_ir, write_bits, read_bits, flags);
}

AJI_ERROR c_aji_access_dr(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE* write_bits, DWORD read_offset, DWORD read_length, BYTE* read_bits) {
    AJI_ERROR _Z13aji_access_drP8AJI_OPENmmmmPKhmmPh(AJI_OPEN_ID, DWORD, DWORD, DWORD, DWORD, const BYTE*, DWORD, DWORD, BYTE*);
    return _Z13aji_access_drP8AJI_OPENmmmmPKhmmPh(open_id, length_dr, flags, write_offset, write_length, write_bits, read_offset, read_length, read_bits);
}

AJI_ERROR c_aji_access_dr_a(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE* write_bits, DWORD read_offset, DWORD read_length, BYTE* read_bits, DWORD batch) {
    AJI_ERROR _Z13aji_access_drP8AJI_OPENmmmmPKhmmPhm(AJI_OPEN_ID, DWORD, DWORD, DWORD, DWORD, const BYTE*, DWORD, DWORD, BYTE*, DWORD);
    return _Z13aji_access_drP8AJI_OPENmmmmPKhmmPhm(open_id, length_dr, flags, write_offset, write_length, write_bits, read_offset, read_length, read_bits, batch);
}

AJI_API AJI_ERROR c_aji_access_overlay(AJI_OPEN_ID node_id, DWORD overlay, DWORD* captured_overlay) {
    AJI_ERROR _Z18aji_access_overlayP8AJI_OPENmPm(AJI_OPEN_ID, DWORD, DWORD*);
    return _Z18aji_access_overlayP8AJI_OPENmPm(node_id, overlay, captured_overlay);
}
#endif //IS_WIN32
