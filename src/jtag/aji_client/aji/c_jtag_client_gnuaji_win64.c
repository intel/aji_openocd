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

#define LIBRARY_NAME_JTAG_CLIENT__MINGW64 "libaji_client.dll"


HINSTANCE c_jtag_client_lib;


AJI_ERROR c_jtag_client_gnuaji_init(void) {
    /* Intel policy strike ...
     * To prevent DLL hijacking we must only provide absolute path
     * to a "safe" path that cannot be modified to the LoadLibrary(). 
     * The only safe path that we can guarantee to be available is 
     * where the program reside, so we we must have all dynamically
     * loaded DLL there.
     */
    char fullpathname[256];
        
#if 1 //Intel security policy
    { //dummy block
    char name[256];
    char* leaf;

    GetModuleFileName(NULL, name, sizeof(name) - 2);
    GetFullPathName(name, sizeof(fullpathname), fullpathname, &leaf);
    *leaf = 0;
    if (leaf < fullpathname + sizeof(fullpathname) - (sizeof(LIBRARY_NAME_JTAG_CLIENT__MINGW64) + 1)) {
        strcat_s(fullpathname, sizeof(fullpathname), LIBRARY_NAME_JTAG_CLIENT__MINGW64);
    }
    else {
        LOG_ERROR("Cannot accommodate DLL filename with full path: Buffer too small\n"  
                  "Buffer size is %lld, and cannot accommodate '%s%s' because it is requires %lld\n",
                   sizeof(fullpathname), fullpathname, LIBRARY_NAME_JTAG_CLIENT__MINGW64,
                   strlen(fullpathname) + sizeof(LIBRARY_NAME_JTAG_CLIENT__MINGW64)+1
        );
        return AJI_FAILURE;
    }

    if (PathIsRelativeA(fullpathname)) {
        LOG_ERROR("Path name for DLL ('%s') cannot be a relative path.\n", fullpathname);
        return AJI_FAILURE;
    }
    } //dummy block
#else
    { // dummy block
        strncpy(fullpathname, 
                LIBRARY_NAME_JTAG_CLIENT__MINGW64, 
                sizeof(LIBRARY_NAME_JTAG_CLIENT__MINGW64)+1
        );
    } //dummy block
#endif //INTEL SECURITY POLICY

    LOG_DEBUG("Loading %s\n", fullpathname);
    c_jtag_client_lib = LoadLibrary(TEXT(fullpathname));
    if (c_jtag_client_lib == NULL) {
        //fprintf(stderr, "Cannot find %s.", fullpathname);
        return AJI_FAILURE;
    }
    return AJI_NO_ERROR;
}

AJI_ERROR c_jtag_client_gnuaji_free(void) {
    if (c_jtag_client_lib != NULL) {
        FreeLibrary(c_jtag_client_lib);
    }
    c_jtag_client_lib = NULL;
    return AJI_NO_ERROR;
}


#define FNAME_AJI_GET_HARDWARE__MINGW64 "_Z16aji_get_hardwarePmP12AJI_HARDWAREm"
AJI_ERROR c_aji_get_hardware(DWORD * hardware_count, AJI_HARDWARE * hardware_list, DWORD timeout) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(DWORD*, AJI_HARDWARE*, DWORD);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_GET_HARDWARE__MINGW64); //extra (void*) cast to prevent warning re casting from AJI_ERROR aji_get_hardware(*)
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(hardware_count, hardware_list, timeout);
}

#define FNAME_AJI_GET_HARDWARE2__MINGW64 "_Z17aji_get_hardware2PmP12AJI_HARDWAREPPcm"
AJI_ERROR c_aji_get_hardware2(DWORD * hardware_count, AJI_HARDWARE * hardware_list, char **server_version_info_list, DWORD timeout) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(DWORD*, AJI_HARDWARE*, char**, DWORD);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib,FNAME_AJI_GET_HARDWARE2__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(hardware_count, hardware_list, server_version_info_list, timeout);
} 

#define FNAME_AJI_FIND_HARDWARE__MINGW64 "_Z17aji_find_hardwaremP12AJI_HARDWAREm"
AJI_ERROR c_aji_find_hardware(DWORD persistent_id, AJI_HARDWARE * hardware, DWORD timeout) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(DWORD, AJI_HARDWARE*, DWORD);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_FIND_HARDWARE__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(persistent_id, hardware, timeout);
}

#define FNAME_AJI_FIND_HARDWARE_A__MINGW64 "_Z17aji_find_hardwarePKcP12AJI_HARDWAREm"
AJI_ERROR c_aji_find_hardware_a(const char * hw_name, AJI_HARDWARE * hardware, DWORD timeout) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(const char*, AJI_HARDWARE*, DWORD);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_FIND_HARDWARE_A__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(hw_name, hardware, timeout);
}

#define FNAME_AJI_READ_DEVICE_CHAIN__MINGW64 "_Z21aji_read_device_chainP9AJI_CHAINPmP10AJI_DEVICEb"
AJI_ERROR c_aji_read_device_chain(AJI_CHAIN_ID chain_id, DWORD * device_count, AJI_DEVICE * device_list, _Bool auto_scan) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID, DWORD*, AJI_DEVICE*, _Bool);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_READ_DEVICE_CHAIN__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, device_count, device_list, auto_scan);
}


#define FNAME_AJI_GET_NODES__MINGW64 "_Z13aji_get_nodesP9AJI_CHAINmPmS1_"
AJI_ERROR AJI_API c_aji_get_nodes(
    AJI_CHAIN_ID         chain_id,
    DWORD                tap_position,
    DWORD* idcodes,
    DWORD* idcode_n) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, DWORD, DWORD*, DWORD*);
    ProdFn pfn = (ProdFn)(void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_GET_NODES__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, idcodes, idcode_n);
}

#define FNAME_AJI_GET_NODES_A__MINGW64 "_Z13aji_get_nodesP9AJI_CHAINmPmS1_S1_"
AJI_ERROR AJI_API c_aji_get_nodes_a(
    AJI_CHAIN_ID         chain_id,
    DWORD                tap_position,
    DWORD* idcodes,
    DWORD* idcode_n,
    DWORD* hub_info) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, DWORD, DWORD*, DWORD*, DWORD*);
    ProdFn pfn = (ProdFn)(void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_GET_NODES_A__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, idcodes, idcode_n, hub_info);
}


#define FNAME_AJI_GET_NODES_B__MINGW64 "_Z13aji_get_nodesP9AJI_CHAINmP11AJI_HIER_IDPmP12AJI_HUB_INFO"
AJI_ERROR AJI_API c_aji_get_nodes_b(
    AJI_CHAIN_ID chain_id,
    DWORD  tap_position,
    AJI_HIER_ID* hier_ids,
    DWORD* hier_id_n,
    AJI_HUB_INFO* hub_infos) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, DWORD, AJI_HIER_ID*, DWORD*, AJI_HUB_INFO*);
    ProdFn pfn = (ProdFn)(void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_GET_NODES_B__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, hier_ids, hier_id_n, hub_infos);
}


#define FNAME_AJI_LOCK__MINGW64 "_Z8aji_lockP8AJI_OPENm14AJI_PACK_STYLE"
AJI_ERROR c_aji_lock(AJI_OPEN_ID open_id, DWORD timeout, AJI_PACK_STYLE pack_style) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID, DWORD, AJI_PACK_STYLE);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_LOCK__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, timeout, pack_style);
}

#define FNAME_AJI_UNLOCK_LOCK_CHAIN__MINGW64 "_Z21aji_unlock_lock_chainP8AJI_OPENP9AJI_CHAIN"
AJI_ERROR c_aji_unlock_lock_chain(AJI_OPEN_ID unlock_id, AJI_CHAIN_ID lock_id) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID, AJI_CHAIN_ID);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_UNLOCK_LOCK_CHAIN__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(unlock_id, lock_id);
}

#define FNAME_AJI_UNLOCK__MINGW64 "_Z10aji_unlockP8AJI_OPEN"
AJI_ERROR c_aji_unlock(AJI_OPEN_ID open_id) {   
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_UNLOCK__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id);
}

#define FNAME_AJI_LOCK_CHAIN__MINGW64 "_Z14aji_lock_chainP9AJI_CHAINm"
AJI_ERROR c_aji_lock_chain(AJI_CHAIN_ID chain_id, DWORD timeout) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID, DWORD);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_LOCK_CHAIN__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, timeout);
}

#define FNAME_AJI_UNLOCK_CHAIN__MINGW64 "_Z16aji_unlock_chainP9AJI_CHAIN"
AJI_ERROR c_aji_unlock_chain(AJI_CHAIN_ID chain_id) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_UNLOCK_CHAIN__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id);
}

#define FNAME_AJI_UNLOCK_CHAIN_LOCK__MINGW64 "_Z21aji_unlock_chain_lockP9AJI_CHAINP8AJI_OPEN14AJI_PACK_STYLE"
AJI_ERROR c_aji_unlock_chain_lock(AJI_CHAIN_ID unlock_id, AJI_OPEN_ID lock_id, AJI_PACK_STYLE pack_style) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, AJI_OPEN_ID, AJI_PACK_STYLE);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_UNLOCK_CHAIN_LOCK__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(unlock_id, lock_id, pack_style);
}

#define FNAME_AJI_FLUSH__MINGW64 "_Z9aji_flushP8AJI_OPEN"
AJI_API AJI_ERROR c_aji_flush(AJI_OPEN_ID open_id) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID);
    ProdFn pfn = (ProdFn)(void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_FLUSH__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id);
}

#define FNAME_AJI_OPEN_DEVICE__MINGW64 "_Z15aji_open_deviceP9AJI_CHAINmPP8AJI_OPENPK9AJI_CLAIMmPKc"
AJI_ERROR c_aji_open_device(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID * open_id, const AJI_CLAIM * claims, DWORD claim_n, const char * application_name){
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID, DWORD, AJI_OPEN_ID*, const AJI_CLAIM*, DWORD, const char*);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_OPEN_DEVICE__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, open_id, claims, claim_n, application_name);
}

#define FNAME_AJI_OPEN_DEVICE_A__MINGW64 "_Z15aji_open_deviceP9AJI_CHAINmPP8AJI_OPENPK10AJI_CLAIM2mPKc"
AJI_ERROR c_aji_open_device_a(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID * open_id, const AJI_CLAIM2 * claims, DWORD claim_n, const char * application_name) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID, DWORD, AJI_OPEN_ID*, const AJI_CLAIM2*, DWORD, const char*);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_OPEN_DEVICE__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, open_id, claims, claim_n, application_name);
} 

#define FNAME_AJI_CLOSE_DEVICE__MINGW64 "_Z16aji_close_deviceP8AJI_OPEN"
AJI_ERROR c_aji_close_device(AJI_OPEN_ID open_id) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_CLOSE_DEVICE__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id);
}

#define FNAME_AJI_OPEN_ENTIRE_DEVICE_CHAIN__MINGW64 "_Z28aji_open_entire_device_chainP9AJI_CHAINPP8AJI_OPEN14AJI_CHAIN_TYPEPKc"
AJI_ERROR c_aji_open_entire_device_chain(AJI_CHAIN_ID chain_id, AJI_OPEN_ID * open_id, AJI_CHAIN_TYPE style, const char * application_name) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID, AJI_OPEN_ID*, const AJI_CHAIN_TYPE, const char*);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_OPEN_ENTIRE_DEVICE_CHAIN__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, open_id, style, application_name);
}

#define FNAME_AJI_OPEN_NODE__MINGW64 "_Z13aji_open_nodeP9AJI_CHAINmmPP8AJI_OPENPK9AJI_CLAIMmPKc"
AJI_ERROR c_aji_open_node(    AJI_CHAIN_ID chain_id,
                              DWORD tap_position,
                              DWORD idcode,
                              AJI_OPEN_ID* node_id,
                              const AJI_CLAIM* claims,
                              DWORD claim_n,
                              const char* application_name) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, DWORD, DWORD, AJI_OPEN_ID*, const AJI_CLAIM*, DWORD, const char*);
    ProdFn pfn = (ProdFn)(void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_OPEN_NODE__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, idcode, node_id, claims, claim_n, application_name);
}

#define FNAME_AJI_OPEN_NODE_A__MINGW64 "_Z13aji_open_nodeP9AJI_CHAINmmmPP8AJI_OPENPK9AJI_CLAIMmPKc"
AJI_ERROR c_aji_open_node_a(  AJI_CHAIN_ID chain_id,
                              DWORD tap_position, 
                              DWORD node_position, 
                              DWORD idcode, 
                              AJI_OPEN_ID *node_id, 
                              const AJI_CLAIM *claims, 
                              DWORD claim_n, 
                              const char *application_name) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, DWORD, DWORD, DWORD, AJI_OPEN_ID*, const AJI_CLAIM*, DWORD, const char*);
    ProdFn pfn = (ProdFn)(void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_OPEN_NODE_A__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, node_position, idcode, node_id, claims, claim_n, application_name);
}

#define FNAME_AJI_OPEN_NODE_B__MINGW64 "_Z13aji_open_nodeP9AJI_CHAINmPK11AJI_HIER_IDPP8AJI_OPENPK10AJI_CLAIM2mPKc"
AJI_ERROR c_aji_open_node_b(  AJI_CHAIN_ID chain_id,
                              DWORD tap_position, 
                              const AJI_HIER_ID *hier_id, 
                              AJI_OPEN_ID *node_id,  
                              const AJI_CLAIM2 *claims, 
                              DWORD claim_n, 
                              const char* application_name) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, DWORD, const AJI_HIER_ID*, AJI_OPEN_ID*, const AJI_CLAIM2*, DWORD, const char*);
    ProdFn pfn = (ProdFn)(void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_OPEN_NODE_B__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, hier_id, node_id, claims, claim_n, application_name);
}

#define FNAME_AJI_TEST_LOGIC_RESET__MINGW64 "_Z20aji_test_logic_resetP8AJI_OPEN"
AJI_ERROR c_aji_test_logic_reset(AJI_OPEN_ID open_id) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID*);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_TEST_LOGIC_RESET__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id);
}

#define FNAME_AJI_DELAY__MINGW64 "_Z9aji_delayP8AJI_OPENm"
AJI_ERROR c_aji_delay(AJI_OPEN_ID open_id, DWORD timeout_microseconds) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID, DWORD);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_DELAY__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, timeout_microseconds);
}

#define FNAME_AJI_RUN_TEST_IDLE__MINGW64 "_Z17aji_run_test_idleP8AJI_OPENm"
AJI_ERROR c_aji_run_test_idle(AJI_OPEN_ID open_id, DWORD num_clocks) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD);
    ProdFn pfn = (ProdFn) (void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_RUN_TEST_IDLE__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, num_clocks);
}

#define FNAME_AJI_RUN_TEST_IDLE_A__MINGW64 "_Z17aji_run_test_idleP8AJI_OPENmm"
AJI_ERROR c_aji_run_test_idle_a(AJI_OPEN_ID open_id, DWORD num_clocks, DWORD flags) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, DWORD);
    ProdFn pfn = (ProdFn) (void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_RUN_TEST_IDLE_A__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, num_clocks, flags);
}

#define FNAME_AJI_ACCESS_IR__MINGW64 "_Z13aji_access_irP8AJI_OPENmPmm"
AJI_ERROR c_aji_access_ir(AJI_OPEN_ID open_id, DWORD instruction, DWORD * captured_ir, DWORD flags) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, DWORD*, DWORD);
    ProdFn pfn = (ProdFn) (void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_ACCESS_IR__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, instruction, captured_ir, flags);
}

#define FNAME_AJI_ACCESS_IR_A__MINGW64 "_Z13aji_access_irP8AJI_OPENmPKhPhm"
AJI_ERROR c_aji_access_ir_a(AJI_OPEN_ID open_id, DWORD length_ir, const BYTE * write_bits, BYTE * read_bits, DWORD flags) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, const BYTE*, BYTE*, DWORD);
    ProdFn pfn = (ProdFn) (void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_ACCESS_IR_A__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, length_ir, write_bits, read_bits, flags);
}

#define FNAME_AJI_ACCESS_IR_MULTIPLE__MINGW64 "_Z22aji_access_ir_multiplemPKP8AJI_OPENPKmPm"
AJI_ERROR c_aji_access_ir_multiple(DWORD num_devices, const AJI_OPEN_ID * open_id, const DWORD * instructions, DWORD * captured_irs) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(DWORD, const AJI_OPEN_ID*, const DWORD*, DWORD*);
    ProdFn pfn = (ProdFn) (void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_ACCESS_IR_MULTIPLE__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(num_devices, open_id, instructions, captured_irs);
}

#define FNAME_AJI_ACCESS_DR__MINGW64 "_Z13aji_access_drP8AJI_OPENmmmmPKhmmPh"
AJI_ERROR c_aji_access_dr(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD read_offset, DWORD read_length, BYTE * read_bits) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, DWORD, DWORD, DWORD, const BYTE*, DWORD, DWORD, BYTE*);
    ProdFn pfn = (ProdFn) (void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_ACCESS_DR__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, length_dr, flags, write_offset, write_length, write_bits, read_offset, read_length, read_bits);
}

#define FNAME_AJI_ACCESS_DR_A__MINGW64 "_Z13aji_access_drP8AJI_OPENmmmmPKhmmPhm"
AJI_ERROR c_aji_access_dr_a(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD read_offset, DWORD read_length, BYTE * read_bits, DWORD batch) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, DWORD, DWORD, DWORD, const BYTE*, DWORD, DWORD, BYTE*, DWORD);
    ProdFn pfn = (ProdFn) (void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_ACCESS_DR__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, length_dr, flags, write_offset, write_length, write_bits, read_offset, read_length, read_bits, batch);
}


#define FNAME_AJI_ACCESS_OVERLAY__MINGW64 "_Z18aji_access_overlayP8AJI_OPENmPm"
AJI_API AJI_ERROR c_aji_access_overlay(AJI_OPEN_ID node_id, DWORD overlay, DWORD* captured_overlay){
assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, DWORD*);
    ProdFn pfn = (ProdFn)(void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_ACCESS_OVERLAY__MINGW64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(node_id, overlay, captured_overlay);
}
