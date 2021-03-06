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

#include "jtagservice.h"

#include <time.h>
#include "log.h"
#include "aji/c_jtag_client_gnuaji.h"


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

/**
 * Create the claim records. One for each device_type
 * @param records On input, an array of size records_n. 
 *                On output, array will be filled with claims information. 
 *                index = @see JTAGSERVE_DEVICE_TYPE
 * @param records_n On input, size of records. On output, if return AJI_TOO_MANY_CLAIMS,
 *                  will contain the required size to host all claims
 * @return AJI_NO_ERROR if there is no error. 
 *         AJI_TOO_MANY_CLAIMS if the records array is too small
 *         AJI_NO_MEMORY if run out of memory
 * @note Memory allocated here freed as part of @jtagservice_free()
 */
AJI_ERROR jtagservice_create_claim_records(CLAIM_RECORD *records, DWORD * records_n) {
    if (UNKNOWN < *records_n) {
        DWORD csize = 0;
//#if PORT == WINDOWS
        AJI_CLAIM* claims = (AJI_CLAIM*) calloc(csize, sizeof(AJI_CLAIM)); //It's empty, NULL perhaps?
//#else
//        AJI_CLAIM2 *claims = (AJI_CLAIM2*) calloc(csize, sizeof(AJI_CLAIM2)); //It's empty, NULL perhaps?
//#endif
        if (claims == NULL) {
            return AJI_NO_MEMORY;
        }
    }
    if (ARM < *records_n) {
        DWORD csize = 4;
//#if PORT == WINDOWS
        AJI_CLAIM* claims = (AJI_CLAIM*) calloc(csize, sizeof(AJI_CLAIM));
//#else
//        AJI_CLAIM2* claims = (AJI_CLAIM2*) calloc(csize, sizeof(AJI_CLAIM2));
//#endif
        if (claims == NULL) {
            return AJI_NO_MEMORY;
        }

        claims[0].type = AJI_CLAIM_IR_SHARED;
        claims[0].value = IR_ARM_IDCODE;
        claims[1].type = AJI_CLAIM_IR_SHARED;
        claims[1].value = IR_ARM_DPACC;
        claims[2].type = AJI_CLAIM_IR_SHARED;
        claims[2].value = IR_ARM_APACC;
        claims[3].type = AJI_CLAIM_IR_SHARED;
        claims[3].value = IR_ARM_ABORT;

        records[ARM].claims_n = csize;
        records[ARM].claims = claims;
    }

    AJI_ERROR status = AJI_NO_ERROR;
    if (*records_n < DEVICE_TYPE_COUNT) {
        status = AJI_TOO_MANY_CLAIMS;
        *records_n = DEVICE_TYPE_COUNT;
    }
    return status;
}

AJI_ERROR jtagservice_device_index_by_idcode(
    const DWORD idcode,
    const AJI_DEVICE *tap_list, const DWORD tap_count, 
    DWORD* tap_index) {

    DWORD index = 0;
    for (; index < tap_count; ++index) {
        if (tap_list[index].device_id == idcode) {
            break;
        }
    }
    
    if (index == tap_count) {
        return AJI_FAILURE;
    }
    *tap_index = index;
    return AJI_NO_ERROR;
}

AJI_ERROR jtagservice_hier_id_index_by_idcode(
    const DWORD idcode,
    const AJI_HIER_ID *hier_id_list, const DWORD hier_id_count,
    DWORD* hier_index) {
    DWORD index = 0;
    for (; index < hier_id_count; ++index) {
        LOG_DEBUG("SLD Node ID matching - Attempting to match %lX. Try %lu with %lX",
            (unsigned long) idcode, (unsigned long)  index, (unsigned long) hier_id_list[index].idcode);
        if (hier_id_list[index].idcode == idcode) {
            break;
        }
    }

    if (index == hier_id_count) {
        return AJI_FAILURE;
    }
    *hier_index = index;
    return AJI_NO_ERROR;
}

AJI_ERROR jtagservice_update_active_tap_record(jtagservice_record* me, const DWORD tap_index, const bool is_sld, const DWORD node_index) {
    AJI_DEVICE device = me->device_list[tap_index];
    me->in_use_device = &(me->device_list[tap_index]); //DO NOT use &device as device is local variable
    me->in_use_device_id = device.device_id;
    me->in_use_device_tap_position = tap_index;
    me->in_use_device_irlen = device.instruction_length;

    return AJI_NO_ERROR;
}

/**
 * Activate Tap
 * \param hardware_index Not yet in use, set to zero.
 */
AJI_ERROR jtagservice_activate_jtag_tap (
    jtagservice_record* me, 
    const DWORD hardware_index,
    const DWORD tap_index
) {
    AJI_ERROR status = AJI_NO_ERROR;
    if (!me->device_type_list[tap_index]) {
        LOG_ERROR("Unknown device type tap #%lu idcode=0x%lu",
            (unsigned long) tap_index, 
            (unsigned long) me->device_list[tap_index].device_id
        );
        //@TODO: Do a type lookup instead.
        return AJI_FAILURE;
    }

    if (!me->device_open_id_list[tap_index]) {
        LOG_ERROR("Unknown OPEN ID for tap #%lu idcode=0x%08lX", 
            (unsigned long) tap_index, 
            (unsigned long) me->device_list[tap_index].device_id
        );

        status = c_aji_lock_chain(me->in_use_hardware_chain_id, 
                                  JTAGSERVICE_TIMEOUT_MS
        );
        if(AJI_NO_ERROR !=  status ) { 
             LOG_ERROR("Cannot lock chain. Returned %d (%s)\n", 
                  status, c_aji_error_decode(status)
             );
             return status;
        }
//#if PORT == WINDOWS
        status = c_aji_open_device(
            me->in_use_hardware_chain_id,
            tap_index,
            &(me->device_open_id_list[tap_index]),
            (const AJI_CLAIM*)(me->claims[me->device_type_list[tap_index]].claims),
            me->claims[me->device_type_list[tap_index]].claims_n,
            me->appIdentifier
        );
//#else
//        status = c_aji_open_device_a(
//            me->in_use_hardware_chain_id,
//            tap_index,
//            &(me->device_open_id_list[tap_index]),
//            (const AJI_CLAIM2*) (me->claims[me->device_type_list[tap_index]].claims), 
//            me->claims[me->device_type_list[tap_index]].claims_n, 
//            me->appIdentifier
//        );
//#endif
        if(AJI_NO_ERROR !=  status ) { 
             LOG_ERROR("Problem openning tap %lu (0x%08lX). Returned %d (%s)\n", 
                  (unsigned long) tap_index, 
                  (unsigned long) me->device_list[tap_index].device_id, 
                  status, 
                  c_aji_error_decode(status)
             );
             return status;
        }
        status = c_aji_unlock_chain(me->in_use_hardware_chain_id);
        if(AJI_NO_ERROR !=  status ) { 
             LOG_ERROR("Cannot unlock chain. Returned %d (%s)\n", 
                  status, c_aji_error_decode(status)
             );
             return status;
        }
    } //end if (!me->device_open_id_list[tap_index]) 
    
    status = jtagservice_update_active_tap_record(me, (unsigned long) tap_index, false, 0);
    return status;
}

AJI_ERROR jtagservice_init(jtagservice_record* me, const DWORD timeout) {
    AJI_ERROR status = AJI_NO_ERROR;

    status = jtagservice_init_common(me, timeout);
    if (status) {
        return status;
    }

    status = jtagservice_init_cable(me, timeout);
    if (status) {
        return status;
    }

    status = jtagservice_init_tap(me, timeout);
    if (status) {
        return status;
    }

    return AJI_NO_ERROR;
}

AJI_ERROR jtagservice_init_tap(jtagservice_record* me, DWORD timeout) {
    me->device_count = 0;
    me->device_list = NULL;
    me->device_open_id_list = NULL;
    me->device_type_list = NULL;

    me->in_use_device_tap_position = -1;
    me->in_use_device = NULL;
    me->in_use_device_id = 0;
    me->in_use_device_irlen = 0;

    return AJI_NO_ERROR;
}

AJI_ERROR jtagservice_init_cable(jtagservice_record* me, DWORD timeout) {
    me->hardware_count = 0;
    me->hardware_list = NULL;
    me->server_version_info_list = NULL;

    me->in_use_hardware = NULL;
    me->in_use_hardware_chain_pid = 0;
    me->in_use_hardware_chain_id = NULL;

    return AJI_NO_ERROR;
}

AJI_ERROR jtagservice_init_common(jtagservice_record* me, DWORD timeout) {
    me->appIdentifier = (char*) calloc(28, sizeof(char));
    if (NULL == me->appIdentifier) {
        return AJI_NO_MEMORY;
    }

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(me->appIdentifier, "OpenOCD.%4d%02d%02d%02d%02d%02d",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec
    );
    LOG_INFO("Application name is %s", me->appIdentifier);

    me->claims_count = DEVICE_TYPE_COUNT;
    me->claims = (CLAIM_RECORD*) calloc(me->claims_count, sizeof(CLAIM_RECORD));
    if (NULL == me->claims) {
        return AJI_NO_MEMORY;
    }
    AJI_ERROR status = jtagservice_create_claim_records(
        me->claims, &me->claims_count
    );
    return status;
}
AJI_ERROR  jtagservice_free(jtagservice_record *me, DWORD timeout) 
{
    AJI_ERROR retval = AJI_NO_ERROR;
    AJI_ERROR status = AJI_NO_ERROR;
    status = jtagservice_free_tap(me, timeout);
    if (status) {
        retval = status;
    }

    status = jtagservice_free_cable(me, timeout);
    if (status) {
        retval = status;
    }
    status = jtagservice_free_common(me, timeout);
    if (status) {
        retval = status;
    }

    return status;
}


AJI_ERROR  jtagservice_free_tap(jtagservice_record* me, const DWORD timeout)
{
    if (me->device_count != 0) {
        free(me->device_list);
        free(me->device_open_id_list);
        free(me->device_type_list);

        me->device_list = NULL;
        me->device_open_id_list = NULL;
        me->device_type_list = NULL;

        me->device_count = 0;
    }

    me->in_use_device_tap_position = -1;
    me->in_use_device = NULL;
    me->in_use_device_id = 0;
    me->in_use_device_irlen = 0;

    return AJI_NO_ERROR;
}

AJI_ERROR  jtagservice_free_cable(jtagservice_record* me, const DWORD timeout)
{   
    if(me->in_use_hardware_chain_id) {
        c_aji_unlock_chain(me->in_use_hardware_chain_id); //TODO: Make all lock/unlock self-contained then remove this
        me->in_use_hardware_chain_id = NULL;
    }

    if (me->hardware_count != 0) {
        free(me->hardware_list);
        free(me->server_version_info_list);

        me->hardware_list = NULL;
        me->server_version_info_list = NULL;

        //me->in_use_hardware_index = 0;
        me->in_use_hardware = NULL;
        me->in_use_hardware_chain_pid = 0;
        me->in_use_hardware_chain_id = NULL;

        me->hardware_count = 0;
    }
    return AJI_NO_ERROR;
}

AJI_ERROR  jtagservice_free_common(jtagservice_record* me, const DWORD timeout)
{   
    if (me->claims_count) {
        for (DWORD i = 0; i < me->claims_count; ++i) {
            if (0 != me->claims[i].claims_n) {
                free(me->claims[i].claims);
                me->claims[i].claims = NULL;
                me->claims[i].claims_n = 0;
            }
        }

        me->claims_count = 0;
    }

    if (me->appIdentifier) {
        free(me->appIdentifier);
        me->appIdentifier = NULL;
    }

    return AJI_NO_ERROR;
}


int jtagservice_query_main(void) {
    LOG_INFO("Check inputs\n");
    char *quartus_jtag_client_config = getenv("QUARTUS_JTAG_CLIENT_CONFIG");
    if (quartus_jtag_client_config != NULL) {
        LOG_INFO("Configuration file, set via QUARTUS_JTAG_CLIENT_CONFIG, is '%s'\n", 
               quartus_jtag_client_config
        );
    } else {
        LOG_DEBUG("Environment variable QUARTUS_JTAG_CLIENT_CONFIG not set.\n");
    }
    
    LOG_INFO("Initialize Variables\n");
    unsigned int timeout = JTAGSERVICE_TIMEOUT_MS;
    
    DWORD hardware_capacity = 10;
    AJI_HARDWARE *hardware_list = (AJI_HARDWARE*) calloc(hardware_capacity, sizeof(AJI_HARDWARE));
    char **server_version_info_list = (char**) calloc(hardware_capacity, sizeof(char*));
    if(NULL == hardware_list) {
        LOG_ERROR("Failed to allocate memory for hardware_list\n");
        return AJI_NO_MEMORY;
    }

    AJI_ERROR status = AJI_NO_ERROR;
    DWORD hardware_count = hardware_capacity;

    DWORD MYJTAGTIMEOUT = 250; //ms
    DWORD TRIES = 4 * 60;
    for (DWORD i=0; i < TRIES; ++i) {
        status = c_aji_get_hardware2( &hardware_count, hardware_list, server_version_info_list, MYJTAGTIMEOUT);
        if (status != AJI_TIMEOUT) {
            break;
        }
        if (i == 2) {
            LOG_INFO("Connecting to server(s) [.                   ]"
                     "\b\b\b\b\b\b\b\b\b\b"
                     "\b\b\b\b\b\b\b\b\b\b");
        } else if ((i % 12) == 2) {
           LOG_OUTPUT(".");
        }
    } //end for i
    LOG_OUTPUT("\n");

    LOG_INFO("Connection Request reply is %d (%s)", status, c_aji_error_decode(status));

    LOG_INFO("Output Result");
    if (AJI_NO_ERROR == status) {
        LOG_INFO("Number of hardware is %lu", (unsigned long) hardware_count);

        for(unsigned int i=0; i<hardware_count; ++i) {
            AJI_HARDWARE hw = hardware_list[i];
            LOG_INFO("    (%u) device_name=%s hw_name=%s server=%s port=%s chain_id=%p persistent_id=%lu, chain_type=%d, features=%lu, server_version_info_list=%s", 
                   i+1, hw.device_name, hw.hw_name, hw.server, hw.port,  hw.chain_id, (unsigned long) hw.persistent_id, hw.chain_type, (unsigned long) hw.features,
                   server_version_info_list[i]);
           
            //Assume if persistent_id==0,hw is not defined. Felt that .port="Unable to connect" 
            //..is not a strong enough indicator that something is wrong       
            if(hw.persistent_id == 0) {
               LOG_ERROR("        Not a valid device.");
               continue;
            }
            AJI_CHAIN_ID chain_id = hw.chain_id;
            status = c_aji_lock_chain(chain_id, timeout);
            if(AJI_NO_ERROR !=  status ) { 
                LOG_ERROR("       Problem with chain locking. Returned %d (%s)", status, c_aji_error_decode(status));
                continue;
            }

            DWORD device_count = hardware_capacity;
            AJI_DEVICE *device_list = (AJI_DEVICE*) calloc(device_count, sizeof(AJI_DEVICE));
            status = c_aji_read_device_chain(chain_id, &device_count, device_list, 1);
            if(AJI_NO_ERROR !=  status ) { 
                LOG_ERROR("       Problem with getting device on this chain. Returned %d (%s)", status, c_aji_error_decode(status));
                free(device_list);
                c_aji_unlock_chain(chain_id);
                continue;
            }

            LOG_INFO("        Number of devices on chain is %lu\n", (unsigned long) device_count);
            for(DWORD tap_position=0; tap_position<device_count; ++tap_position) {
                AJI_DEVICE device = device_list[tap_position];
                LOG_INFO("        (A%lu) device_id=%08lX, instruction_length=%d, features=%lu, device_name=%s", 
                         (unsigned long) tap_position+1, (unsigned long) device.device_id, device.instruction_length,
                         (unsigned long) device.features, device.device_name
                );
                        
               
                DWORD hier_id_n = hardware_capacity; 
                AJI_HIER_ID *hier_ids = (AJI_HIER_ID*) calloc(hier_id_n, sizeof(AJI_HIER_ID));
                AJI_HUB_INFO *hub_infos = (AJI_HUB_INFO*) calloc(AJI_MAX_HIERARCHICAL_HUB_DEPTH, sizeof(AJI_HUB_INFO));

                status =  c_aji_get_nodes_b(chain_id, tap_position, hier_ids, &hier_id_n, hub_infos);
                if(AJI_NO_ERROR !=  status ) { 
                    LOG_ERROR("       Problem with getting nodes for this tap position. Returned %d (%s)\n", status, c_aji_error_decode(status));
                    free(hier_ids);
                    free(hub_infos);
                    continue;
                }
               
                LOG_INFO("            Number of SLD nodes (hier_id_n)=%lu,\n", (unsigned long) hier_id_n); //hier_id_n = 0 and status=AJI_NO_ERROR if no SLD_HUB
                for(DWORD k=0; k<hier_id_n; ++k) { //With ARRIA10, this loop is entered for the FPGA Tap, if it has SLD nodes.
                    printf("            (B%lu) ", (unsigned long) k);
                    jtagservice_sld_node_printf(&(hier_ids[k]), &(hub_infos[k]));
                    printf("\n");
                } //end for k (hier_id_n)


                int claim_size = 2; 
                
//#if PORT == WINDOWS
                AJI_CLAIM claims[] = {
                      { AJI_CLAIM_IR_SHARED, device.device_id == 0x4BA00477 ? 0b1110u : 0b0000000110u },
                      { AJI_CLAIM_IR_SHARED, device.device_id == 0x4BA00477 ? 0b1111u : 0b1111111111u }, //NO NEED for BYPASS instruction actually
                };
//#else
//                AJI_CLAIM2 claims[] = { 
//                      { AJI_CLAIM_IR_SHARED, 0, device.device_id == 0x4BA00477 ? 0b1110u : 0b0000000110u },
//                      { AJI_CLAIM_IR_SHARED, 0, device.device_id == 0x4BA00477 ? 0b1111u : 0b1111111111u }, //NO NEED for BYPASS instruction actually
//                }; 
//#endif
                char appname[] = "MyApp";
                printf("            (C1-1) Open Device %lX ...\n", (unsigned long) device.device_id); fflush(stdout);
                AJI_OPEN_ID open_id = NULL;   
                
//#if PORT == WINDOWS
                status = c_aji_open_device(chain_id, tap_position, &open_id, claims, claim_size, appname);
//#else 
//                status = c_aji_open_device_a(chain_id, tap_position, &open_id, claims, claim_size, appname);
//#endif

                if(AJI_NO_ERROR  != status) {
                    printf("            Cannot open device. Returned %d (%s). This is not an error if the device is not SOCVHPS or FPGA Tap\n", status, c_aji_error_decode(status));
                    continue;
                }
                    
                printf("            (C1-2) Lock Device ...\n"); fflush(stdout);
                status = c_aji_unlock_chain_lock(chain_id, open_id, AJI_PACK_AUTO); //hardcoded timeout of 10000ms 
                if(AJI_NO_ERROR  != status) {
                    printf("       Cannot lock device. Returned %d (%s)\n", status, c_aji_error_decode(status));
                    c_aji_close_device(open_id);
                    continue;
                }
                /*
                //Need not set state of TAP.
                printf("            (C2-1) Set TAP state  TEST_LOGIC_RESET ...\n"); fflush(stdout);             
                status = c_aji_run_test_idle(open_id, 10); //Doing two clock pulse. One now, and one potentially deferred until the next instruction
                status = c_aji_test_logic_reset(open_id);  
                if(AJI_NO_ERROR  != status) {
                   printf("       Cannot set TAP state to RUN_TEST_IDLE. Returned %d\n", status);
                   c_aji_unlock_lock_chain(open_id, chain_id);
                   c_aji_close_device(open_id);
                   continue;
                }
                */
                QWORD ir_idcode = device.device_id == 0x4BA00477 ? 0b1110 : 0b0000000110; // claims[0].value;
                printf("            (C2-2) Read IDCODE : Set IR (0x%llX)... (only works for Arria10 and ARMVHPS)\n", ir_idcode); fflush(stdout);
                DWORD captured_ir = 0xFFFF;      
                status = c_aji_access_ir(open_id, ir_idcode,  &captured_ir, 0);
                            //if I am in TEST_LOGIC_RESET, I am suppose to get AJI_BAD_TAP STATE but I am not, and I get 0b1 on captured_ir
                if(AJI_NO_ERROR  != status) {
                     printf("       Cannot send IDCODE instruction. Returned %d (%s) \n", status, c_aji_error_decode(status));
                     c_aji_unlock_lock_chain(open_id, chain_id);
                     c_aji_close_device(open_id);
                     continue;
                }
                printf("                 Return IR buffer reads %04lX\n", (unsigned long) captured_ir);
                
                printf("            (C2-3) Read IDCODE : Read DR ...\n"); fflush(stdout);  
                DWORD length_dr = 32,
                      flags = 0,
                      write_offset = 0,
                      write_length = 0, //32,
                      read_offset = 0,
                      read_length = 32;

                BYTE  write_bits[] = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
                BYTE  read_bits[]  = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

                status = c_aji_access_dr( open_id, length_dr, flags, write_offset, write_length, write_bits,  read_offset, read_length, read_bits  );
                if(AJI_NO_ERROR  != status) {
                   printf("       Cannot receive IDCODE output. Returned %d (%s)\n", status, c_aji_error_decode(status));
                   c_aji_unlock_lock_chain(open_id, chain_id);
                   c_aji_close_device(open_id);
                   continue;
                }
                printf("                 Return DR buffer reads %02X%02X%02X%02X %02X%02X%02X%02X\n", 
                            read_bits[3], read_bits[2], read_bits[1], read_bits[0], 
                            read_bits[7], read_bits[6], read_bits[5], read_bits[4]
                );
           

                printf("            (C1-3) Flush Device ...\n"); fflush(stdout);
                c_aji_flush(open_id);
                   
                printf("            (C1-4) Unlock Device ...\n"); fflush(stdout);
                c_aji_unlock_lock_chain(open_id, chain_id);
                    
                printf("            (C1-5) Close Device ...\n"); fflush(stdout);
                c_aji_close_device(open_id);
                printf("(C1-5) Done.\n");

                free(hier_ids);
                free(hub_infos);
            } // end for tap_position (device_count)
                 
 
           free(device_list);
           c_aji_unlock_chain(chain_id);
       } //end for i (hardware List)
    } //end if status


    printf("Free\n");
    free(hardware_list);
 
    printf("Completed\n");
	return 0;
}

/**
 * Print SLD Node information
 * 
 * \param hier_id The SLD node information
 * \param hub_info Supplimentary hub information.
 *        if you do not want it displayed
 */
void jtagservice_sld_node_printf(const AJI_HIER_ID* hier_id, const AJI_HUB_INFO* hub_infos) {
    printf(" idcode=%08lX position_n=%lu position: ( ",
        (unsigned long) hier_id->idcode,
        (unsigned long) hier_id->position_n
    );
    if (hub_infos) {
        for (int m = 0; m <= hier_id->position_n; ++m) {
            printf("%d ", hier_id->positions[m]);
        } //end for m
        printf(")  hub_infos: ");
        for (int m = 0; m <= hier_id->position_n; ++m) {
            printf(" (Hub %d) bridge_idcode=%08lX, hub_id_code=%08lX", 
                m, 
                (unsigned long) ( m == 0 ? 0 : hub_infos->bridge_idcode[m]), 
                (unsigned long) (hub_infos->hub_idcode[m])
            );
        } //end for m (bridge)
    }
}
