#ifndef JTAGSERVICE_H_INCLUDED
#define JTAGSERVICE_H_INCLUDED

#if IS_WIN32
#include <windows.h>
#endif

#include "h/c_aji.h"


#define JTAGSERVICE_TIMEOUT_MS 10000

enum jtagservice_lock {
    NONE    = 0b000000,
    CHAIN   = 0b000001,
    DEVICE  = 0b000010,
    
    ALL     = 0b111111,
};

typedef struct jtagservice_record jtagservice_record;
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
    DWORD         in_use_hardware_chain_pid;

    //() Tap device
    DWORD        device_count;
    AJI_DEVICE  *device_list;
    AJI_OPEN_ID *device_open_id_list;
    DEVICE_TYPE *device_type_list;
    
    DWORD       in_use_device_tap_position;
    AJI_DEVICE *in_use_device;
    DWORD       in_use_device_id; 
    BYTE        in_use_device_irlen;

    //state tracking
    enum jtagservice_lock locked;

    AJI_CLAIM** claims; ///! List of AJI_CLAIM, by DEVICE_TYPE
};
 

_Bool jtagservice_is_locked(jtagservice_record *me, enum jtagservice_lock lock);
AJI_ERROR jtagservice_lock(jtagservice_record *me, enum jtagservice_lock, DWORD timeout);
AJI_ERROR jtagservice_unlock(jtagservice_record *me, enum jtagservice_lock, DWORD timeout);

AJI_ERROR jtagservice_init(jtagservice_record* me, DWORD timeout);
AJI_ERROR jtagservice_free(jtagservice_record *me, DWORD timeout);




int jtagservice_query_main(void);
#endif //JTAGSERVICE_H_INCLUDED

