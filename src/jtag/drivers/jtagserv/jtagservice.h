#ifndef JTAGSERVICE_H_INCLUDED
#define JTAGSERVICE_H_INCLUDED

#if IS_WIN32
#include <windows.h>
#endif

#include "h/aji.h"
#include "h/c_aji.h"


#define JTAGSERVICE_TIMEOUT_MS 10000

#define IR_ARM_ABORT  0b1000 // dr_len=35
#define IR_ARM_DPACC  0b1010 // dr_len=35
#define IR_ARM_APACC  0b1011 // dr_len=35
#define IR_ARM_IDCODE 0b1110 // dr_len=32
#define IR_ARM_BYPASS 0b1111 // dr_len=1

#define IR_RISCV_BYPASS0       0x00  // dr_len=1
#define IR_RISCV_IDCODE        0x01  // dr_len=32       
#define IR_RISCV_DTMCS         0x10  // dr_len=32
#define IR_RISCV_DMI           0x11  // dr_len=<address_length>+34
#define IR_RISCV_BYPASS        0x1f  // dr_len=1

#define IR_VJTAG_USER0     0xC; // dr_len=<not important>
#define IR_VJTAG_USER1     0xE; // dr_len=<not important>

typedef struct CLAIM_RECORD CLAIM_RECORD;
struct CLAIM_RECORD {
    DWORD claims_n;    ///! number of claims
    AJI_CLAIM* claims; ///! IR claims
};

#define DEVICE_TYPE_COUNT 4
enum DEVICE_TYPE {
    UNKNOWN = 0, ///! UNKNWON DEVICE
    ARM = 1, ///! ARM device, with IR length = 4 bit
    RISCV = 2, ///! RISCV device, with IR length = 5 bit 
    VJTAG = 3, ///! vJTAG/SLD 
};

// Windows does not like typedef enum
#if PORT!=WINDOWS 
typedef enum DEVICE_TYPE DEVICE_TYPE;
#endif 

typedef struct SLD_RECORD SLD_RECORD;
struct SLD_RECORD {
    DWORD idcode;
    DWORD node_position;
};

typedef struct jtagservice_record jtagservice_record;
struct jtagservice_record {
    char *appIdentifier;

    //data members
    //() Cable
    /* Not sure AJI_HARDWARE can survive function boundarie
     * so store its persistent ID as backup.
     * If proven that AJI_HARDWARE can work, then keep it.
     */
    DWORD          hardware_count; 
    AJI_HARDWARE  *hardware_list;
    char         **server_version_info_list;
    
    //DWORD         in_use_hardware_index; 
    //< Current detection system does not allow this to be determined
    AJI_HARDWARE *in_use_hardware;
    DWORD         in_use_hardware_chain_pid;
    AJI_CHAIN_ID  in_use_hardware_chain_id;

    //() Tap device
    DWORD        device_count;
    AJI_DEVICE  *device_list;
    AJI_OPEN_ID *device_open_id_list;
    DEVICE_TYPE *device_type_list;
    
    DWORD       in_use_device_tap_position;
    AJI_DEVICE *in_use_device;
    DWORD       in_use_device_id; 
    BYTE        in_use_device_irlen;

    //SLD / Virtual JTAG
    DWORD  *hier_id_n; //< How many SLD node per TAP device.
                       //< size = device_count since one number
                       //< per TAP (device), in the same order 
                       //< as device_list
    AJI_HIER_ID **hier_ids; //< hier_ids[TAP][SLD] where TAP =
                            //< [0, device_count), in the same  
                            //< order as device_list,
                            //< and SLD = [0, hier_id_n[TAP])
    AJI_HUB_INFO** hub_infos; //< hub_infos[TAP][HUB] where 
                      //< TAP = [0, device_count), in the same  
                      //< order as device_list, and hub =
                      //< [0, AJI_MAX_HIERARCHICAL_HUB_DEPTH)

    DWORD claims_count;
    CLAIM_RECORD *claims; ///! List of AJI_CLAIM, by DEVICE_TYPE
};
 
AJI_ERROR jtagservice_init(jtagservice_record *me, DWORD timeout);
AJI_ERROR jtagservice_free(jtagservice_record *me, DWORD timeout);

int jtagservice_query_main(void);
void jtagservice_display_sld_nodes(const jtagservice_record me);


void jtagservice_sld_node_printf(
    const AJI_HIER_ID* hier_id, 
    const AJI_HUB_INFO* hub_info);
AJI_ERROR jtagservice_device_index_by_idcode(
    const DWORD idcode,
    const AJI_DEVICE* tap_list, const DWORD tap_count,
    DWORD* tap_index);
AJI_ERROR jtagservice_hier_id_index_by_idcode(
    const DWORD idcode,
    const AJI_HIER_ID *vtap_list, const DWORD vtap_count,
    DWORD* vtap_index);

#endif //JTAGSERVICE_H_INCLUDED

