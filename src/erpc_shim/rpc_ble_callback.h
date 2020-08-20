/*
 * Generated by erpcgen 1.7.4 on Thu Aug 20 10:51:20 2020.
 *
 * AUTOGENERATED - DO NOT EDIT
 */


#if !defined(_rpc_ble_callback_h_)
#define _rpc_ble_callback_h_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "erpc_version.h"

#if 10704 != ERPC_VERSION_NUMBER
#error "The generated shim code version is different to the rest of eRPC code."
#endif

#if !defined(ERPC_TYPE_DEFINITIONS)
#define ERPC_TYPE_DEFINITIONS

// Enumerators data types declarations
typedef enum RPC_T_APP_RESULT
{
    RPC_APP_RESULT_SUCCESS = 0,
    RPC_APP_RESULT_PENDING = 3329,
    RPC_APP_RESULT_ACCEPT = 3331,
    RPC_APP_RESULT_REJECT = 3332,
    RPC_APP_RESULT_NOT_RELEASE = 3333,
    RPC_APP_RESULT_PREP_QUEUE_FULL = 1033,
    RPC_APP_RESULT_INVALID_OFFSET = 1031,
    RPC_APP_RESULT_INVALID_VALUE_SIZE = 1037,
    RPC_APP_RESULT_INVALID_PDU = 1028,
    RPC_APP_RESULT_ATTR_NOT_FOUND = 1034,
    RPC_APP_RESULT_ATTR_NOT_LONG = 1035,
    RPC_APP_RESULT_INSUFFICIENT_RESOURCES = 1041,
    RPC_APP_RESULT_APP_ERR = 1152,
    RPC_APP_RESULT_CCCD_IMPROPERLY_CONFIGURED = 1277,
    RPC_APP_RESULT_PROC_ALREADY_IN_PROGRESS = 1278
} RPC_T_APP_RESULT;

// Aliases data types declarations
typedef struct binary_t binary_t;

// Structures/unions data types declarations
struct binary_t
{
    uint8_t * data;
    uint32_t dataLength;
};


#endif // ERPC_TYPE_DEFINITIONS

/*! @brief rpc_ble_callback identifiers */
enum _rpc_ble_callback_ids
{
    krpc_ble_callback_service_id = 1,
    krpc_ble_callback_rpc_ble_handle_gap_msg_id = 1,
    krpc_ble_callback_rpc_ble_gap_callback_id = 2,
};

#if defined(__cplusplus)
extern "C" {
#endif

//! @name rpc_ble_callback
//@{
void rpc_ble_handle_gap_msg(const binary_t * gap_msg);

RPC_T_APP_RESULT rpc_ble_gap_callback(uint8_t cb_type, const binary_t * cb_data);
//@}

#if defined(__cplusplus)
}
#endif

#endif // _rpc_ble_callback_h_
