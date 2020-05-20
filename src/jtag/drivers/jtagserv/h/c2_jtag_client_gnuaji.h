/**
 * C2 AJI functions
 *
 * This wraps C AJI functions in a wrapper that provide reasonable/expected responds to the the reply
 * from JTAG server. For example, attempt a retry if there were a temporary failure in communication
 *
 * It also handles resource allocation. However, being C, user has to deallocate the memory/free the
 * resource acquired. In general, if the AJI API description says the user has to allocate the appropriate
 * memory, then the wrapper will allocate the said memory but the user has to free the memory. This is limitted
 * to dynamically sized array. For example, in c2_aji_get_hardware()
 *
 * All functions maintain their C_AJI name but with 2 inserted between "c" and "_" prefix. Any recommended
 * parameters will be set in global struct c2_aji_param.
 */
 
#include "c_jtag_client_gnuaji.h"

/* A structure holding the parameters for C2 AJI function */
struct c2_aji_configuration {
   DWORD timeout; //miliseconds
};

static
struct c2_aji_configuration c2_aji_config = {
  .timeout=10000
};


inline 
AJI_API AJI_ERROR c2_aji_get_hardware(DWORD * hardware_count, AJI_HARDWARE * hardware_list, DWORD timeout) {
    return c_aji_get_hardware(hardware_count, hardware_list, timeout);
}

inline 
AJI_API AJI_ERROR c2_aji_get_hardware2(DWORD * hardware_count, AJI_HARDWARE * hardware_list, char **server_version_info_list, DWORD timeout) {
    return c_aji_get_hardware2(hardware_count, hardware_list, server_version_info_list, timeout);
} 

