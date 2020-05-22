#ifndef JTAGSERVICE_H_INCLUDED
#define JTAGSERVICE_H_INCLUDED

#include "c_aji.h"

#define JTAGSERVICE_TIMEOUT_MS 10000

enum jtagservice_lock {
    NONE    = 0b000000,
    CHAIN   = 0b000001,
    DEVICE  = 0b000010,
    
    ALL     = 0b111111,
};

struct jtagservice_record {
    //data members
    //() Cable
    DWORD chain_pid; //persistent_id for the chain
    
    //() Tap device
    DWORD device_id; 
    DWORD device_tap_position; //1-indexed+1, device.
    BYTE  device_irlen; 
    
    //state tracking
    enum jtagservice_lock locked;
};
 
static struct jtagservice_record jtagservice  = {
    .chain_pid = 0,
    .locked    = NONE,
};

_Bool jtagservice_is_locked(enum jtagservice_lock lock);
AJI_ERROR jtagservice_lock(enum jtagservice_lock, DWORD timeout);
AJI_ERROR jtagservice_unlock(enum jtagservice_lock);
AJI_ERROR jtagservice_free(void);

#endif //JTAGSERVICE_H_INCLUDED

