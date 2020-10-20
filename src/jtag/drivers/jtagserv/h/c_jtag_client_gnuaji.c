#include "c_jtag_client_gnuaji.h"


/**
 * Decode AJI into its string equivalent
 * \param out On input, a reserved space of size @see outsize to store data
 *        Should be at least 40 char
 * \param outsize On input, size of out, on output, the actual size
 */
const char* c_aji_error_decode(AJI_ERROR code) {
    switch (code) {
    case  0: return "AJI_NO_ERROR";
    case  1: return "AJI_FAILURE";
    case  2: return "AJI_TIMEOUT";

    case 32: return "AJI_UNKNOWN_HARDWARE";
    case 33: return "AJI_INVALID_CHAIN_ID";
    case 34: return "AJI_LOCKED";
    case 35: return "AJI_NOT_LOCKED";

    case 36: return "AJI_CHAIN_IN_USE";
    case 37: return "AJI_NO_DEVICES";
    case 38: return "AJI_CHAIN_NOT_CONFIGURED";
    case 39: return "AJI_BAD_TAP_POSITION";
    case 40: return "AJI_DEVICE_DOESNT_MATCH";
    case 41: return "AJI_IR_LENGTH_ERROR";
    case 42: return "AJI_DEVICE_NOT_CONFIGURED";
    case 43: return "AJI_CHAINS_CLAIMED";

    case 44: return "AJI_INVALID_OPEN_ID";
    case 45: return "AJI_INVALID_PARAMETER";
    case 46: return "AJI_BAD_TAP_STATE";
    case 47: return "AJI_TOO_MANY_DEVICES";
    case 48: return "AJI_IR_MULTIPLE";
    case 49: return "AJI_BAD_SEQUENCE";
    case 50: return "AJI_INSTRUCTION_CLAIMED";
    case  51: return "AJI_MODE_NOT_AVAILABLE";
    case 52: return "AJI_INVALID_DUMMY_BITS";

    case 80: return "AJI_FILE_ERROR";
    case 81: return "AJI_NET_DOWN";
    case 82: return "AJI_SERVER_ERROR";
    case 83: return "AJI_NO_MEMORY";
    case 84: return "AJI_BAD_PORT";
    case 85: return "AJI_PORT_IN_USE";
    case 86: return "AJI_BAD_HARDWARE";
    case 87: return "AJI_BAD_JTAG_CHAIN";
    case 88: return "AJI_SERVER_ACTIVE";
    case 89: return "AJI_NOT_PERMITTED";
    case 90: return "AJI_HARDWARE_DISABLED";

    case 125: return "AJI_HIERARCHICAL_HUB_NOT_SUPPORTED";
    case 126: return "AJI_UNIMPLEMENTED";
    case 127: return "AJI_INTERNAL_ERROR";

        // These errors are generated on the client side only
    case 256: return "AJI_NO_HUBS";
    case 257: return "AJI_TOO_MANY_HUBS";
    case 258: return "AJI_NO_MATCHING_NODES";
    case 259: return "AJI_TOO_MANY_MATCHING_NODES";
    case 260: return "AJI_TOO_MANY_HIERARCHIES";
    } //end switch
    return "UNKNOWN_ERROR_CODE";
}