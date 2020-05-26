//START_MODULE_HEADER////////////////////////////////////////////////////////
//
// $Header$
//
// Description: Entries points for AJI with GNU name decoration
//
// Authors:     Andrew Draper
//
//              Copyright (c) Altera Corporation 2013
//              All rights reserved.
//
//END_MODULE_HEADER//////////////////////////////////////////////////////////

//START_ALGORITHM_HEADER/////////////////////////////////////////////////////
//
//
//END_ALGORITHM_HEADER///////////////////////////////////////////////////////
//

// INCLUDE FILES ////////////////////////////////////////////////////////////

#include "aji.h"

#define AJI_API //JTAG_DLLEXPORT

//#include "aji_sys.h"

//START_FUNCTION_HEADER////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

#if 0
AJI_API const char * aji_get_error_info(void);

AJI_API AJI_ERROR aji_get_potential_hardware(DWORD * hardware_count, AJI_HARDWARE * hardware_list);
#endif

AJI_API AJI_ERROR _Z16aji_get_hardwarePjP12AJI_HARDWAREj(DWORD * hardware_count, AJI_HARDWARE * hardware_list, DWORD timeout /*  = 0x7FFFFFFF */); //cinlyooi
AJI_API AJI_ERROR _Z17aji_get_hardware2PjP12AJI_HARDWAREPPcj(DWORD * hardware_count, AJI_HARDWARE * hardware_list, char **server_version_info_list, DWORD timeout /*  = 0x7FFFFFFF */); //cinlyooi

#if 0
AJI_API AJI_ERROR _Z17aji_find_hardwarePKcP12AJI_HARDWAREm(DWORD persistent_id, AJI_HARDWARE * hardware, DWORD timeout);
#endif

AJI_API AJI_ERROR _Z17aji_find_hardwarePKcP12AJI_HARDWAREj(const char * hw_name, AJI_HARDWARE * hardware, DWORD timeout); //cinlyooi
AJI_API AJI_ERROR _Z17aji_find_hardwarejP12AJI_HARDWAREj(unsigned int, AJI_HARDWARE*, unsigned int); //cinlyooi, not in original 

#if 0
AJI_API AJI_ERROR aji_print_hardware_name(AJI_CHAIN_ID chain_id, char * hw_name, DWORD hw_name_len);
#endif

AJI_API AJI_ERROR _Z23aji_print_hardware_nameP9AJI_CHAINPcmbPm(AJI_CHAIN_ID chain_id, char * hw_name, DWORD hw_name_len, _Bool explicit_localhost, DWORD * needed_hw_name_len /* = NULL */);

#if 0
AJI_API AJI_ERROR aji_add_hardware(const AJI_HARDWARE * hardware);

AJI_API AJI_ERROR aji_change_hardware_settings(AJI_CHAIN_ID chain_id, const AJI_HARDWARE * hardware);

AJI_API AJI_ERROR aji_remove_hardware(AJI_CHAIN_ID chain_id);_Bool   

AJI_API AJI_ERROR aji_add_remote_server(const char * server, const char * password);

AJI_API AJI_ERROR aji_add_remote_server(const char * server, const char * password, _Bool temporary);
#endif

AJI_API AJI_ERROR _Z15aji_get_serversPjPPKcb(DWORD * server_count, const char * * servers, _Bool temporary);

#if 0
AJI_API AJI_ERROR aji_enable_remote_clients(_Bool enable, const char * password);

AJI_API AJI_ERROR aji_get_remote_clients_enabled(_Bool * enable);
#endif

AJI_API AJI_ERROR _Z14aji_add_recordPK10AJI_RECORD(const AJI_RECORD * record);

AJI_API AJI_ERROR _Z17aji_remove_recordPK10AJI_RECORD(const AJI_RECORD * record);

AJI_API AJI_ERROR _Z15aji_get_recordsPjP10AJI_RECORD(DWORD * record_n, AJI_RECORD * records);

AJI_API AJI_ERROR _Z15aji_get_recordsPKcPjP10AJI_RECORD(const char * server, DWORD * record_n, AJI_RECORD * records);

#if 0
AJI_API AJI_ERROR aji_get_records(const char * server, DWORD * record_n, AJI_RECORD * records, DWORD autostart_type_n, const char * * autostart_types);

AJI_API AJI_ERROR aji_replace_local_jtagserver(const char * replace);

AJI_API AJI_ERROR aji_enable_local_jtagserver(_Bool enable);

AJI_API AJI_ERROR aji_configuration_in_memory(_Bool enable);

AJI_API AJI_ERROR aji_load_quartus_devices(const char * filename);

AJI_API AJI_ERROR aji_scan_device_chain(AJI_CHAIN_ID chain_id);

AJI_API AJI_ERROR aji_define_device(AJI_CHAIN_ID chain_id, DWORD tap_position, const AJI_DEVICE * device);

AJI_API AJI_ERROR aji_define_device(const AJI_DEVICE * device);

AJI_API AJI_ERROR aji_undefine_device(const AJI_DEVICE * device);

AJI_API AJI_ERROR aji_get_defined_devices(DWORD * device_count, AJI_DEVICE * device_list);

AJI_API AJI_ERROR aji_get_local_quartus_devices(DWORD * device_count, AJI_DEVICE * device_list);
#endif

AJI_API AJI_ERROR _Z21aji_read_device_chainP9AJI_CHAINPjP10AJI_DEVICEb(AJI_CHAIN_ID chain_id, DWORD * device_count, AJI_DEVICE * device_list, _Bool auto_scan); //cinlyooi

#if 0
AJI_API AJI_ERROR aji_set_parameter(AJI_CHAIN_ID chain_id, const char * name, DWORD value);

AJI_API AJI_ERROR aji_set_parameter(AJI_CHAIN_ID chain_id, const char * name, const BYTE * value, DWORD valuelen);

AJI_API AJI_ERROR aji_get_parameter(AJI_CHAIN_ID chain_id, const char * name, DWORD * value);

AJI_API AJI_ERROR aji_get_parameter(AJI_CHAIN_ID chain_id, const char * name, BYTE * value, DWORD * valuemax, DWORD valuetx = 0);
#endif

//@deprecated, using AJI_CLAIM2 version
AJI_API AJI_ERROR _Z15aji_open_deviceP9AJI_CHAINjPP8AJI_OPENPK9AJI_CLAIMjPKc(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID * open_id, const AJI_CLAIM * claims, DWORD claim_n, const char * application_name);

AJI_API AJI_ERROR _Z15aji_open_deviceP9AJI_CHAINjPP8AJI_OPENPK10AJI_CLAIM2jPKc(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_OPEN_ID * open_id, const AJI_CLAIM2 * claims, DWORD claim_n, const char * application_name);

AJI_API AJI_ERROR _Z16aji_close_deviceP8AJI_OPEN(AJI_OPEN_ID open_id);

#if 0
AJI_API AJI_ERROR aji_open_entire_device_chain(AJI_CHAIN_ID chain_id, AJI_OPEN_ID * open_id, AJI_CHAIN_TYPE style, const char * application_name);
#endif
AJI_API AJI_ERROR _Z28aji_open_entire_device_chainP9AJI_CHAINPP8AJI_OPEN14AJI_CHAIN_TYPEPKc(AJI_CHAIN_ID chain_id, AJI_OPEN_ID * open_id, AJI_CHAIN_TYPE style, const char * application_name); //cinlyooi, not in original
AJI_API AJI_ERROR _Z8aji_lockP8AJI_OPENj14AJI_PACK_STYLE(AJI_OPEN_ID open_id, DWORD timeout, AJI_PACK_STYLE pack_style); //cinlyooi

AJI_API AJI_ERROR _Z10aji_unlockP8AJI_OPEN(AJI_OPEN_ID open_id); //cinlyooi

#if 0
AJI_API AJI_ERROR aji_unlock_lock(AJI_OPEN_ID unlock_id, AJI_OPEN_ID lock_id);

AJI_API AJI_ERROR aji_unlock_chain_lock(AJI_CHAIN_ID unlock_id, AJI_OPEN_ID lock_id, AJI_PACK_STYLE pack_style);

AJI_API AJI_ERROR aji_unlock_lock_chain(AJI_OPEN_ID unlock_id, AJI_CHAIN_ID lock_id);
#endif

AJI_API AJI_ERROR _Z15aji_unlock_lockP8AJI_OPENS0_(AJI_OPEN_ID unlock_id, AJI_OPEN_ID lock_id); //cinlyooi

AJI_API AJI_ERROR _Z9aji_flushP8AJI_OPEN(AJI_OPEN_ID open_id); //cinlyooi

AJI_API AJI_ERROR _Z8aji_pushP8AJI_OPENm(AJI_OPEN_ID open_id, DWORD timeout);

AJI_API AJI_ERROR _Z18aji_set_checkpointP8AJI_OPENm(AJI_OPEN_ID open_id, DWORD checkpoint);

AJI_API AJI_ERROR _Z18aji_get_checkpointP8AJI_OPENPm(AJI_OPEN_ID open_id, DWORD * checkpoint);

AJI_API AJI_ERROR _Z14aji_lock_chainP9AJI_CHAINj(AJI_CHAIN_ID chain_id, DWORD timeout); //cinlyooi

AJI_API AJI_ERROR _Z16aji_unlock_chainP9AJI_CHAIN(AJI_CHAIN_ID chain_id); //cinlyooi

AJI_API AJI_ERROR _Z21aji_unlock_chain_lockP9AJI_CHAINP8AJI_OPEN14AJI_PACK_STYLE(AJI_CHAIN_ID unlock_id, AJI_OPEN_ID lock_id, AJI_PACK_STYLE pack_style); //cinlyooi

AJI_API AJI_ERROR _Z21aji_unlock_lock_chainP8AJI_OPENP9AJI_CHAIN(AJI_OPEN_ID unlock_id, AJI_CHAIN_ID lock_id);//cinlyooi


AJI_API AJI_ERROR _Z13aji_access_irP8AJI_OPENjPjj(AJI_OPEN_ID open_id, DWORD instruction, DWORD * captured_ir, DWORD flags);  //cinlyooi, not in original

#if 0
AJI_API AJI_ERROR aji_access_ir(AJI_OPEN_ID open_id, DWORD length_ir, const BYTE * write_bits, BYTE * read_bits, DWORD flags = 0);

AJI_API AJI_ERROR aji_access_ir_multiple(DWORD num_devices, const AJI_OPEN_ID * open_id, const DWORD * instructions, DWORD * captured_irs);
#endif

AJI_API AJI_ERROR _Z13aji_access_irP8AJI_OPENjPKhPhj(AJI_OPEN_ID open_id, DWORD length_ir, const BYTE * write_bits, BYTE * read_bits, DWORD flags); //cinlyooi

AJI_API AJI_ERROR _Z22aji_access_ir_multiplejPKP8AJI_OPENPKjPj(DWORD num_devices, const AJI_OPEN_ID * open_id, const DWORD * instructions, DWORD * captured_irs); //cinlyooi




AJI_API AJI_ERROR _Z13aji_access_drP8AJI_OPENjjjjPKhjjPh(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD read_offset, DWORD read_length, BYTE * read_bits);  //cinlyooi

AJI_API AJI_ERROR _Z13aji_access_drP8AJI_OPENjjjjPKhjjPhj(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD read_offset, DWORD read_length, BYTE * read_bits, DWORD batch); //cinlyooi

#if 0
AJI_API AJI_ERROR aji_access_dr(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD read_offset, DWORD read_length, BYTE * read_bits, DWORD batch, DWORD * timestamp);

AJI_API AJI_ERROR aji_access_dr_multiple(DWORD num_devices, DWORD flags, const AJI_OPEN_ID * open_id, const DWORD * length_dr, const DWORD * write_offset, const DWORD * write_length, const BYTE * const * write_bits, const DWORD * read_offset, const DWORD * read_length, BYTE * const * read_bits);
#endif

AJI_API AJI_ERROR _Z13aji_access_drP8AJI_OPENjjjjPKhjjPhjPj(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD read_offset, DWORD read_length, BYTE * read_bits, DWORD batch, DWORD * timestamp); //cinlyooi, not in original

AJI_API AJI_ERROR _Z20aji_access_dr_repeatP8AJI_OPENjjjjPKhjjPhS2_S2_S2_S2_j(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD read_offset, DWORD read_length, BYTE * read_bits, const BYTE * success_mask, const BYTE * success_value, const BYTE * failure_mask, const BYTE * failure_value, DWORD timeout); //cinlyooi

AJI_API AJI_ERROR _Z22aji_access_dr_multiplejjPKP8AJI_OPENPKjS4_S4_PKPKhS4_S4_PKPh(DWORD num_devices, DWORD flags, const AJI_OPEN_ID * open_id, const DWORD * length_dr, const DWORD * write_offset, const DWORD * write_length, const BYTE * const * write_bits, const DWORD * read_offset, const DWORD * read_length, BYTE * const * read_bits); //cinlyooi, not in original

#if 0
AJI_API AJI_ERROR aji_access_dr_repeat_multiple(DWORD num_devices, DWORD flags, const AJI_OPEN_ID * open_id, const DWORD length_dr, const DWORD write_offset, const DWORD write_length, const BYTE * const * write_bits, const DWORD read_offset, const DWORD read_length, BYTE * const * read_bits, const BYTE * success_mask, const BYTE * success_value, const BYTE * failure_mask, const BYTE * failure_value, DWORD timeout);

AJI_API AJI_ERROR aji_any_sequence(AJI_OPEN_ID open_id, DWORD num_tcks, const BYTE * tdi_bits, const BYTE * tms_bits, BYTE * tdo_bits);
#endif

AJI_API AJI_ERROR _Z29aji_access_dr_repeat_multiplejjPKP8AJI_OPENjjjPKPKhjjPKPhS4_S4_S4_S4_j(DWORD num_devices, DWORD flags, const AJI_OPEN_ID * open_id, const DWORD length_dr, const DWORD write_offset, const DWORD write_length, const BYTE * const * write_bits, const DWORD read_offset, const DWORD read_length, BYTE * const * read_bits, const BYTE * success_mask, const BYTE * success_value, const BYTE * failure_mask, const BYTE * failure_value, DWORD timeout); //cinlyooi


AJI_API AJI_ERROR _Z17aji_run_test_idleP8AJI_OPENj(AJI_OPEN_ID open_id, DWORD num_clocks); //cinlyooi

#if 0
AJI_API AJI_ERROR aji_run_test_idle(AJI_OPEN_ID open_id, DWORD num_clocks, DWORD flags);

AJI_API AJI_ERROR aji_test_logic_reset(AJI_OPEN_ID open_id);
#endif

AJI_API AJI_ERROR _Z20aji_test_logic_resetP8AJI_OPEN(AJI_OPEN_ID open_id); //cinlyooi, not in original

AJI_API AJI_ERROR _Z17aji_run_test_idleP8AJI_OPENjj(AJI_OPEN_ID open_id, DWORD num_clocks, DWORD flags);  //cinlyooi

AJI_API AJI_ERROR _Z9aji_delayP8AJI_OPENm(AJI_OPEN_ID open_id, DWORD timeout_microseconds);

#if 0
AJI_API AJI_ERROR aji_watch_status(AJI_OPEN_ID open_id, DWORD status_mask, DWORD status_value, DWORD poll_interval);
#endif

AJI_API AJI_ERROR _Z12aji_watch_drP8AJI_OPENmmmmPKhmmS2_S2_m(AJI_OPEN_ID open_id, DWORD length_dr, DWORD flags, DWORD write_offset, DWORD write_length, const BYTE * write_bits, DWORD watch_offset, DWORD watch_length, const BYTE * watch_polarity, const BYTE * watch_success_mask, DWORD poll_interval);

AJI_API AJI_ERROR _Z23aji_get_watch_triggeredP8AJI_OPENPb(AJI_OPEN_ID open_id, _Bool * triggered);

#if 0
AJI_API AJI_ERROR aji_passive_serial_download(AJI_OPEN_ID open_id, const BYTE * data, DWORD length, DWORD * status);

AJI_API AJI_ERROR aji_direct_pin_control(AJI_OPEN_ID open_id, DWORD mask, DWORD outputs, DWORD * inputs);

AJI_API AJI_ERROR aji_active_serial_command(AJI_OPEN_ID open_id, DWORD write_length, const BYTE * write_data, DWORD read_length, BYTE * read_data);

AJI_API AJI_ERROR aji_active_serial_command_repeat(AJI_OPEN_ID open_id, DWORD write_length, const BYTE * write_data, DWORD read_length, BYTE * read_data, const BYTE * success_mask, const BYTE * success_value, const BYTE * failure_mask, const BYTE * failure_value, DWORD timeout);
#endif

AJI_API void _Z28aji_register_output_callbackPFvPvmPKcES_(void( * output_fn)(void * handle, DWORD level, const char * line), void * handle);

#if 0
AJI_API void aji_register_progress_callback(AJI_OPEN_ID open_id, void ( * progress_fn)(void * handle, DWORD bits), void * handle);

AJI_API AJI_ERROR aji_get_nodes(AJI_CHAIN_ID chain_id, DWORD tap_position, DWORD * idcodes, DWORD * idcode_n);

AJI_API AJI_ERROR aji_get_nodes(AJI_CHAIN_ID chain_id, DWORD tap_position, DWORD * idcodes, DWORD * idcode_n, DWORD * hub_info);

AJI_API AJI_ERROR aji_open_node(AJI_CHAIN_ID chain_id, DWORD tap_position, DWORD idcode, AJI_OPEN_ID * node_id, const AJI_CLAIM * claims, DWORD* claim_n, const char * application_name);
#endif

AJI_API AJI_ERROR _Z13aji_get_nodesP9AJI_CHAINjP11AJI_HIER_IDPjP12AJI_HUB_INFO(AJI_CHAIN_ID chain_id, DWORD tap_position, AJI_HIER_ID *hier_ids, DWORD* hier_id_n, AJI_HUB_INFO *hubinfos);

AJI_API AJI_ERROR _Z13aji_find_nodeP9AJI_CHAINiimPK9AJI_CLAIMmPKcPP8AJI_OPENPm(AJI_CHAIN_ID chain_id, int device_index /* -1 for any */, int instance /* -1 for any */, DWORD type, const AJI_CLAIM * claims, DWORD claim_n, const char * application_name, AJI_OPEN_ID * node_id, DWORD * node_n);

AJI_API AJI_ERROR _Z17aji_get_node_infoP8AJI_OPENPmS1_(AJI_OPEN_ID node_id, DWORD * device_index, DWORD * info);

AJI_API AJI_ERROR _Z18aji_access_overlayP8AJI_OPENmPm(AJI_OPEN_ID node_id, DWORD overlay, DWORD * captured_overlay);
#ifdef __cplusplus
} //end extern "C"
#endif 

