#include "c_aji.h"
#include "c_jtag_client_gnuaji_win64.h"

/**
 * C AJI functions
 * It wraps mangled C++ name in jtag_client_gnu_aji.h with non-manged
 * C function name . Will use the same function name as the original
 * C++ name but with a "c" prefix
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
 * $
 */
#include <windows.h>
#include "c_aji.h"

#define LIBRARY_NAME_JTAG_CLIENT "jtag_client.dll"


HINSTANCE c_jtag_client_lib;


AJI_ERROR c_jtag_client_gnuaji_init(void) {
    c_jtag_client_lib = LoadLibrary(TEXT(LIBRARY_NAME_JTAG_CLIENT));
    if (c_jtag_client_lib == NULL) {
//        LOG_ERROR("Cannot find %s.", LIBRARY_NAME_JTAG_CLIENT);
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


#define FNAME_AJI_GET_HARDWARE "?aji_get_hardware@@YA?AW4AJI_ERROR@@PEAKPEAUAJI_HARDWARE@@K@Z"
AJI_ERROR c_aji_get_hardware(DWORD * hardware_count, AJI_HARDWARE * hardware_list, DWORD timeout) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(DWORD*, AJI_HARDWARE*, DWORD);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_GET_HARDWARE); //extra (void*) cast to prevent warning re casting from AJI_ERROR aji_get_hardware(*)
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(hardware_count, hardware_list, timeout);
}

#define FNAME_AJI_GET_HARDWARE2 "?aji_get_hardware2@@YA?AW4AJI_ERROR@@PEAKPEAUAJI_HARDWARE@@PEAPEADK@Z"
AJI_ERROR c_aji_get_hardware2(DWORD * hardware_count, AJI_HARDWARE * hardware_list, char **server_version_info_list, DWORD timeout) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(DWORD*, AJI_HARDWARE*, char**, DWORD);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib,FNAME_AJI_GET_HARDWARE2);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(hardware_count, hardware_list, server_version_info_list, timeout);
} 

#define FNAME_AJI_FIND_HARDWARE "?aji_find_hardware@@YA?AW4AJI_ERROR@@KPEAUAJI_HARDWARE@@K@Z"
AJI_ERROR c_aji_find_hardware(DWORD persistent_id, AJI_HARDWARE * hardware, DWORD timeout) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(DWORD, AJI_HARDWARE*, DWORD);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_FIND_HARDWARE);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(persistent_id, hardware, timeout);
}

#define FNAME_AJI_FIND_HARDWARE_A "?aji_find_hardware@@YA?AW4AJI_ERROR@@PEBDPEAUAJI_HARDWARE@@K@Z"
AJI_ERROR c_aji_find_hardware_a(const char * hw_name, AJI_HARDWARE * hardware, DWORD timeout) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(const char*, AJI_HARDWARE*, DWORD);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_FIND_HARDWARE_A);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(hw_name, hardware, timeout);
}

#define FNAME_AJI_READ_DEVICE_CHAIN "?aji_read_device_chain@@YA?AW4AJI_ERROR@@PEAVAJI_CHAIN@@PEAKPEAUAJI_DEVICE@@_N@Z"
AJI_ERROR c_aji_read_device_chain(AJI_CHAIN_ID chain_id, DWORD * device_count, AJI_DEVICE * device_list, _Bool auto_scan) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID, DWORD*, AJI_DEVICE*, _Bool);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_READ_DEVICE_CHAIN);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, device_count, device_list, auto_scan);
}


#define FNAME_AJI_GET_NODES "?aji_get_nodes@@YA?AW4AJI_ERROR@@PEAVAJI_CHAIN@@KPEAK1@Z"
AJI_ERROR AJI_API c_aji_get_nodes(
    AJI_CHAIN_ID         chain_id,
    DWORD                tap_position,
    DWORD* idcodes,
    DWORD* idcode_n) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, DWORD, DWORD*, DWORD*);
    ProdFn pfn = (ProdFn)(void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_GET_NODES);
    if (pfn == NULL) {
        //        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, idcodes, idcode_n);
}

#define FNAME_AJI_GET_NODES_A "?aji_get_nodes@@YA?AW4AJI_ERROR@@PEAVAJI_CHAIN@@KPEAK11@Z"
AJI_ERROR AJI_API c_aji_get_nodes_a(
    AJI_CHAIN_ID         chain_id,
    DWORD                tap_position,
    DWORD* idcodes,
    DWORD* idcode_n,
    DWORD* hub_info) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, DWORD, DWORD*, DWORD*, DWORD*);
    ProdFn pfn = (ProdFn)(void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_GET_NODES_A);
    if (pfn == NULL) {
        //        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, idcodes, idcode_n, hub_info);
}

#define FNAME_AJI_GET_NODES_B "?aji_get_nodes@@YA?AW4AJI_ERROR@@PEAVAJI_CHAIN@@KPEAUAJI_HIER_ID@@PEAKPEAUAJI_HUB_INFO@@@Z"
AJI_ERROR AJI_API c_aji_get_nodes_b(
    AJI_CHAIN_ID chain_id,
    DWORD  tap_position,
    AJI_HIER_ID* hier_ids,
    DWORD* hier_id_n,
    AJI_HUB_INFO* hub_infos) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, DWORD, AJI_HIER_ID*, DWORD*, AJI_HUB_INFO*);
    ProdFn pfn = (ProdFn)(void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_GET_NODES_B);
    if (pfn == NULL) {
        //        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, hier_ids, hier_id_n, hub_infos);
}

#define FNAME_AJI_LOCK "?aji_lock@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@KW4AJI_PACK_STYLE@@@Z"
AJI_ERROR c_aji_lock(AJI_OPEN_ID open_id, DWORD timeout, AJI_PACK_STYLE pack_style) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID, DWORD, AJI_PACK_STYLE);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_LOCK);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(open_id, timeout, pack_style);
}

#define FNAME_AJI_UNLOCK_LOCK_CHAIN "?aji_unlock_lock_chain@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@PEAVAJI_CHAIN@@@Z"
AJI_ERROR c_aji_unlock_lock_chain(AJI_OPEN_ID unlock_id, AJI_CHAIN_ID lock_id) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID, AJI_CHAIN_ID);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_UNLOCK_LOCK_CHAIN);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(unlock_id, lock_id);
}

#define FNAME_AJI_UNLOCK "?aji_unlock@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@@Z"
AJI_ERROR c_aji_unlock(AJI_OPEN_ID open_id) {   
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_UNLOCK);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(open_id);
}

#define FNAME_AJI_LOCK_CHAIN "?aji_lock_chain@@YA?AW4AJI_ERROR@@PEAVAJI_CHAIN@@K@Z"
AJI_ERROR c_aji_lock_chain(AJI_CHAIN_ID chain_id, DWORD timeout) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID, DWORD);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_LOCK_CHAIN);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, timeout);
}

#define FNAME_AJI_UNLOCK_CHAIN "?aji_unlock_chain@@YA?AW4AJI_ERROR@@PEAVAJI_CHAIN@@@Z"
AJI_ERROR c_aji_unlock_chain(AJI_CHAIN_ID chain_id) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_UNLOCK_CHAIN);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(chain_id);
}

#define FNAME_AJI_UNLOCK_CHAIN_LOCK "?aji_unlock_chain_lock@@YA?AW4AJI_ERROR@@PEAVAJI_CHAIN@@PEAVAJI_OPEN@@W4AJI_PACK_STYLE@@@Z"
AJI_ERROR c_aji_unlock_chain_lock(AJI_CHAIN_ID unlock_id, AJI_OPEN_ID lock_id, AJI_PACK_STYLE pack_style) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_CHAIN_ID, AJI_OPEN_ID, AJI_PACK_STYLE);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_UNLOCK_CHAIN_LOCK);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(unlock_id, lock_id, pack_style);
}

#define FNAME_AJI_OPEN_DEVICE "?aji_open_device@@YA?AW4AJI_ERROR@@PEAVAJI_CHAIN@@KPEAPEAVAJI_OPEN@@PEBUAJI_CLAIM@@KPEBD@Z"
AJI_ERROR c_aji_open_device(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID * open_id, const AJI_CLAIM * claims, DWORD claim_n, const char * application_name){
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID, DWORD, AJI_OPEN_ID*, const AJI_CLAIM*, DWORD, const char*);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_OPEN_DEVICE);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, open_id, claims, claim_n, application_name);
}

#define FNAME_AJI_OPEN_DEVICE_A "?aji_open_device@@YA?AW4AJI_ERROR@@PEAVAJI_CHAIN@@KPEAPEAVAJI_OPEN@@PEBUAJI_CLAIM2@@KPEBD@Z"
AJI_ERROR c_aji_open_device_a(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID * open_id, const AJI_CLAIM2 * claims, DWORD claim_n, const char * application_name) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID, DWORD, AJI_OPEN_ID*, const AJI_CLAIM2*, DWORD, const char*);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_OPEN_DEVICE);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, tap_position, open_id, claims, claim_n, application_name);
} 

#define FNAME_AJI_CLOSE_DEVICE "?aji_close_device@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@@Z"
AJI_ERROR c_aji_close_device(AJI_OPEN_ID open_id) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_CLOSE_DEVICE);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(open_id);
}

#define FNAME_AJI_OPEN_ENTIRE_DEVICE_CHAIN "?aji_open_entire_device_chain@@YA?AW4AJI_ERROR@@PEAVAJI_CHAIN@@PEAPEAVAJI_OPEN@@W4AJI_CHAIN_TYPE@@PEBD@Z"
AJI_ERROR c_aji_open_entire_device_chain(AJI_CHAIN_ID chain_id, AJI_OPEN_ID * open_id, AJI_CHAIN_TYPE style, const char * application_name) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_CHAIN_ID, AJI_OPEN_ID*, const AJI_CHAIN_TYPE, const char*);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_OPEN_ENTIRE_DEVICE_CHAIN);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(chain_id, open_id, style, application_name);
}

#define FNAME_AJI_TEST_LOGIC_RESET "?aji_test_logic_reset@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@@Z"
AJI_ERROR c_aji_test_logic_reset(AJI_OPEN_ID open_id) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID*);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_TEST_LOGIC_RESET);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(open_id);
}

#define FNAME_AJI_DELAY "?aji_delay@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@K@Z"
AJI_ERROR c_aji_delay(AJI_OPEN_ID open_id, DWORD timeout_microseconds) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR (*ProdFn)(AJI_OPEN_ID, DWORD);
    ProdFn pfn = (ProdFn) (void*) GetProcAddress(c_jtag_client_lib, FNAME_AJI_DELAY);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(open_id, timeout_microseconds);
}

#define FNAME_AJI_RUN_TEST_IDLE "?aji_run_test_idle@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@K@Z"
AJI_ERROR c_aji_run_test_idle(AJI_OPEN_ID open_id, DWORD num_clocks) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD);
    ProdFn pfn = (ProdFn) (void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_RUN_TEST_IDLE);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(open_id, num_clocks);
}

#define FNAME_AJI_RUN_TEST_IDLE_A "?aji_run_test_idle@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@KK@Z"
AJI_ERROR c_aji_run_test_idle_a(AJI_OPEN_ID open_id, DWORD num_clocks, DWORD flags) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, DWORD);
    ProdFn pfn = (ProdFn) (void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_RUN_TEST_IDLE_A);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(open_id, num_clocks, flags);
}

#define FNAME_AJI_ACCESS_IR "?aji_access_ir@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@KPEAKK@Z"
AJI_ERROR c_aji_access_ir(AJI_OPEN_ID open_id, DWORD instruction, DWORD * captured_ir, DWORD flags) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, DWORD*, DWORD);
    ProdFn pfn = (ProdFn) (void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_ACCESS_IR);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(open_id, instruction, captured_ir, flags);
}

#define FNAME_AJI_ACCESS_IR_A "?aji_access_ir@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@KPEBEPEAEK@Z"
AJI_ERROR c_aji_access_ir_a(AJI_OPEN_ID open_id, DWORD length_ir, const BYTE * write_bits, BYTE * read_bits, DWORD flags) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, const BYTE*, BYTE*, DWORD);
    ProdFn pfn = (ProdFn) (void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_ACCESS_IR_A);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(open_id, length_ir, write_bits, read_bits, flags);
}

#define FNAME_AJI_ACCESS_IR_MULTIPLE "?aji_access_ir_multiple@@YA?AW4AJI_ERROR@@KPEBQEAVAJI_OPEN@@PEBKPEAK@Z"
AJI_ERROR c_aji_access_ir_multiple(DWORD num_devices, const AJI_OPEN_ID * open_id, const DWORD * instructions, DWORD * captured_irs) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(DWORD, const AJI_OPEN_ID*, const DWORD*, DWORD*);
    ProdFn pfn = (ProdFn) (void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_ACCESS_IR_MULTIPLE);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(num_devices, open_id, instructions, captured_irs);
}

#define FNAME_AJI_ACCESS_DR "?aji_access_dr@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@KKKKPEBEKKPEAE@Z"
AJI_ERROR c_aji_access_dr(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD read_offset, DWORD read_length, BYTE * read_bits) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, DWORD, DWORD, DWORD, const BYTE*, DWORD, DWORD, BYTE*);
    ProdFn pfn = (ProdFn) (void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_ACCESS_DR);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(open_id, length_dr, flags, write_offset, write_length, write_bits, read_offset, read_length, read_bits);
}

#define FNAME_AJI_ACCESS_DR_A "?aji_access_dr@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@KKKKPEBEKKPEAEK@Z"
AJI_ERROR c_aji_access_dr_a(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD read_offset, DWORD read_length, BYTE * read_bits, DWORD batch) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID, DWORD, DWORD, DWORD, DWORD, const BYTE*, DWORD, DWORD, BYTE*, DWORD);
    ProdFn pfn = (ProdFn) (void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_ACCESS_DR);
    if (pfn == NULL) {
//        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(open_id, length_dr, flags, write_offset, write_length, write_bits, read_offset, read_length, read_bits, batch);
}


#define FNAME_AJI_FLUSH "?aji_flush@@YA?AW4AJI_ERROR@@PEAVAJI_OPEN@@@Z"
AJI_API AJI_ERROR c_aji_flush(AJI_OPEN_ID open_id) {
    assert(c_jtag_client_lib != NULL);
    typedef AJI_ERROR(*ProdFn)(AJI_OPEN_ID);
    ProdFn pfn = (ProdFn)(void*)GetProcAddress(c_jtag_client_lib, FNAME_AJI_FLUSH);
    if (pfn == NULL) {
        //        LOG_ERROR("Cannot find function '%s'", __FUNCTION__);
        return AJI_FAILURE;
    }
    return (pfn)(open_id);
}
