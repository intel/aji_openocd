#include "jtagservice.h"

#include "log.h"
#include "c_jtag_client_gnuaji.h"

_Bool jtagservice_is_locked(enum jtagservice_lock lock) {
    return jtagservice.locked & lock;
}

AJI_ERROR jtagservice_lock(enum jtagservice_lock lock, DWORD timeout) { 
    AJI_ERROR status = AJI_NO_ERROR;
    if(jtagservice_is_locked(CHAIN)) {
        AJI_HARDWARE hardware;
        status = c_aji_find_hardware(jtagservice.chain_pid, &hardware, 
                JTAGSERVICE_TIMEOUT_MS
        );
        if(AJI_NO_ERROR != status) {
            LOG_ERROR("Failed to find the chosen cable."
                  " Return Status is %i\n", status
            );
            return status;
        }

        status = c_aji_lock_chain(hardware.chain_id, timeout);
        if( AJI_NO_ERROR == status ) {
            jtagservice.locked &= ~CHAIN;
        }
        return status;
    } 
    
    return status;
} //end jtagserv_unlock


AJI_ERROR jtagservice_unlock(enum jtagservice_lock lock) { 
    if(jtagservice_is_locked(CHAIN)) {
        AJI_HARDWARE hardware;
        AJI_ERROR status = c_aji_find_hardware(jtagservice.chain_pid, &hardware, 
                JTAGSERVICE_TIMEOUT_MS
        );
        if(AJI_NO_ERROR != status) {
            LOG_ERROR("Failed to find the chosen cable."
                  " Return Status is %i\n", status
            );
            return status;
        }
        c_aji_unlock_chain(hardware.chain_id);
        jtagservice.locked &= ~CHAIN;
    }    
    return AJI_NO_ERROR;
}

AJI_ERROR  jtagservice_free(void) {
    AJI_ERROR status = AJI_NO_ERROR;
    status = jtagservice_unlock(ALL);
    return status;
}

