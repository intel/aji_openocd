#ifndef JTAGSERVICE_H_INCLUDED
#define JTAGSERVICE_H_INCLUDED

#include "c2_aji.h"

#define JTAGSERVICE_TIMEOUT_MS 10000

enum jtagservice_lock_status {
    NONE_LOCKED    = 0b000000,
    CHAIN_LOCKED   = 0b000001,
    DEVICE_LOCKED  = 0b000010
};

struct jtagservice_record {
    //data member
    DWORD chain_pid; //persistent_id for the chain
    
    //state tracking
    enum jtagservice_lock_status locked_services;
};
 
static struct jtagservice_record jtagservice  = {
    .chain_pid = 0,
    .locked_services    = NONE_LOCKED,
};


void jtagservice_unlock(void);
void jtagservice_free(void);

#endif //JTAGSERVICE_H_INCLUDED

