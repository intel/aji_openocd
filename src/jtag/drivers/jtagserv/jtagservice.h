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
    /* Not sure AJI_HARDWARE can survive function boundarie
     * so store its persistent ID as backup.
     * If proven that AJI_HARDWARE can work, then keep it.
     */
    DWORD          hardware_count; 
    AJI_HARDWARE  *hardware_list;
    char         **server_version_info_list;
    
    DWORD         in_use_hardware_index;
    AJI_HARDWARE *in_use_hardware;
    DWORD         in_use_chain_pid;

    //() Tap device
    DWORD      device_count;
    AJI_DEVICE *device_list;
    
    
    DWORD       in_use_device_tap_position; //1-indexed
    AJI_DEVICE *in_use_device;
    DWORD       in_use_device_id; 
    BYTE        in_use_device_irlen; 


    
    
    //state tracking
    enum jtagservice_lock locked;
};
 
static struct jtagservice_record jtagservice  = {
    .hardware_count = 0,
//    .hardware_list = NULL,
//    .server_version_info_list = NULL,
//    .in_use_hardware = NULL,
    .in_use_hardware_index = 0,
    .in_use_chain_pid = 0,
    
    .device_count = 0,
//    .device_list = NULL,
    .in_use_device_tap_position = 0,
//    .in_use_device = NULL,
    .in_use_device_id = 0,
    .in_use_device_irlen = 0,
    
    .locked    = NONE,
};

_Bool jtagservice_is_locked(enum jtagservice_lock lock);
AJI_ERROR jtagservice_lock(enum jtagservice_lock, DWORD timeout);
AJI_ERROR jtagservice_unlock(enum jtagservice_lock);
AJI_ERROR jtagservice_free(void);




int jtagservice_query_main(void);
#endif //JTAGSERVICE_H_INCLUDED

