/****************************************************************************
 *   Copyright (c) 2001 by Intel Corporation                                *
 *   author: Whitaker, Alan and Draper, Andrew                              *
 *   SPDX-License-Identifier: MIT                                           *
 *                                                                          *
 *   Permission is hereby granted, free of charge, to any person obtaining  *
 *   a copy of this software and associated documentation files (the        *
 *   "Software"), to deal in the Software without restriction, including    *
 *   without limitation the rights to use, copy, modify, merge, publish,    *
 *   distribute, sublicense, and/or sell copies of the Software, and to     *
 *   permit persons to whom the Software is furnished to do so, subject to  *
 *   the following conditions:                                              *
 *                                                                          *
 *   The above copyright notice and this permission notice (including the   *
 *   next paragraph) shall be included in all copies or substantial         *
 *   portions of the Software.                                              *
 *                                                                          *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     *
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. *
 *   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   *
 *   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   *
 *   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      *
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 *
 ****************************************************************************/


//NOTE: File is MIT-licensed because it is just a modification of aji_sys.h
//    from aji_client.

//# START_MODULE_HEADER/////////////////////////////////////////////////////////
//#
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
#pragma once

#ifndef INC_AJI_SYS_H
#define INC_AJI_SYS_H


//# INCLUDE FILES //////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*Dealing with what used to be Makefile.am line
      MINIJTAGSERV_BUILTINS_CPPFLAGS += -DLITTLE=1 -DBIG=2 -DENDIAN=LITTLE
  but only works for gcc 4.6 or later
*/
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ENDIAN 1
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ENDIAN 2
#elif
#error Cannot handle __ORDER_PDP__ENDIAN__
#else
#error Unknown Byte Order (Endian) or not gcc >4.6
#endif

//# MACRO DEFINITIONS //////////////////////////////////////////////////////

#if PORT==WINDOWS 
#define AJI_API __declspec(dllimport)
#else
#define AJI_API
#endif

#if PORT==WINDOWS 
#include <winsock2.h>
#include <windows.h>
#endif

#include <stdint.h>
typedef uint8_t      BYTE;

/* In MinGW, you will get a conflict warning for
 * DWORD because MinGW's minwindef.h includes
 * its own definition of DWORD.
 * The purpose is to handle realign
 * Cygwin's native datatype to Window's (and
 * our's) expection of DWORD as a 4 byte
 * unsigned int datatype.
 *
 * minwindef.h is #include-d in windows.h and
 * indirectly from winsock2.h.
 *
 * MinGW uses include guard _MINWINDEF_ for
 * minwindef.h so will test for MINGW using the
 * include guard. This means winsock2.h and
 * windows.h must be #include-d first. As
 * per best practice they are #include-d at
 * the beginning of the file, before all
 * other #includes
 */
#ifndef _MINWINDEF_
typedef uint32_t  DWORD; 
#endif // _MINWINDEF_

typedef uint64_t    QWORD;

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
    AJI_TOO_MANY_HIERARCHIES = 260,

// OpenOCD
    AJI_TOO_MANY_CLAIMS = 512,
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
    AJI_DR_NO_OPTIMIZATION = 0, // No optimization
    AJI_DR_UNUSED_0        = 1,  // Allow zeros to be written to unspecified bits
    AJI_DR_UNUSED_0_OMIT   = 3,  // Allow zeros at the TDI end, allow any value at TDO end
    AJI_DR_UNUSED_X        = 15, // Allow any value to be written to unspecified bits
    AJI_DR_NO_SHORT        = 16, // Must clock all bits through (disable optimisations)
    AJI_DR_END_PAUSE_DR    = 32, // End the dr scan in the PAUSE_DR state
    AJI_DR_START_PAUSE_DR  = 64, // Allow the dr scan to start in the PAUSE_DR state
    AJI_DR_NO_RESTORE      = 128 // Do not reload previous instruction and overlay after relocking
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

#ifdef __cplusplus

typedef class AJI_CHAIN * AJI_CHAIN_ID;
typedef class AJI_OPEN  * AJI_OPEN_ID;

#else
typedef struct AJI_CHAIN * AJI_CHAIN_ID;

//AJI_OPEN has to be void because AJI_OPEN in C++ contains virtual functions
typedef void AJI_OPEN;          
typedef AJI_OPEN  * AJI_OPEN_ID;

#endif //end #ifdef __cplusplus

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


#endif //INC_AJI_SYS_H
