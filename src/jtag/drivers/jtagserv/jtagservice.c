#include "jtagservice.h"

#include "c2_jtag_client_gnuaji.h"

void jtagservice_unlock(void) {
    if(jtagservice.locked_services & CHAIN_LOCKED) {
        AJI_HARDWARE hw;
        c2_aji_find_hardware(jtagservice.chain_pid, &hw, 1000);    
        c2_aji_unlock_chain(hw.chain_id);
        jtagservice.locked_services &= ~CHAIN_LOCKED;
    } 
} //end jtagserv_unlock


void jtagservice_free(void) {
    jtagservice_unlock();
}

