//# START_MODULE_HEADER/////////////////////////////////////////////////////////
//#
//# $Header$
//#
//# Description:
//#
//# Authors:     Alan Whitaker, Andrew Draper
//#
//#              Copyright (c) Altera Corporation 2000 - 2001
//#              All rights reserved.
//#
//# END_MODULE_HEADER///////////////////////////////////////////////////////////

//# START_ALGORITHM_HEADER//////////////////////////////////////////////////
//#
//#
//# END_ALGORITHM_HEADER////////////////////////////////////////////////////
//#

//# INTERFACE DESCRIPTION //////////////////////////////////////////////////

#ifndef AJI_SYS_H_INCLUDED
#define AJI_SYS_H_INCLUDED

//# INCLUDE FILES //////////////////////////////////////////////////////////

//# MACRO DEFINITIONS //////////////////////////////////////////////////////

#if PORT==WINDOWS
#define AJI_API __declspec(dllimport)
#else
#define AJI_API
#endif

typedef unsigned char       BYTE;
// On UNIX 64-bit, long is a 64 bit type (4 words)
#if PORT==UNIX && defined(MODE_64_BIT)
    typedef unsigned int    DWORD;
#else
    typedef unsigned long   DWORD;
#endif

#if PORT==WINDOWS
    typedef unsigned __int64    QWORD;
#else
    typedef unsigned long long  QWORD;
#endif

// This is to maintain consistency between the programmer UI and JTAGSERVER
// since we need a "hint" as to what the network cable is really called
// Marketing may change the name, and we need to make sure it is updated in
// both places since a string compare is done.
#define AJI_NETWORK_CABLE_NAME "EthernetBlaster"

#define AJI_MAX_HIERARCHICAL_HUB_DEPTH 8
//# ENUMERATIONS ///////////////////////////////////////////////////////////
typedef enum AJI_ERROR AJI_ERROR;
enum AJI_ERROR // These are transmitted from client to server so must not change.
{
    AJI_NO_ERROR               = 0,
    AJI_FAILURE                = 1,
    AJI_TIMEOUT                = 2,

    AJI_UNKNOWN_HARDWARE      = 32,
    AJI_INVALID_CHAIN_ID      = 33,
    AJI_LOCKED                = 34,
    AJI_NOT_LOCKED            = 35,

    AJI_CHAIN_IN_USE          = 36,
    AJI_NO_DEVICES            = 37,
    AJI_CHAIN_NOT_CONFIGURED  = 38,
    AJI_BAD_TAP_POSITION      = 39,
    AJI_DEVICE_DOESNT_MATCH   = 40,
    AJI_IR_LENGTH_ERROR       = 41,
    AJI_DEVICE_NOT_CONFIGURED = 42,
    AJI_CHAINS_CLAIMED        = 43,

    AJI_INVALID_OPEN_ID       = 44,
    AJI_INVALID_PARAMETER     = 45,
    AJI_BAD_TAP_STATE         = 46,
    AJI_TOO_MANY_DEVICES      = 47,
    AJI_IR_MULTIPLE           = 48,
    AJI_BAD_SEQUENCE          = 49,
    AJI_INSTRUCTION_CLAIMED   = 50,
    AJI_MODE_NOT_AVAILABLE    = 51, // The mode requested is not supported by this hardware
    AJI_INVALID_DUMMY_BITS    = 52, // The number of dummmy bits is out of range

    AJI_FILE_ERROR            = 80,
    AJI_NET_DOWN              = 81,
    AJI_SERVER_ERROR          = 82,
    AJI_NO_MEMORY             = 83, // Out of memory when configuring
    AJI_BAD_PORT              = 84, // Port number (eg LPT1) does not exist
    AJI_PORT_IN_USE           = 85,
    AJI_BAD_HARDWARE          = 86, // Hardware (eg byteblaster cable) not connected to port
    AJI_BAD_JTAG_CHAIN        = 87, // JTAG chain connected to hardware is broken
    AJI_SERVER_ACTIVE         = 88, // Another thread in this process is using the JTAG Server
    AJI_NOT_PERMITTED         = 89,
    AJI_HARDWARE_DISABLED     = 90, // Disable input to On-board cable is driven active

    AJI_HIERARCHICAL_HUB_NOT_SUPPORTED = 125,
    AJI_UNIMPLEMENTED        = 126,
    AJI_INTERNAL_ERROR       = 127,

// These errors are generated on the client side only
    AJI_NO_HUBS              = 256,
    AJI_TOO_MANY_HUBS        = 257,
    AJI_NO_MATCHING_NODES    = 258,
    AJI_TOO_MANY_MATCHING_NODES = 259,
    AJI_TOO_MANY_HIERARCHIES = 260
};

typedef enum AJI_CHAIN_TYPE AJI_CHAIN_TYPE;
enum AJI_CHAIN_TYPE // These are transmitted from client to server so must not change.
{
    AJI_CHAIN_UNKNOWN = 0,
    AJI_CHAIN_JTAG    = 1,
    AJI_CHAIN_SERIAL  = 2,
    AJI_CHAIN_PASSIVE = 2, // Passive serial (EPC2 style)
    AJI_CHAIN_ACTIVE  = 3  // Active serial (Motorola SPI device)
};

enum AJI_FEATURE // These are transmitted from client to server so must not change.
{
    AJI_FEATURE_DUMMY    = 0x0001, // This is a dummy chain which does not have hardware
    AJI_FEATURE_DYNAMIC  = 0x0002, // This chain was auto-detected and can't be removed
    AJI_FEATURE_SPECIAL  = 0x0004, // This chain needs special setup before use
    AJI_FEATURE_LED      = 0x0100, // This chain has a controllable status LED
    AJI_FEATURE_BUTTON   = 0x0200, // This chain has a "Start" button
    AJI_FEATURE_INSOCKET = 0x0400, // This chain supports in-socket programming
    AJI_FEATURE_JTAG     = 0x0800, // This chain supports JTAG access
    AJI_FEATURE_PASSIVE  = 0x1000, // This chain supports passive serial download
    AJI_FEATURE_ACTIVE   = 0x2000, // This chain supports active serial access
    AJI_FEATURE_SECONDARY= 0x4000, // This chain was created by a secondary server
    AJI_FEATURE_IDENTIFY = 0x8000  // This chain supports an LED identify feature
};

typedef enum AJI_PACK_STYLE AJI_PACK_STYLE;
enum AJI_PACK_STYLE // These are used in the client API so must not change.
{
    AJI_PACK_NEVER  = 0,
    AJI_PACK_AUTO   = 1,
    AJI_PACK_MANUAL = 2,
    AJI_PACK_STREAM = 3
};

enum AJI_DR_FLAGS // These are transmitted from client to server so must not change.
{
    AJI_DR_UNUSED_0       = 1,  // Allow zeros to be written to unspecified bits
    AJI_DR_UNUSED_0_OMIT  = 3,  // Allow zeros at the TDI end, allow any value at TDO end
    AJI_DR_UNUSED_X       = 15, // Allow any value to be written to unspecified bits
    AJI_DR_NO_SHORT       = 16, // Must clock all bits through (disable optimisations)
    AJI_DR_END_PAUSE_DR   = 32, // End the dr scan in the PAUSE_DR state
    AJI_DR_START_PAUSE_DR = 64, // Allow the dr scan to start in the PAUSE_DR state
    AJI_DR_NO_RESTORE     = 128 // Do not reload previous instruction and overlay after relocking
};

enum AJI_IR_FLAGS // These are transmitted from client to server so must not change.
{
    AJI_IR_COULD_BREAK   = 2  // IR chain could break at this device on next scan
};

enum AJI_RTI_FLAGS // These are transmitted from client to server so must not change.
{
    AJI_ACCURATE_CLOCK = 1, // Clock must run at exactly the clock rate set for this scan
    AJI_EXIT_RTI_STATE = 2  // Exit from RUN-TEST-IDLE state after scan
};

enum AJI_DEVFEAT // Device feature bitfield
                 // These are transmitted from client to server so must not change.
{
    AJI_DEVFEAT_BAD_IR_CAPTURE          = 1, // Captured IR value does not always end 'b01
    AJI_DEVFEAT_USER01_BREAK_IR         = 2, // USER01 instructions can break the IR chain
    AJI_DEVFEAT_POSSIBLE_HUB            = 4, // Potentially contains a SLD HUB
    AJI_DEVFEAT_COMMAND_RESPONSE_CHAIN  = 8  // Support COMMAND and RESPONSE scan chains
};

enum AJI_PINS    // Direct pin control bitfield
                 // These are transmitted from client to server so must not change.

{                  // Mode:  JTAG   Passive   Active
    AJI_PIN_TDI = 0x01,  //   TDI     DATA      ASDI
    AJI_PIN_TMS = 0x02,  //   TMS   nCONFIG  nCONFIG
    AJI_PIN_TCK = 0x04,  //   TCK     DCLK      DCLK
    AJI_PIN_NCS = 0x08,  //     -        -       nCS
    AJI_PIN_NCE = 0x10,  //     -        -       nCE

    AJI_PIN_TDO = 0x01,  //   TDO  CONFDONE CONFDONE
    AJI_PIN_NST = 0x02   //  RTCK   nSTATUS  DATAOUT
};

//# FORWARD REFERENCES FOR CLASSES /////////////////////////////////////////

//# TYPEDEFS ///////////////////////////////////////////////////////////////
typedef struct AJI_CHAIN * AJI_CHAIN_ID;  //} Expecting to cause problem later
typedef struct AJI_OPEN  * AJI_OPEN_ID;   //} as I changed "class" to "struct"

//# CLASS AND STRUCTURE DECLARATIONS ///////////////////////////////////////

// The AJI_HARDWARE class represents one chain attached to one hardware driver.
// This chain can either be a jtag chain or a passive serial chain (as
// indicated by chain_type).

typedef struct AJI_HARDWARE AJI_HARDWARE;
struct AJI_HARDWARE
{
    AJI_CHAIN_ID   chain_id;
    DWORD          persistent_id;
    const char   * hw_name;     // Name of this type of hardware
    const char   * port;
    const char   * device_name; // Name given to hardware by user (or NULL)
    AJI_CHAIN_TYPE chain_type;
    const char   * server;      // Name of server this is attached to (NULL if local)
    DWORD          features;    // Logical or of AJI_FEATURE_xxx
};

// The AJI_DEVICE class represents the information which the server needs to
// know about one JTAG TAP controller on a JTAG chain.

typedef struct AJI_DEVICE AJI_DEVICE;
struct AJI_DEVICE
{
    DWORD         device_id;
    DWORD         mask;                 // 1 bit in mask indicates X in device_id
    BYTE          instruction_length;
    DWORD         features;             // Bitwise or of AJI_DEVFEAT
    const char  * device_name;          // May be NULL
};

typedef struct AJI_HUB_INFO AJI_HUB_INFO;
struct AJI_HUB_INFO
{
    DWORD bridge_idcode[AJI_MAX_HIERARCHICAL_HUB_DEPTH];
    DWORD hub_idcode[AJI_MAX_HIERARCHICAL_HUB_DEPTH];
};

typedef struct AJI_HIER_ID AJI_HIER_ID;
struct AJI_HIER_ID
{
    DWORD idcode;                                   // 32-bit node info
    BYTE position_n;                                // Hierarchy level, 0 indicates top-level node
    BYTE positions[AJI_MAX_HIERARCHICAL_HUB_DEPTH]; // Position of node in each hierarchy
};

// The AJI_CLAIM structure is used to pass in a list of the instructions,
// overlay values etc which we want to use, either exclusively or shared
// with other clients.  The type field describes what we are trying to claim.

// The least significant bits hold the type of resource being claimed.  The
// following bits are valid for all resource types:
// - The SHARED bit allows more then one client to claim the resource at the
//   same time (providing that the claim types are compatible).

// The following bits are valid for IR resources (those which represent
// access to JTAG instructions):
// - The OVERLAY bit tells the JTAG server that this is the overlay register
//   for the device (a register whose value changes the meaning of the
//   overlaid registers).  Exclusive access to an overlay register is not
//   allowed.
// - The OVERLAID bit tells the JTAG server that the meaning of this register
//   is affected by the value in the overlay register.  If this value is
//   present in the IR when the device is locked then the JTAG server will
//   ensure that the overlay register holds the correct value.

// A device can only have one overlay register (which must be listed before
// all overlaid registers in the list of claims).

// For exclusive and shared claims the first client to claim an instruction
// is successful.  Subsequent claims for the same IR / OVERLAY are succeed
// only if all the claims are SHARED.

// A weak claim is always successful at open time, however the availability
// of the IR / OVERLAY value are checked each time it is used.  A subsequent
// exclusive or shared claim will override a weak claim for a resource.
// A weak claim of ~0 allows weak access to all values in that resource.
// Because weak claims are checked at the time aji_access_ir/dr is called
// there is a performance implication to using weak claims.

// This enum lists all valid combinations
typedef enum AJI_CLAIM_TYPE AJI_CLAIM_TYPE;
enum AJI_CLAIM_TYPE
{
    AJI_CLAIM_IR                 = 0x0000, // Exclusive access to this IR value
    AJI_CLAIM_IR_SHARED          = 0x0100, // Shared access to this IR value
    AJI_CLAIM_IR_SHARED_OVERLAY  = 0x0300, // Shared access to this OVERLAY IR value
    AJI_CLAIM_IR_OVERLAID        = 0x0400, // Exclusive access to this OVERLAID IR value
    AJI_CLAIM_IR_SHARED_OVERLAID = 0x0500, // Shared access to this OVERLAID IR value
    AJI_CLAIM_IR_WEAK            = 0x0800, // Allow access to this IR value if unclaimed
                                           // (value ~0 means all unclaimed IR values)

    AJI_CLAIM_OVERLAY            = 0x0001, // Exclusive access to this value in the OVERLAY DR
    AJI_CLAIM_OVERLAY_SHARED     = 0x0101, // Shared access to this value in the OVERLAY DR
    AJI_CLAIM_OVERLAY_WEAK       = 0x0801  // Allow access to this value in OVERLAY DR if unclaimed
                                           // (value ~0 means all unclaimed OVERLAY DR values)
};

typedef struct AJI_CLAIM AJI_CLAIM;
// AJI_CLAIM is deprecated and replaced by AJI_CLAIM2. The deprecated AJI_CLAIM
// can only access non-hierarchical hub and top-level hub of a debug fabric
// with hierarchical hubs.
struct AJI_CLAIM
{
    AJI_CLAIM_TYPE type;
    DWORD value;
};

typedef struct AJI_CLAIM2 AJI_CLAIM2;
// AJI_CLAIM2 is the new claim that supports access to debug fabric with
// hierarchical hubs. AJI_CLAIM2 with AJI_CLAIM2.length == 0 and upper 32 bits
// AJI_CLAIM2.value == 0 is equivalent to a legacy AJI_CLAIM.
struct AJI_CLAIM2
{
    AJI_CLAIM_TYPE type;
    DWORD length;
    QWORD value;
};

// The AJI_RECORD structure is used to publish information to other clients of
// jtag server.

enum AJI_RECORD_FLAG
{
    AJI_RECORD_SAME_PROCESS = 1, // This record was published by this process
    AJI_RECORD_LOCAL_ONLY = 2,   // This record is only suitable for local clients
    AJI_RECORD_CHECK_IN_USE = 0x40000000 // Don't unregister if a client queried recently
};

typedef struct AJI_RECORD AJI_RECORD;
struct AJI_RECORD
{
    const char * type;           // Ends with @ if only one record of this type is allowed per server
    DWORD        number_n;
    DWORD      * number;
    DWORD        flags;
    const char * name;
};

//# TYPEDEFS which require class declarations //////////////////////////////

//# INLINE FUNCTIONS ///////////////////////////////////////////////////////

//# EXTERN VARIABLE DECLARATIONS ///////////////////////////////////////////

// Obsolete, don't use
enum { AJI_SERVER_VERSION = 0 };
/*
//# FUNCTION PROTOTYPES ////////////////////////////////////////////////////
// Obsolete, don't use
inline AJI_ERROR aji_get_server_version       (DWORD              * version)
    { *version = AJI_SERVER_VERSION; return AJI_NO_ERROR; }

AJI_API const char * aji_get_error_info       (void);

AJI_ERROR AJI_API aji_get_potential_hardware  (DWORD              * hardware_count,
                                               AJI_HARDWARE       * hardware_list);

AJI_ERROR AJI_API aji_get_hardware            (DWORD              * hardware_count,
                                               AJI_HARDWARE       * hardware_list,
                                               DWORD                timeout /*= 0x7FFFFFFF/);

AJI_ERROR AJI_API aji_get_hardware2           (DWORD              * hardware_count,
                                               AJI_HARDWARE       * hardware_list,
                                               char             * * server_version_info_list,
                                               DWORD                timeout = 0x7FFFFFFF);

AJI_ERROR AJI_API aji_find_hardware           (DWORD                persistent_id,
                                               AJI_HARDWARE       * hardware,
                                               DWORD                timeout);

AJI_ERROR AJI_API aji_find_hardware           (const char         * hw_name,
                                               AJI_HARDWARE       * hardware,
                                               DWORD                timeout);

AJI_ERROR AJI_API aji_print_hardware_name     (AJI_CHAIN_ID         chain_id,
                                               char               * hw_name,
                                               DWORD                hw_name_len);

AJI_ERROR AJI_API aji_print_hardware_name     (AJI_CHAIN_ID         chain_id,
                                               char               * hw_name,
                                               DWORD                hw_name_len,
                                               bool                 explicit_localhost ,
                                               DWORD              * needed_hw_name_len = NULL);

AJI_ERROR AJI_API aji_add_hardware            (const AJI_HARDWARE * hardware);

AJI_ERROR AJI_API aji_change_hardware_settings(AJI_CHAIN_ID         chain_id,
                                               const AJI_HARDWARE * hardware);

AJI_ERROR AJI_API aji_remove_hardware         (AJI_CHAIN_ID         chain_id);

AJI_ERROR AJI_API aji_add_remote_server       (const char         * server,
                                               const char         * password);

AJI_ERROR AJI_API aji_add_remote_server       (const char         * server,
                                               const char         * password,
                                               bool                 temporary);

AJI_ERROR AJI_API aji_get_servers             (DWORD              * server_count,
                                               const char       * * servers,
                                               bool                 temporary);

AJI_ERROR AJI_API aji_enable_remote_clients   (bool                 enable,
                                               const char         * password);

AJI_ERROR AJI_API aji_get_remote_clients_enabled(bool             * enable);

AJI_ERROR AJI_API aji_add_record              (const AJI_RECORD   * record);

AJI_ERROR AJI_API aji_remove_record           (const AJI_RECORD   * record);

AJI_ERROR AJI_API aji_get_records             (DWORD              * record_n,
                                               AJI_RECORD         * records);

AJI_ERROR AJI_API aji_get_records             (const char         * server,
                                               DWORD              * record_n,
                                               AJI_RECORD         * records);

AJI_ERROR AJI_API aji_get_records             (const char         * server,
                                               DWORD              * record_n,
                                               AJI_RECORD         * records,
                                               DWORD                autostart_type_n,
                                               const char       * * autostart_types);

AJI_ERROR AJI_API aji_replace_local_jtagserver(const char         * replace);

AJI_ERROR AJI_API aji_enable_local_jtagserver(bool                  enable);

AJI_ERROR AJI_API aji_configuration_in_memory(bool                  enable);

AJI_ERROR AJI_API aji_load_quartus_devices    (const char         * filename);

AJI_ERROR AJI_API aji_scan_device_chain       (AJI_CHAIN_ID         chain_id);

AJI_ERROR AJI_API aji_define_device           (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position, 
                                               const AJI_DEVICE   * device);

AJI_ERROR AJI_API aji_define_device           (const AJI_DEVICE   * device);

AJI_ERROR AJI_API aji_undefine_device         (const AJI_DEVICE   * device);

AJI_ERROR AJI_API aji_get_defined_devices     (DWORD              * device_count,
                                               AJI_DEVICE         * device_list);

AJI_ERROR AJI_API aji_get_local_quartus_devices(DWORD             * device_count,
                                               AJI_DEVICE         * device_list);

AJI_ERROR AJI_API aji_read_device_chain       (AJI_CHAIN_ID         chain_id,
                                               DWORD              * device_count,
                                               AJI_DEVICE         * device_list,
                                               bool                 auto_scan = true);

// Maximum size for get/set_parameter block
enum { AJI_PARAMETER_MAX = 3072 };

AJI_ERROR AJI_API aji_set_parameter           (AJI_CHAIN_ID         chain_id,
                                               const char         * name,
                                               DWORD                value);

AJI_ERROR AJI_API aji_set_parameter           (AJI_CHAIN_ID         chain_id,
                                               const char         * name,
                                               const BYTE         * value,
                                               DWORD                valuelen);

AJI_ERROR AJI_API aji_get_parameter           (AJI_CHAIN_ID         chain_id,
                                               const char         * name,
                                               DWORD              * value);

AJI_ERROR AJI_API aji_get_parameter           (AJI_CHAIN_ID         chain_id,
                                               const char         * name,
                                               BYTE               * value,
                                               DWORD              * valuemax,
                                               DWORD                valuetx = 0);

AJI_ERROR AJI_API aji_open_device             (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position, 
                                               AJI_OPEN_ID        * open_id,
                                               const AJI_CLAIM    * claims,
                                               DWORD                claim_n,
                                               const char         * application_name);

AJI_ERROR AJI_API aji_open_device             (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position,
                                               AJI_OPEN_ID        * open_id,
                                               const AJI_CLAIM2   * claims,
                                               DWORD                claim_n,
                                               const char         * application_name);

AJI_ERROR AJI_API aji_close_device            (AJI_OPEN_ID          open_id);

AJI_ERROR AJI_API aji_open_entire_device_chain(AJI_CHAIN_ID         chain_id,
                                               AJI_OPEN_ID        * open_id,
                                               AJI_CHAIN_TYPE       style,
                                               const char         * application_name);

AJI_ERROR AJI_API aji_lock                    (AJI_OPEN_ID          open_id,
                                               DWORD                timeout,
                                               AJI_PACK_STYLE       pack_style);

AJI_ERROR AJI_API aji_unlock                  (AJI_OPEN_ID          open_id);

AJI_ERROR AJI_API aji_unlock_lock             (AJI_OPEN_ID          unlock_id,
                                               AJI_OPEN_ID          lock_id);

AJI_ERROR AJI_API aji_unlock_chain_lock       (AJI_CHAIN_ID         unlock_id,
                                               AJI_OPEN_ID          lock_id,
                                               AJI_PACK_STYLE       pack_style);

AJI_ERROR AJI_API aji_unlock_lock_chain       (AJI_OPEN_ID          unlock_id,
                                               AJI_CHAIN_ID         lock_id);

AJI_ERROR AJI_API aji_flush                   (AJI_OPEN_ID          open_id);

AJI_ERROR AJI_API aji_push                    (AJI_OPEN_ID          open_id,
                                               DWORD                timeout);

AJI_ERROR AJI_API aji_set_checkpoint          (AJI_OPEN_ID          open_id,
                                               DWORD                checkpoint);

AJI_ERROR AJI_API aji_get_checkpoint          (AJI_OPEN_ID          open_id,
                                               DWORD              * checkpoint);

AJI_ERROR AJI_API aji_lock_chain              (AJI_CHAIN_ID         chain_id,
                                               DWORD                timeout);

AJI_ERROR AJI_API aji_unlock_chain            (AJI_CHAIN_ID         chain_id);

AJI_ERROR AJI_API aji_access_ir               (AJI_OPEN_ID          open_id,
                                               DWORD                instruction,
                                               DWORD              * captured_ir,
                                               DWORD                flags = 0);

AJI_ERROR AJI_API aji_access_ir               (AJI_OPEN_ID          open_id,
                                               DWORD                length_ir,
                                               const BYTE         * write_bits,
                                               BYTE               * read_bits,
                                               DWORD                flags = 0);

AJI_ERROR AJI_API aji_access_ir_multiple      (DWORD                num_devices,
                                               const AJI_OPEN_ID  * open_id,
                                               const DWORD        * instructions,
                                               DWORD              * captured_irs);

AJI_ERROR AJI_API aji_access_dr               (AJI_OPEN_ID          open_id,
                                               DWORD                length_dr,
                                               DWORD                flags,
                                               DWORD                write_offset,
                                               DWORD                write_length,
                                               const BYTE         * write_bits,
                                               DWORD                read_offset,
                                               DWORD                read_length,
                                               BYTE               * read_bits);

AJI_ERROR AJI_API aji_access_dr               (AJI_OPEN_ID          open_id,
                                               DWORD                length_dr,
                                               DWORD                flags,
                                               DWORD                write_offset,
                                               DWORD                write_length,
                                               const BYTE         * write_bits,
                                               DWORD                read_offset,
                                               DWORD                read_length,
                                               BYTE               * read_bits,
                                               DWORD                batch);

AJI_ERROR AJI_API aji_access_dr               (AJI_OPEN_ID          open_id,
                                               DWORD                length_dr,
                                               DWORD                flags,
                                               DWORD                write_offset,
                                               DWORD                write_length,
                                               const BYTE         * write_bits,
                                               DWORD                read_offset,
                                               DWORD                read_length,
                                               BYTE               * read_bits,
                                               DWORD                batch,
                                               DWORD              * timestamp);

AJI_ERROR AJI_API aji_access_dr_multiple      (DWORD                num_devices,
                                               DWORD                flags,
                                               const AJI_OPEN_ID  * open_id,
                                               const DWORD        * length_dr,
                                               const DWORD        * write_offset,
                                               const DWORD        * write_length,
                                               const BYTE * const * write_bits,
                                               const DWORD        * read_offset,
                                               const DWORD        * read_length,
                                               BYTE * const       * read_bits);

AJI_ERROR AJI_API aji_access_dr_repeat        (AJI_OPEN_ID          open_id,
                                               DWORD                length_dr,
                                               DWORD                flags,
                                               DWORD                write_offset,
                                               DWORD                write_length,
                                               const BYTE         * write_bits,
                                               DWORD                read_offset,
                                               DWORD                read_length,
                                               BYTE               * read_bits,
                                               const BYTE         * success_mask,
                                               const BYTE         * success_value,
                                               const BYTE         * failure_mask,
                                               const BYTE         * failure_value,
                                               DWORD                timeout);

AJI_ERROR AJI_API aji_access_dr_repeat_multiple(DWORD               num_devices,
                                               DWORD                flags,
                                               const AJI_OPEN_ID  * open_id,
                                               const DWORD          length_dr,
                                               const DWORD          write_offset,
                                               const DWORD          write_length,
                                               const BYTE * const * write_bits,
                                               const DWORD          read_offset,
                                               const DWORD          read_length,
                                               BYTE * const       * read_bits,
                                               const BYTE         * success_mask,
                                               const BYTE         * success_value,
                                               const BYTE         * failure_mask,
                                               const BYTE         * failure_value,
                                               DWORD                timeout);

AJI_ERROR AJI_API aji_any_sequence            (AJI_OPEN_ID          open_id,
                                               DWORD                num_tcks,
                                               const BYTE         * tdi_bits,
                                               const BYTE         * tms_bits,
                                               BYTE               * tdo_bits);

AJI_ERROR AJI_API aji_run_test_idle           (AJI_OPEN_ID          open_id,
                                               DWORD                num_clocks);

AJI_ERROR AJI_API aji_run_test_idle           (AJI_OPEN_ID          open_id,
                                               DWORD                num_clocks,
                                               DWORD                flags);

AJI_ERROR AJI_API aji_test_logic_reset        (AJI_OPEN_ID          open_id);

AJI_ERROR AJI_API aji_delay                   (AJI_OPEN_ID          open_id,
                                               DWORD                timeout_microseconds);

AJI_ERROR AJI_API aji_watch_status            (AJI_OPEN_ID          open_id,
                                               DWORD                status_mask,
                                               DWORD                status_value,
                                               DWORD                poll_interval);

AJI_ERROR AJI_API aji_watch_dr                (AJI_OPEN_ID          open_id,
                                               DWORD                length_dr,
                                               DWORD                flags,
                                               DWORD                write_offset,
                                               DWORD                write_length,
                                               const BYTE         * write_bits,
                                               DWORD                watch_offset,
                                               DWORD                watch_length,
                                               const BYTE         * watch_polarity,
                                               const BYTE         * watch_success_mask,
                                               DWORD                poll_interval);

AJI_ERROR AJI_API aji_get_watch_triggered     (AJI_OPEN_ID          open_id,
                                               bool               * triggered);

AJI_ERROR AJI_API aji_passive_serial_download (AJI_OPEN_ID          open_id,
                                               const BYTE         * data,
                                               DWORD                length,
                                               DWORD              * status);

AJI_ERROR AJI_API aji_direct_pin_control      (AJI_OPEN_ID          open_id,
                                               DWORD                mask,
                                               DWORD                outputs,
                                               DWORD              * inputs);

AJI_ERROR AJI_API aji_active_serial_command   (AJI_OPEN_ID          open_id,
                                               DWORD                write_length,
                                               const BYTE         * write_data,
                                               DWORD                read_length,
                                               BYTE               * read_data);

AJI_ERROR AJI_API aji_active_serial_command_repeat(AJI_OPEN_ID      open_id,
                                               DWORD                write_length,
                                               const BYTE         * write_data,
                                               DWORD                read_length,
                                               BYTE               * read_data,
                                               const BYTE         * success_mask,
                                               const BYTE         * success_value,
                                               const BYTE         * failure_mask,
                                               const BYTE         * failure_value,
                                               DWORD                timeout);

void AJI_API aji_register_output_callback     (void              ( * output_fn)(void * handle, DWORD level, const char * line),
                                               void               * handle);

void AJI_API aji_register_progress_callback   (AJI_OPEN_ID          open_id,
                                               void              ( * progress_fn)(void * handle, DWORD bits),
                                               void               * handle);

AJI_ERROR AJI_API aji_get_nodes               (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position,
                                               DWORD              * idcodes,
                                               DWORD              * idcode_n);

AJI_ERROR AJI_API aji_get_nodes               (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position,
                                               DWORD              * idcodes,
                                               DWORD              * idcode_n,
                                               DWORD              * hub_info);

AJI_ERROR AJI_API aji_get_nodes               (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position,
                                               AJI_HIER_ID        * hier_ids,
                                               DWORD              * hier_id_n,
                                               AJI_HUB_INFO       * hub_infos);

AJI_ERROR AJI_API aji_open_node               (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position,
                                               DWORD                idcode,
                                               AJI_OPEN_ID        * node_id,
                                               const AJI_CLAIM    * claims,
                                               DWORD                claim_n,
                                               const char         * application_name);

AJI_ERROR AJI_API aji_open_node               (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position,
                                               DWORD                node_position,
                                               DWORD                idcode,
                                               AJI_OPEN_ID        * node_id,
                                               const AJI_CLAIM    * claims,
                                               DWORD                claim_n,
                                               const char         * application_name);

AJI_ERROR AJI_API aji_open_node               (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position,
                                               const AJI_HIER_ID  * hier_id,
                                               AJI_OPEN_ID        * node_id,
                                               const AJI_CLAIM2   * claims,
                                               DWORD                claim_n,
                                               const char         * application_name);

AJI_ERROR AJI_API aji_find_node               (AJI_CHAIN_ID         chain_id,
                                               int                  device_index, // -1 for any
                                               int                  instance,     // -1 for any
                                               DWORD                type,
                                               const AJI_CLAIM    * claims,
                                               DWORD                claim_n,
                                               const char         * application_name,
                                               AJI_OPEN_ID        * node_id,
                                               DWORD              * node_n);

AJI_ERROR AJI_API aji_find_node               (AJI_CHAIN_ID         chain_id,
                                               int                  device_index, // -1 for any
                                               int                  instance,     // -1 for any
                                               DWORD                type,
                                               const AJI_CLAIM2   * claims,
                                               DWORD                claim_n,
                                               const char         * application_name,
                                               AJI_OPEN_ID        * node_id,
                                               DWORD              * node_n);

AJI_ERROR AJI_API aji_get_node_info           (AJI_OPEN_ID          node_id,
                                               DWORD              * device_index,
                                               DWORD              * info);

AJI_ERROR AJI_API aji_get_node_info           (AJI_OPEN_ID          node_id,
                                               DWORD              * device_index,
                                               AJI_HIER_ID        * info);

AJI_ERROR AJI_API aji_access_overlay          (AJI_OPEN_ID          node_id,
                                               DWORD                overlay,
                                               DWORD              * captured_overlay);

AJI_ERROR AJI_API aji_open_hub                (AJI_CHAIN_ID         chain_id,
                                               DWORD                tap_position,
                                               DWORD                hub_level,
                                               const AJI_HIER_ID  * hier_id,
                                               AJI_OPEN_ID        * hub_id,
                                               const AJI_CLAIM2   * claims,
                                               DWORD                claim_n,
                                               const char         * application_name);
*/
#endif
