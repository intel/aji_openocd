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


#define LIBRARY_NAME_SAFESTRING__LINUX "libSafeString.so"
#define LIBRARY_NAME_JTAG_CLIENT__LINUX "libaji_client.so"

#define MAX_PATH 256
void* c_safestring_lib;
void* c_jtag_client_lib;


AJI_ERROR c_jtag_client_gnuaji_init(void) {
    char safestring_path[MAX_PATH],
         jtag_client_path[MAX_PATH];

#if 1 //INTEL SECURITY POLICY APPLY
    { //dummy block
    char sym_exe_path[MAX_PATH], 
         abs_exe_path[MAX_PATH];
    snprintf(sym_exe_path, sizeof(sym_exe_path), "/proc/%d/exe", getpid());

    ssize_t s = readlink(sym_exe_path, abs_exe_path, sizeof(abs_exe_path));
    if(s == -1) {
   	    LOG_ERROR("Cannot find actual program path from symlink %s. errno is %d.", sym_exe_path, errno);
	    return AJI_FAILURE;
    }
    if( (s+1lu) > sizeof(abs_exe_path)) {
   	    LOG_ERROR("Buffer for storing absolute path is too small");
	    return AJI_FAILURE;
    }
    abs_exe_path[s]=0;
    if('/' != abs_exe_path[0]) {
  	    LOG_ERROR(
            "ERROR: Cannot resolve symbolic program path into an absolute path: %s => %s\n", 
            sym_exe_path, 
            abs_exe_path
        );
	    return AJI_FAILURE;
     }

    char *abs_exe_dir = dirname(abs_exe_path); 
    int  abs_exe_dir_length = strnlen(abs_exe_dir, sizeof(abs_exe_path));
    int maxlibsize =  sizeof(LIBRARY_NAME_SAFESTRING__LINUX) > sizeof(LIBRARY_NAME_JTAG_CLIENT__LINUX)?
	 	              sizeof(LIBRARY_NAME_SAFESTRING__LINUX) : sizeof(LIBRARY_NAME_JTAG_CLIENT__LINUX);

    if( (abs_exe_dir_length+1 + maxlibsize+1) > MAX_PATH) {
  	    LOG_ERROR("Buffer to short to hold full name of libraries. Needs %d, got %d\n", 
                  abs_exe_dir_length+1 + maxlibsize+1, MAX_PATH
        );
	    return AJI_FAILURE;
    }

    snprintf(safestring_path,  sizeof(safestring_path),  "%s/%s", abs_exe_dir, LIBRARY_NAME_SAFESTRING__LINUX);
    snprintf(jtag_client_path, sizeof(jtag_client_path), "%s/%s", abs_exe_dir, LIBRARY_NAME_JTAG_CLIENT__LINUX);
    } //dummy block
#else 
    { //dummyblock
    snprintf(safestring_path,  sizeof(safestring_path),  "%s", LIBRARY_NAME_SAFESTRING__LINUX);
    snprintf(jtag_client_path, sizeof(jtag_client_path), "%s", LIBRARY_NAME_JTAG_CLIENT__LINUX);
    } //dummy block
#endif //INTEL_SECURITY_POLICY

    LOG_DEBUG("Loading %s", safestring_path);
    c_safestring_lib = dlopen(safestring_path, RTLD_GLOBAL|RTLD_NOW);
    if (c_safestring_lib == NULL) {
        char *err = dlerror();
        LOG_ERROR(
            "Cannot load %s ...\n"
            "... Required path is %s ...\n"
            "... Error is %s",
            LIBRARY_NAME_SAFESTRING__LINUX, 
            safestring_path, 
            err
        );
        return AJI_FAILURE;
    }

    LOG_DEBUG("Loading %s", jtag_client_path);
    c_jtag_client_lib = dlopen(jtag_client_path, RTLD_GLOBAL|RTLD_NOW);
    if (c_jtag_client_lib == NULL) {
        char *err = dlerror();
        LOG_ERROR(
            "Cannot load %s ...\n"
            "... Required path is %s ...\n"
            "... Error is %s",
            LIBRARY_NAME_JTAG_CLIENT__LINUX,
            jtag_client_path,
            err
        );
        return AJI_FAILURE;
    }
    return AJI_NO_ERROR;
}

AJI_ERROR c_jtag_client_gnuaji_free(void) {
    if (c_jtag_client_lib != NULL) {
        dlclose(c_jtag_client_lib);
    }
    c_jtag_client_lib = NULL;

    if (c_safestring_lib != NULL) {
        dlclose(c_safestring_lib);
    }
    c_safestring_lib = NULL;
    return AJI_NO_ERROR;
}


#define FNAME_AJI_GET_HARDWARE__LINUX64 "_Z16aji_get_hardwarePjP12AJI_HARDWAREj"
AJI_ERROR c_aji_get_hardware(DWORD * hardware_count, AJI_HARDWARE * hardware_list, DWORD timeout) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(DWORD*, AJI_HARDWARE*, DWORD);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_GET_HARDWARE__LINUX64); //extra (void*) cast to prevent warning re casting from AJI_ERROR aji_get_hardware(*)
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(hardware_count, hardware_list, timeout);
}

#define FNAME_AJI_GET_HARDWARE2__LINUX64 "_Z17aji_get_hardware2PjP12AJI_HARDWAREPPcj"
AJI_ERROR c_aji_get_hardware2(DWORD * hardware_count, AJI_HARDWARE * hardware_list, char **server_version_info_list, DWORD timeout) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(DWORD*, AJI_HARDWARE*, char**, DWORD);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib,FNAME_AJI_GET_HARDWARE2__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(hardware_count, hardware_list, server_version_info_list, timeout);
} 

#define FNAME_AJI_FIND_HARDWARE__LINUX64 "_Z17aji_find_hardwarejP12AJI_HARDWAREj"
AJI_ERROR c_aji_find_hardware(DWORD persistent_id, AJI_HARDWARE * hardware, DWORD timeout) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(DWORD, AJI_HARDWARE*, DWORD);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_FIND_HARDWARE__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(persistent_id, hardware, timeout);
}

#define FNAME_AJI_FIND_HARDWARE_A__LINUX64 "_Z17aji_find_hardwarePKcP12AJI_HARDWAREj"
AJI_ERROR c_aji_find_hardware_a(const char * hw_name, AJI_HARDWARE * hardware, DWORD timeout) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(const char*, AJI_HARDWARE*, DWORD);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_FIND_HARDWARE_A__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(hw_name, hardware, timeout);
}

#define FNAME_AJI_READ_DEVICE_CHAIN__LINUX64 "_Z21aji_read_device_chainP9AJI_CHAINPjP10AJI_DEVICEb"
AJI_ERROR c_aji_read_device_chain(AJI_CHAIN_ID chain_id, DWORD * device_count, AJI_DEVICE * device_list, _Bool auto_scan) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID, DWORD*, AJI_DEVICE*, _Bool);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_READ_DEVICE_CHAIN__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, device_count, device_list, auto_scan);
}


#define FNAME_AJI_GET_NODES__LINUX64 "_Z13aji_get_nodesP9AJI_CHAINjPjS1_"
AJI_ERROR AJI_API c_aji_get_nodes(
    AJI_CHAIN_ID         chain_id,
    DWORD                tap_position,
    DWORD* idcodes,
    DWORD* idcode_n) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, DWORD, DWORD*, DWORD*);
    ProdFn pfn = (ProdFn)(void*)dlsym(c_jtag_client_lib, FNAME_AJI_GET_NODES__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, idcodes, idcode_n);
}

#define FNAME_AJI_GET_NODES_A__LINUX64 "_Z13aji_get_nodesP9AJI_CHAINjPjS1_S1_"
AJI_ERROR AJI_API c_aji_get_nodes_a(
    AJI_CHAIN_ID         chain_id,
    DWORD                tap_position,
    DWORD* idcodes,
    DWORD* idcode_n,
    DWORD* hub_info) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, DWORD, DWORD*, DWORD*, DWORD*);
    ProdFn pfn = (ProdFn)(void*)dlsym(c_jtag_client_lib, FNAME_AJI_GET_NODES_A__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, idcodes, idcode_n, hub_info);
}


#define FNAME_AJI_GET_NODES_B__LINUX64 "_Z13aji_get_nodesP9AJI_CHAINjP11AJI_HIER_IDPjP12AJI_HUB_INFO"
AJI_ERROR AJI_API c_aji_get_nodes_b(
    AJI_CHAIN_ID chain_id,
    DWORD  tap_position,
    AJI_HIER_ID* hier_ids,
    DWORD* hier_id_n,
    AJI_HUB_INFO* hub_infos) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, DWORD, AJI_HIER_ID*, DWORD*, AJI_HUB_INFO*);
    ProdFn pfn = (ProdFn)(void*)dlsym(c_jtag_client_lib, FNAME_AJI_GET_NODES_B__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, hier_ids, hier_id_n, hub_infos);
}


#define FNAME_AJI_LOCK__LINUX64 "_Z8aji_lockP8AJI_OPENj14AJI_PACK_STYLE"
AJI_ERROR c_aji_lock(AJI_OPEN_ID open_id, DWORD timeout, AJI_PACK_STYLE pack_style) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID, DWORD, AJI_PACK_STYLE);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_LOCK__LINUX64);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, timeout, pack_style);
}

#define FNAME_AJI_UNLOCK_LOCK_CHAIN__LINUX64 "_Z21aji_unlock_lock_chainP8AJI_OPENP9AJI_CHAIN"
AJI_ERROR c_aji_unlock_lock_chain(AJI_OPEN_ID unlock_id, AJI_CHAIN_ID lock_id) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID, AJI_CHAIN_ID);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_UNLOCK_LOCK_CHAIN__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(unlock_id, lock_id);
}

#define FNAME_AJI_UNLOCK__LINUX64 "_Z10aji_unlockP8AJI_OPEN"
AJI_ERROR c_aji_unlock(AJI_OPEN_ID open_id) {   
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_UNLOCK__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id);
}

#define FNAME_AJI_LOCK_CHAIN__LINUX64 "_Z14aji_lock_chainP9AJI_CHAINj"
AJI_ERROR c_aji_lock_chain(AJI_CHAIN_ID chain_id, DWORD timeout) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID, DWORD);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_LOCK_CHAIN__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, timeout);
}

#define FNAME_AJI_UNLOCK_CHAIN__LINUX64 "_Z16aji_unlock_chainP9AJI_CHAIN"
AJI_ERROR c_aji_unlock_chain(AJI_CHAIN_ID chain_id) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_UNLOCK_CHAIN__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id);
}

#define FNAME_AJI_UNLOCK_CHAIN_LOCK__LINUX64 "_Z21aji_unlock_chain_lockP9AJI_CHAINP8AJI_OPEN14AJI_PACK_STYLE"
AJI_ERROR c_aji_unlock_chain_lock(AJI_CHAIN_ID unlock_id, AJI_OPEN_ID lock_id, AJI_PACK_STYLE pack_style) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, AJI_OPEN_ID, AJI_PACK_STYLE);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_UNLOCK_CHAIN_LOCK__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(unlock_id, lock_id, pack_style);
}

#define FNAME_AJI_FLUSH__LINUX64 "_Z9aji_flushP8AJI_OPEN"
AJI_API AJI_ERROR c_aji_flush(AJI_OPEN_ID open_id) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID);
    ProdFn pfn = (ProdFn)(void*)dlsym(c_jtag_client_lib, FNAME_AJI_FLUSH__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id);
}

#define FNAME_AJI_OPEN_DEVICE__LINUX64 "_Z15aji_open_deviceP9AJI_CHAINjPP8AJI_OPENPK9AJI_CLAIMjPKc"
AJI_ERROR c_aji_open_device(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID * open_id, const AJI_CLAIM * claims, DWORD claim_n, const char * application_name){
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID, DWORD, AJI_OPEN_ID*, const AJI_CLAIM*, DWORD, const char*);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_OPEN_DEVICE__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, open_id, claims, claim_n, application_name);
}

#define FNAME_AJI_OPEN_DEVICE_A__LINUX64 "_Z15aji_open_deviceP9AJI_CHAINjPP8AJI_OPENPK10AJI_CLAIM2jPKc"
AJI_ERROR c_aji_open_device_a(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID * open_id, const AJI_CLAIM2 * claims, DWORD claim_n, const char * application_name) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID, DWORD, AJI_OPEN_ID*, const AJI_CLAIM2*, DWORD, const char*);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_OPEN_DEVICE__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, open_id, claims, claim_n, application_name);
} 

#define FNAME_AJI_CLOSE_DEVICE__LINUX64 "_Z16aji_close_deviceP8AJI_OPEN"
AJI_ERROR c_aji_close_device(AJI_OPEN_ID open_id) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_CLOSE_DEVICE__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id);
}

#define FNAME_AJI_OPEN_ENTIRE_DEVICE_CHAIN__LINUX64 "_Z28aji_open_entire_device_chainP9AJI_CHAINPP8AJI_OPEN14AJI_CHAIN_TYPEPKc"
AJI_ERROR c_aji_open_entire_device_chain(AJI_CHAIN_ID chain_id, AJI_OPEN_ID * open_id, AJI_CHAIN_TYPE style, const char * application_name) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID, AJI_OPEN_ID*, const AJI_CHAIN_TYPE, const char*);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_OPEN_ENTIRE_DEVICE_CHAIN__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, open_id, style, application_name);
}

#define FNAME_AJI_OPEN_NODE__LINUX64 "_Z13aji_open_nodeP9AJI_CHAINjjPP8AJI_OPENPK9AJI_CLAIMjPKc"
AJI_ERROR AJI_API c_aji_open_node             (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position,
                                               DWORD                idcode,
                                               AJI_OPEN_ID        * node_id,
                                               const AJI_CLAIM    * claims,
                                               DWORD                claim_n,
                                               const char         * application_name){
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, DWORD, DWORD, AJI_OPEN_ID*, const AJI_CLAIM*, DWORD, const char*);
    ProdFn pfn = (ProdFn)(void*)dlsym(c_jtag_client_lib, FNAME_AJI_OPEN_NODE__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, idcode, node_id, claims, claim_n, application_name);
}

#define FNAME_AJI_OPEN_NODE_A__LINUX64 "_Z13aji_open_nodeP9AJI_CHAINjjjPP8AJI_OPENPK9AJI_CLAIMjPKc"
AJI_ERROR AJI_API c_aji_open_node_a           (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position,
                                               DWORD                node_position,
                                               DWORD                idcode,
                                               AJI_OPEN_ID        * node_id,
                                               const AJI_CLAIM    * claims,
                                               DWORD                claim_n,
                                               const char         * application_name){
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, DWORD, DWORD, DWORD, AJI_OPEN_ID*, const AJI_CLAIM*, DWORD, const char*);
    ProdFn pfn = (ProdFn)(void*)dlsym(c_jtag_client_lib, FNAME_AJI_OPEN_NODE_A__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, node_position, idcode, node_id, claims, claim_n, application_name);
}

#define FNAME_AJI_OPEN_NODE_B__LINUX64 "_Z13aji_open_nodeP9AJI_CHAINjPK11AJI_HIER_IDPP8AJI_OPENPK10AJI_CLAIM2jPKc"
AJI_ERROR AJI_API c_aji_open_node_b           (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position,
                                               const AJI_HIER_ID  * hier_id,
                                               AJI_OPEN_ID        * node_id,
                                               const AJI_CLAIM2   * claims,
                                               DWORD                claim_n,
                                               const char         * application_name){
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, DWORD, const AJI_HIER_ID*, AJI_OPEN_ID*, const AJI_CLAIM2*, DWORD, const char*);
    ProdFn pfn = (ProdFn)(void*)dlsym(c_jtag_client_lib, FNAME_AJI_OPEN_NODE_B__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, hier_id, node_id, claims, claim_n, application_name);
}


#define FNAME_AJI_TEST_LOGIC_RESET__LINUX64 "_Z20aji_test_logic_resetP8AJI_OPEN"
AJI_ERROR c_aji_test_logic_reset(AJI_OPEN_ID open_id) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID*);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_TEST_LOGIC_RESET__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id);
}

#define FNAME_AJI_DELAY__LINUX64 "_Z9aji_delayP8AJI_OPENj"
AJI_ERROR c_aji_delay(AJI_OPEN_ID open_id, DWORD timeout_microseconds) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID, DWORD);
    ProdFn pfn = (ProdFn) (void*) dlsym(c_jtag_client_lib, FNAME_AJI_DELAY__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, timeout_microseconds);
}

#define FNAME_AJI_RUN_TEST_IDLE__LINUX64 "_Z17aji_run_test_idleP8AJI_OPENj"
AJI_ERROR c_aji_run_test_idle(AJI_OPEN_ID open_id, DWORD num_clocks) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD);
    ProdFn pfn = (ProdFn) (void*)dlsym(c_jtag_client_lib, FNAME_AJI_RUN_TEST_IDLE__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, num_clocks);
}

#define FNAME_AJI_RUN_TEST_IDLE_A__LINUX64 "_Z17aji_run_test_idleP8AJI_OPENjj"
AJI_ERROR c_aji_run_test_idle_a(AJI_OPEN_ID open_id, DWORD num_clocks, DWORD flags) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, DWORD);
    ProdFn pfn = (ProdFn) (void*)dlsym(c_jtag_client_lib, FNAME_AJI_RUN_TEST_IDLE_A__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, num_clocks, flags);
}

#define FNAME_AJI_ACCESS_IR__LINUX64 "_Z13aji_access_irP8AJI_OPENjPjj"
AJI_ERROR c_aji_access_ir(AJI_OPEN_ID open_id, DWORD instruction, DWORD * captured_ir, DWORD flags) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, DWORD*, DWORD);
    ProdFn pfn = (ProdFn) (void*)dlsym(c_jtag_client_lib, FNAME_AJI_ACCESS_IR__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, instruction, captured_ir, flags);
}

#define FNAME_AJI_ACCESS_IR_A__LINUX64 "_Z13aji_access_irP8AJI_OPENjPKhPhj"
AJI_ERROR c_aji_access_ir_a(AJI_OPEN_ID open_id, DWORD length_ir, const BYTE * write_bits, BYTE * read_bits, DWORD flags) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, const BYTE*, BYTE*, DWORD);
    ProdFn pfn = (ProdFn) (void*)dlsym(c_jtag_client_lib, FNAME_AJI_ACCESS_IR_A__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, length_ir, write_bits, read_bits, flags);
}

#define FNAME_AJI_ACCESS_DR__LINUX64 "_Z13aji_access_drP8AJI_OPENjjjjPKhjjPh"
AJI_ERROR c_aji_access_dr(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD read_offset, DWORD read_length, BYTE * read_bits) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, DWORD, DWORD, DWORD, const BYTE*, DWORD, DWORD, BYTE*);
    ProdFn pfn = (ProdFn) (void*)dlsym(c_jtag_client_lib, FNAME_AJI_ACCESS_DR__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, length_dr, flags, write_offset, write_length, write_bits, read_offset, read_length, read_bits);
}

#define FNAME_AJI_ACCESS_DR_A__LINUX64 "_Z13aji_access_drP8AJI_OPENjjjjPKhjjPhj"
AJI_ERROR c_aji_access_dr_a(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD read_offset, DWORD read_length, BYTE * read_bits, DWORD batch) {
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, DWORD, DWORD, DWORD, const BYTE*, DWORD, DWORD, BYTE*, DWORD);
    ProdFn pfn = (ProdFn) (void*)dlsym(c_jtag_client_lib, FNAME_AJI_ACCESS_DR__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(open_id, length_dr, flags, write_offset, write_length, write_bits, read_offset, read_length, read_bits, batch);
}


#define FNAME_AJI_ACCESS_OVERLAY__LINUX64 "_Z18aji_access_overlayP8AJI_OPENjPj"
AJI_API AJI_ERROR c_aji_access_overlay(AJI_OPEN_ID node_id, DWORD overlay, DWORD* captured_overlay){
    assert(c_safestring_lib != NULL);
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, DWORD*);
    ProdFn pfn = (ProdFn)(void*)dlsym(c_jtag_client_lib, FNAME_AJI_ACCESS_OVERLAY__LINUX64);
    assert(pfn != NULL);
    if (pfn == NULL) {
        return AJI_FAILURE;
    }
    return (pfn)(node_id, overlay, captured_overlay);
}
