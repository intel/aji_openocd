#include "jtagservice.h"

#include "log.h"
#include "c_jtag_client_gnuaji.h"


/**
 * Print hardware name suitable for @see aji_find_hardware(hw_name, ...) use.
 *
 * @param hw_name On input, a string at least as large as hw_name_len,
 *                On output, the printed name string
 * @param hw_name_len On input, the size of hw_name
 *                    On output, the actual length of the name string
 * @param type  type string, e.g. "USB-BlasterII". Cannot  be NULL
 * @param server The server name (jtagd terminology), in the form 
 *               myserver.com:port. Can be NULL
 * @param explicit_localhost If server is NULL, set to true 
                              to use "localhost" in place of server
 * @param port   The port name (jtagd terminology), normally the PCI bus number
 *               Can be NULL
 *
 * @return AJI_NO_ERROR if everything ok
 *         AJI_TOO_MANY_DEVICE if the string is longer than what hw_name can take
 *                      In this case hw_name_len will give you the required length
 *                      hw_name will remain unaltered
 * 
 * @see AJI_ERROR AJI_CHAIN_JS::print_hardware_name()
 */
AJI_ERROR jtagservice_print_hardware_name(
    char  *hw_name, DWORD *hw_name_len,
    const char *type, const char *server, const bool explicit_localhost, 
    const char *port
) {
    DWORD need = strlen(type) + 1;
    if (server != NULL) {
        need += 4 + strlen(server);
    } else {
        need += 13; //i.e. " on localhost"
    }
    if (port != NULL) {
        need += 3 + strlen(port);
    }
    
    if ( *hw_name_len < need ) {
        *hw_name_len = need;
        return AJI_TOO_MANY_DEVICES; // Not enough space
    }
    
    *hw_name_len = need;
     
    char * ptr = hw_name;
    char * const end = ptr + *hw_name_len;
    ptr += snprintf(ptr, *hw_name_len, "%s", hw_name);

    if (server != NULL) {
        ptr += snprintf(ptr, (size_t)(end-ptr), " on %s", server);
    } else if (explicit_localhost) {
        ptr += snprintf(ptr, (size_t)(end-ptr), "%s", " on localhost");
    }
    if (port != NULL) {
        ptr += snprintf(ptr, (size_t)(end-ptr), " [%s]", port);
    }
    return AJI_NO_ERROR;
}

_Bool jtagservice_is_locked(jtagservice_record *me, enum jtagservice_lock lock) 
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
    return me->locked & lock;
}

AJI_ERROR jtagservice_lock(jtagservice_record *me, enum jtagservice_lock lock, DWORD timeout) 
{   LOG_INFO("***> IN %s(%d): %s %d\n", __FILE__, __LINE__, __FUNCTION__, me->locked);
  
    if(NONE == lock) {
        return AJI_NO_ERROR;
    }
    
    if(!me->in_use_hardware_chain_pid) {
        return AJI_FAILURE;
    }
    
    AJI_ERROR status = AJI_NO_ERROR;
    AJI_HARDWARE hw;
    status = c_aji_find_hardware(me->in_use_hardware_chain_pid, &hw, timeout);
    if( status != AJI_NO_ERROR) {
        return status;
    }
    if(!jtagservice_is_locked(me, CHAIN)) {
        status = c_aji_lock_chain(hw.chain_id, timeout);
    } //end if(jtagservice_is_locked(CHAIN))
    
    if( AJI_NO_ERROR == status ) {
            me->locked = CHAIN;
    }
    if( lock & CHAIN ) {
        return status;
    }
    
    return status;
} //end jtagserv_lock


AJI_ERROR jtagservice_unlock(jtagservice_record *me, enum jtagservice_lock lock, DWORD timeout) 
{   LOG_INFO("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
 
    AJI_ERROR status = AJI_NO_ERROR;
    if((CHAIN & lock) && jtagservice_is_locked(me, CHAIN)) {
        LOG_DEBUG("***> IN %s(%d): %s aji_unlock_chain via Persistent ID\n", __FILE__, __LINE__, __FUNCTION__);

        AJI_HARDWARE hw;
        status = c_aji_find_hardware(me->in_use_hardware_chain_pid, &hw, timeout);
        if(AJI_NO_ERROR == status){
            status = c_aji_unlock_chain(hw.chain_id);        
        }        
        me->locked &= ~CHAIN;
    }    //end if(jtagservice_is_locked(CHAIN)
    return status;
}

AJI_ERROR  jtagservice_free(jtagservice_record *me, DWORD timeout) 
{   LOG_DEBUG("***> IN %s(%d): %s\n", __FILE__, __LINE__, __FUNCTION__);
    
    jtagservice_unlock(me, ALL, timeout);

    //TODO: Free the variables    
    if(me->device_count != 0) {
        free(me->device_list);
        free(me->device_open_id_list);
        
        me->device_count = 0;
        me->device_list = NULL;
        me->device_open_id_list = NULL;
    }

    if(me->hardware_count != 0) {
        free(me->hardware_list);
        free(me->server_version_info_list);
        
        me->hardware_count = 0;
        me->hardware_list = NULL;
        me->server_version_info_list = NULL;
    }
    return AJI_NO_ERROR;
}






 
int jtagservice_query_main(void) {
    printf("Check inputs\n");
    char *quartus_jtag_client_config = getenv("QUARTUS_JTAG_CLIENT_CONFIG");
    if (quartus_jtag_client_config != NULL) {
        printf("Configuration file, set via QUARTUS_JTAG_CLIENT_CONFIG, is '%s'\n", 
               quartus_jtag_client_config
        );
    } else {
        printf("Environment variable QUARTUS_JTAG_CLIENT_CONFIG not setted\n");
    }
    
    printf("Initialize Variables\n");
    unsigned int timeout =10000; //0x7FFFFF;
    
    unsigned int hardware_capacity = 10;
    AJI_HARDWARE *hardware_list = (AJI_HARDWARE*) calloc(hardware_capacity, sizeof(AJI_HARDWARE));
    char **server_version_info_list = (char**) calloc(hardware_capacity, sizeof(char*));
    if(NULL == hardware_list) {
        printf("Failed to allocate memory for hardware_list\n");
        return AJI_NO_MEMORY;
    }

    printf("Query JTAG\n");
    unsigned int hardware_count = hardware_capacity;
    AJI_ERROR status = c_aji_get_hardware2( &hardware_count, hardware_list, server_version_info_list,timeout);
    printf("Return Status is %i\n", status);

    printf("Output Result\n");
    if (AJI_NO_ERROR == status) {
        printf("Number of hardware is %d\n", hardware_count);

        for(unsigned int i=0; i<hardware_count; ++i) {
            AJI_HARDWARE hw = hardware_list[i];
            printf("    (%u) device_name=%s hw_name=%s server=%s port=%s chain_id=%p persistent_id=%d, chain_type=%d, features=%d, server_version_info_list=%s\n", 
                   i+1, hw.device_name, hw.hw_name, hw.server, hw.port,  hw.chain_id, hw.persistent_id, hw.chain_type, hw.features,
                   server_version_info_list[i]);
           
           //Assume if persistent_id==0,hw is not defined. Felt that .port="Unable to connect" 
           //..is not a strong enough indicator that something is wrong       
           if(hw.persistent_id == 0) {
               printf("        Not a valid device.\n");
               continue;
           }
           AJI_CHAIN_ID chain_id = hw.chain_id;
           status = _Z14aji_lock_chainP9AJI_CHAINj(chain_id, timeout);
           if(AJI_NO_ERROR !=  status ) { 
               printf("       Problem with chain locking. Returned %d\n", status);
               continue;
           }

           DWORD device_count = hardware_capacity;
           AJI_DEVICE *device_list = (AJI_DEVICE*) calloc(device_count, sizeof(AJI_DEVICE));
           status = _Z21aji_read_device_chainP9AJI_CHAINPjP10AJI_DEVICEb(chain_id, &device_count, device_list, 1);
           if(AJI_NO_ERROR !=  status ) { 
               printf("       Problem with Getting device on this chain. Returned %d\n", status);
               free(device_list);
               _Z16aji_unlock_chainP9AJI_CHAIN(chain_id);
               continue;
           }
           printf("        Number of devices on chain is %d\n", device_count);
           for(DWORD tap_position=0; tap_position<device_count; ++tap_position) {
               AJI_DEVICE device = device_list[tap_position];
               printf("        (A%d) device_id=%08X, instruction_length=%d, features=%d, device_name=%s\n", 
                        tap_position+1, device.device_id, device.instruction_length, device.features, device.device_name);
                        
               
               DWORD hier_id_n = hardware_capacity; 
               AJI_HIER_ID *hier_ids = (AJI_HIER_ID*) calloc(hier_id_n, sizeof(AJI_HIER_ID));
               AJI_HUB_INFO *hub_infos = (AJI_HUB_INFO*) calloc(AJI_MAX_HIERARCHICAL_HUB_DEPTH, sizeof(AJI_HUB_INFO));

               status =  _Z13aji_get_nodesP9AJI_CHAINjP11AJI_HIER_IDPjP12AJI_HUB_INFO(chain_id, tap_position, hier_ids, &hier_id_n,  hub_infos);

               if(AJI_NO_ERROR !=  status ) { 
                   printf("       Problem with Getting nodes for this tap position. Returned %d\n", status);
                   free(hier_ids);
                   free(hub_infos);
                   continue;
               }
               
               printf("            Number of Hierarcies(hier_id_n)=%d,\n", hier_id_n); //hier_id_n = 0 and status=AJI_NO_ERROR if no SLD_HUB
               for(DWORD k=0; k<hier_id_n; ++k) { //With ARRIA10, this loop is entered for the FPGA Tap, if it has SLD nodes.
                    AJI_HIER_ID hid = hier_ids[k];
                    printf("            (B%d) idcode=%08X position_n=%d position=",
                             k+1,
                             hid.idcode,
                             hid.position_n // position_n = n means n+1 fields in hid.position[] defined
                    );
                    for(int m=0; m<hid.position_n+1; ++m) {
                        printf("%d", hid.positions[m]);
                    } //end for m
                    for(int m=hid.position_n+1; m<AJI_MAX_HIERARCHICAL_HUB_DEPTH; ++m) {
                        printf("x");
                    } //end for m (AJI_MAX_HIERARCHICAL_HUB_DEPTH);
                    printf(" hub_infos:");
                    for(int m=0; m<hid.position_n+1; ++m) {
                        printf("(H%d) bridge_idcode=%08X, hub_id_code=%08X", m, hub_infos[m].bridge_idcode, hub_infos[m].hub_idcode);               
                    } //end for m (bridge)
                    printf("\n");
               } //end for k (heir_id_n)
               
                if(hier_id_n == 0) { 
                    //With ARRIA10, this loop is entered for the ARM HPS TAP.
                    //HOWEVER, it will also be entered if the FPGA TAP has no SLD Hub.
                    int claim_size = 2;
                    AJI_CLAIM2 claims[] = { //setup for HPS
                          { AJI_CLAIM_IR_SHARED,          0, device.device_id == 0x4BA00477? 0b1111 : 0b1111111111 }, //NO NEED for BYPASS instruction actually
                          { AJI_CLAIM_IR_SHARED,          0, device.device_id == 0x4BA00477? 0b1110 : 0b1111111100 },
                    };

                    char appname[] = "MyApp";
                    printf("            (C1-1) Open Device %X ...\n", device.device_id); fflush(stdout);
                    AJI_OPEN_ID open_id = NULL;   
                    
                    status = _Z15aji_open_deviceP9AJI_CHAINjPP8AJI_OPENPK10AJI_CLAIM2jPKc(chain_id, tap_position, &open_id, claims, claim_size, appname);
                    if(AJI_NO_ERROR  != status) {
                       printf("            Cannot open device. Returned %d\n", status);
                       continue;
                    }
                    
                    printf("            (C1-2) Lock Device ...\n"); fflush(stdout);
                    status = _Z21aji_unlock_chain_lockP9AJI_CHAINP8AJI_OPEN14AJI_PACK_STYLE(chain_id, open_id, AJI_PACK_AUTO); //hardcoded timeout of 10000ms 
                    if(AJI_NO_ERROR  != status) {
                       printf("       Cannot lock device. Returned %d\n", status);
                       _Z16aji_close_deviceP8AJI_OPEN(open_id);
                       continue;
                    }
                    
                    /* Need not set state of TAP.
                    printf("            (C2-1) Set TAP state  TEST_LOGIC_RESET ...\n"); fflush(stdout);             
                    //status = _Z17aji_run_test_idleP8AJI_OPENj(open_id, 10); //Doing two clock pulse. One now, and one potentially deferred until the next instruction
                    //status = _Z20aji_test_logic_resetP8AJI_OPEN(open_id);  
                    if(AJI_NO_ERROR  != status) {
                       printf("       Cannot set TAP state to RUN_TEST_IDLE. Returned %d\n", status);
                       _Z10aji_unlockP8AJI_OPEN(open_id);
                       _Z16aji_close_deviceP8AJI_OPEN(open_id);
                       continue;
                    }
                    */
                    
                    printf("            (C2-2) Read IDCODE : Set IR ...\n"); fflush(stdout);  
                    { //begin dummy block
                    DWORD captured_ir = 0xFFFF;      
                    status = _Z13aji_access_irP8AJI_OPENjPjj(open_id, claims[1].value,  &captured_ir, 0);  
                                //if I am in TEST_LOGIC_RESET, I am suppose to get AJI_BAD_TAP STATE but I am not, and I get 0b1 on captured_ir
                    if(AJI_NO_ERROR  != status) {
                       printf("       Cannot send IDCODE instruction. Returned %d\n", status);
                       _Z10aji_unlockP8AJI_OPEN(open_id);
                       _Z16aji_close_deviceP8AJI_OPEN(open_id);
                       continue;
                    }
                    printf("                 Return IR buffer reads %04X\n", captured_ir);
                    } //end dummy block
                    
                    printf("            (C2-3) Read IDCODE : Read DR ...\n"); fflush(stdout);  
                    
                    DWORD length_dr = 32,
                          flags = 0,
                          write_offset = 0,
                          write_length = 0, //32,
                          read_offset = 0,
                          read_length = 32;

                    BYTE  write_bits[] = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
                    BYTE  read_bits[]  = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
                    
                    status = _Z13aji_access_drP8AJI_OPENjjjjPKhjjPh( open_id, length_dr, flags, write_offset, write_length, write_bits,  read_offset, read_length, read_bits  );
                    if(AJI_NO_ERROR  != status) {
                       printf("       Cannot receive IDCODE output. Returned %d\n", status);
                       _Z10aji_unlockP8AJI_OPEN(open_id);
                       _Z16aji_close_deviceP8AJI_OPEN(open_id);
                       continue;
                    }
                    printf("                 Return DR buffer reads %02X%02X%02X%02X %02X%02X%02X%02X\n", 
                            read_bits[3], read_bits[2], read_bits[1], read_bits[0], 
                            read_bits[7], read_bits[6], read_bits[5], read_bits[4]
                    );
           

                    printf("            (C1-3) Flush Device ...\n"); fflush(stdout);
                    _Z9aji_flushP8AJI_OPEN(open_id);
                    
                    printf("            (C1-4) Unlock Device ...\n"); fflush(stdout);
                    _Z10aji_unlockP8AJI_OPEN(open_id);
                    
                    printf("            (C1-5) Close Device ...\n"); fflush(stdout);
                    _Z16aji_close_deviceP8AJI_OPEN(open_id);
                    printf("(C1-5) Done.\n");
               } //end if (hier_id_n == 0)

               free(hier_ids);
               free(hub_infos);
           } // end for tap_position (device_count)
                 
 
           free(device_list);
           _Z16aji_unlock_chainP9AJI_CHAIN(chain_id);
       } //end for i (hardware List)
    } //end if status


    printf("Free\n");
//    free(hardware_list);
 
    printf("Completed\n");
	return 0;
}


