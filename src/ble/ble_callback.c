
/**
*****************************************************************************************
*     Copyright (C) 2020  Seeed Technology Co.,Ltd. All rights reserved.
*****************************************************************************************
   * @file      ble_callback.c
   * @brief     
   * @author    Hongtai.Liu
   * @date      2020-08-10
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2020 Seeed Technology Co.,Ltd</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif

#include "FreeRTOS.h"
#include "mpu_wrappers.h"
#include "wifi_conf.h"
#include "wifi_constants.h"
#include "trace_app.h"
#include "bte.h"
#include "bt_types.h"
#include "gap.h"
#include "gap_adv.h"
#include "gap_bond_le.h"
#include "gap_callback_le.h"
#include "gap_config.h"
#include "gap_conn_le.h"
#include "gap_le.h"
#include "gap_le_types.h"
#include "gap_msg.h"
#include "gap_privacy.h"
#include "gap_scan.h"
#include "gap_storage_le.h"
#include "os_task.h"
#include "os_msg.h"
#include "app_msg.h"
#include "rtk_coex.h"
#include "profile_server.h"
#include "profile_client.h"
#include "ble_callback.h"
#include "ble_utils.h"
#include "ble_callback.h"
#include "ble_utils.h"
#include "erpc_port.h"
#ifdef __cplusplus
}
#endif

/**
 * @brief    Handle msg GAP_MSG_LE_DEV_STATE_CHANGE
 * @note     All the gap device state events are pre-handled in this function.
 *           Then the event handling function shall be called according to the new_state
 * @param[in] new_state  New gap device state
 * @param[in] cause GAP device state change cause
 * @return   void
 */
void ble_dev_state_evt_handler(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    log_d("ble_dev_state_evt_handler: init state %d, adv state %d, cause 0x%x", new_state.gap_init_state, new_state.gap_adv_state, cause);
    if (ble_gap_dev_state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {
            log_d("GAP stack ready");
            // BLE stack is ready
            uint8_t bt_addr[6];
            gap_get_param(GAP_PARAM_BD_ADDR, bt_addr);
            log_d("[BLE Device] Local BT addr: %2x:%2x:%2x:%2x:%2x:%2x", bt_addr[5], bt_addr[4], bt_addr[3], bt_addr[2], bt_addr[1], bt_addr[0]);
        }
    }

    // Assign different tasks according to different roles
    // As a Client
    if (ble_dev_role == BLE_DEVICE_ROLE_CLIENT)
    {
        if (ble_gap_dev_state.gap_scan_state != new_state.gap_scan_state)
        {
            if (new_state.gap_scan_state == GAP_SCAN_STATE_IDLE)
            {
                log_d("[BLE Device] GAP scan stop");
            }
            else if (new_state.gap_scan_state == GAP_SCAN_STATE_SCANNING)
            {
                log_d("[BLE Device] GAP scan start");
            }
        }
    }
    // As a Server
    else
    {
        if (ble_gap_dev_state.gap_adv_state != new_state.gap_adv_state)
        {
            if (new_state.gap_adv_state == GAP_ADV_STATE_IDLE)
            {
                if (new_state.gap_adv_sub_state == GAP_ADV_TO_IDLE_CAUSE_CONN)
                {
                    log_d("[BLE Device] GAP adv stopped: because connection created");
                }
                else
                {
                    log_d("[BLE Device] GAP adv stopped");
                }
            }
            else if (new_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING)
            {
                log_d("[BLE Device] GAP adv start");
            }
        }
    }

    ble_gap_dev_state = new_state;
}

/**
 * @brief    Handle msg GAP_MSG_LE_CONN_STATE_CHANGE
 * @note     All the gap conn state events are pre-handled in this function.
 *           Then the event handling function shall be called according to the new_state
 * @param[in] conn_id Connection ID
 * @param[in] new_state  New gap connection state
 * @param[in] disc_cause Use this cause when new_state is GAP_CONN_STATE_DISCONNECTED
 * @return   void
 */
void ble_conn_state_evt_handler(uint8_t conn_id, T_GAP_CONN_STATE new_state, uint16_t disc_cause)
{

    log_d("ble_conn_state_evt_handler: conn_id %d old_state %d new_state %d, disc_cause 0x%x", conn_id, ble_gap_conn_state, new_state, disc_cause);

    if (ble_dev_role == BLE_DEVICE_ROLE_CLIENT)
    {

        if (conn_id >= BLE_CLIENT_MAX_LINKS)
        {
            return;
        }

        ble_clinet_link_table[conn_id].conn_state = new_state;

        switch (new_state)
        {
        case GAP_CONN_STATE_DISCONNECTED:
        {
            if ((disc_cause != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) && (disc_cause != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
            {
                log_d("connection lost, conn_id %d, cause 0x%x", conn_id, disc_cause);
            }
            log_d("[BLE Device] Disconnected conn_id %d", conn_id);
            memset(&ble_clinet_link_table[conn_id], 0, sizeof(T_APP_LINK));
            break;
        }
        case GAP_CONN_STATE_CONNECTED:
        {
            le_get_conn_addr(conn_id, ble_clinet_link_table[conn_id].bd_addr, (uint8_t *)&ble_clinet_link_table[conn_id].bd_type);
            log_d("[BLE Device] Connected conn_id %d", conn_id);
            {
                uint8_t tx_phy;
                uint8_t rx_phy;
                le_get_conn_param(GAP_PARAM_CONN_TX_PHY_TYPE, &tx_phy, conn_id);
                le_get_conn_param(GAP_PARAM_CONN_RX_PHY_TYPE, &rx_phy, conn_id);
                log_d("GAP_CONN_STATE_CONNECTED: tx_phy %d, rx_phy %d", tx_phy, rx_phy);
            }
            break;
        }
        default:
            break;
        }
    }
    else
    {
        switch (new_state)
        {
        case GAP_CONN_STATE_DISCONNECTED:
        {
            if ((disc_cause != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) && (disc_cause != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
            {

                log_d("connection lost cause 0x%x", disc_cause);
            }
            log_d("[BLE Device] BT Disconnected, start ADV\n\r");
            le_adv_start();
            break;
        }
        case GAP_CONN_STATE_CONNECTED:
        {
            uint16_t conn_interval;
            uint16_t conn_latency;
            uint16_t conn_supervision_timeout;
            uint8_t remote_bd[6];
            T_GAP_REMOTE_ADDR_TYPE remote_bd_type;

            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
            le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_latency, conn_id);
            le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);
            le_get_conn_addr(conn_id, remote_bd, (uint8_t *)&remote_bd_type);

            log_d("GAP_CONN_STATE_CONNECTED:remote_bd %x:%x:%x:%x:%x:%x, remote_addr_type %d, conn_interval 0x%x, conn_latency 0x%x, conn_supervision_timeout 0x%x", remote_bd[0], remote_bd[1], remote_bd[2], remote_bd[3], remote_bd[4], remote_bd[5], remote_bd_type, conn_interval, conn_latency, conn_supervision_timeout);
            log_d("[BLE Device] BT Connected\n\r");
            break;
        }
        default:
            break;
        }
    }
    ble_gap_conn_state = new_state;
}

/**
 * @brief    Handle msg GAP_MSG_LE_CONN_PARAM_UPDATE
 * @note     All the connection parameter update change  events are pre-handled in this function.
 * @param[in] conn_id Connection ID
 * @param[in] status  New update state
 * @param[in] cause Use this cause when status is GAP_CONN_PARAM_UPDATE_STATUS_FAIL
 * @return   void
 */
void ble_param_update_evt_handler(uint8_t conn_id, uint8_t status, uint16_t cause)
{
    switch (status)
    {
    case GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS:
    {
        uint16_t conn_interval;
        uint16_t conn_slave_latency;
        uint16_t conn_supervision_timeout;

        le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
        le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
        le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);
        log_d("connParamUpdateEvtHandlerDefault update success:conn_interval 0x%x, conn_slave_latency 0x%x, conn_supervision_timeout 0x%x", conn_interval, conn_slave_latency, conn_supervision_timeout);
        break;
    }
    case GAP_CONN_PARAM_UPDATE_STATUS_FAIL:
    {
        log_d("connParamUpdateEvtHandlerDefault update failed: cause 0x%x", cause);
        break;
    }
    case GAP_CONN_PARAM_UPDATE_STATUS_PENDING:
    {
        log_d("connParamUpdateEvtHandlerDefault update pending: conn_id %d", conn_id);
        break;
    }
    default:
        break;
    }
}

/**
 * @brief    Handle msg GAP_MSG_LE_AUTHEN_STATE_CHANGE
 * @note     All the gap authentication state events are pre-handled in this function.
 *           Then the event handling function shall be called according to the new_state
 * @param[in] conn_id Connection ID
 * @param[in] new_state  New authentication state
 * @param[in] cause Use this cause when new_state is GAP_AUTHEN_STATE_COMPLETE
 * @return   void
 */
void ble_authen_state_evt_handler(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{
    log_d("app_handle_authen_state_evt:conn_id %d, cause 0x%x", conn_id, cause);

    switch (new_state)
    {
    case GAP_AUTHEN_STATE_STARTED:
    {
        log_d("app_handle_authen_state_evt: GAP_AUTHEN_STATE_STARTED");
    }
    break;

    case GAP_AUTHEN_STATE_COMPLETE:
    {
        if (cause == GAP_SUCCESS)
        {
            log_d("Pair success");
            log_d("app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair success");
        }
        else
        {
            log_d("Pair failed: cause 0x%x", cause);
            log_d("app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair failed");
        }
    }
    break;

    default:
    {
        log_d("app_handle_authen_state_evt: unknown newstate %d", new_state);
    }
    break;
    }
}

/**
 * @brief    Handle msg GAP_MSG_LE_CONN_MTU_INFO
 * @note     This msg is used to inform APP that exchange mtu procedure is completed.
 * @param[in] conn_id Connection ID
 * @param[in] mtu_size  New mtu size
 * @return   void
 */
void ble_mtu_info_evt_handler(uint8_t conn_id, uint16_t mtu_size)
{
    log_d("app_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d", conn_id, mtu_size);
}

/**
  * @brief Callback for gap le to notify app
  * @param[in] cb_type callback msy type @ref GAP_LE_MSG_Types.
  * @param[in] p_cb_data point to callback data @ref T_LE_CB_DATA.
  * @retval result @ref T_APP_RESULT
  */
T_APP_RESULT ble_gap_callback(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)p_cb_data;
    binary_t *cb_data = NULL;
    cb_data = (binary_t *) erpc_malloc(sizeof(binary_t));

    log_d("ble_gap_callback: cb_type %d", cb_type);
    switch (cb_type)
    {
    case GAP_MSG_LE_DATA_LEN_CHANGE_INFO:
    {
        log_d("GAP_MSG_LE_DATA_LEN_CHANGE_INFO: conn_id %d, tx octets 0x%x, max_tx_time 0x%x", p_data->p_le_data_len_change_info->conn_id, p_data->p_le_data_len_change_info->max_tx_octets, p_data->p_le_data_len_change_info->max_tx_time);
        break;
    }
    case GAP_MSG_LE_MODIFY_WHITE_LIST:
    {
        log_d("GAP_MSG_LE_MODIFY_WHITE_LIST: operation %d, cause 0x%x", p_data->p_le_modify_white_list_rsp->operation, p_data->p_le_modify_white_list_rsp->cause);
        break;
    }
    case GAP_MSG_LE_CONN_UPDATE_IND:
    {
        log_d("GAP_MSG_LE_CONN_UPDATE_IND: conn_id %d, conn_interval_max 0x%x, conn_interval_min 0x%x, conn_latency 0x%x,supervision_timeout 0x%x", p_data->p_le_conn_update_ind->conn_id, p_data->p_le_conn_update_ind->conn_interval_max, p_data->p_le_conn_update_ind->conn_interval_min, p_data->p_le_conn_update_ind->conn_latency, p_data->p_le_conn_update_ind->supervision_timeout);
        /* if reject the proposed connection parameter from peer device, use APP_RESULT_REJECT. */
        result = APP_RESULT_ACCEPT;
        break;
    }
    case GAP_MSG_LE_PHY_UPDATE_INFO:
    {
        log_d("GAP_MSG_LE_PHY_UPDATE_INFO:conn_id %d, cause 0x%x, rx_phy %d, tx_phy %d", p_data->p_le_phy_update_info->conn_id, p_data->p_le_phy_update_info->cause, p_data->p_le_phy_update_info->rx_phy, p_data->p_le_phy_update_info->tx_phy);
        break;
    }
    case GAP_MSG_LE_REMOTE_FEATS_INFO:
    {
        uint8_t remote_feats[8];
        log_d("GAP_MSG_LE_REMOTE_FEATS_INFO: conn id %d, cause 0x%x, remote_feats %b", p_data->p_le_remote_feats_info->conn_id, p_data->p_le_remote_feats_info->cause, TRACE_BINARY(8, p_data->p_le_remote_feats_info->remote_feats));
        if (p_data->p_le_remote_feats_info->cause == GAP_SUCCESS)
        {
            memcpy(remote_feats, p_data->p_le_remote_feats_info->remote_feats, 8);
            if (remote_feats[LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX1] & LE_SUPPORT_FEATURES_LE_2M_MASK_BIT)
            {
                log_d("GAP_MSG_LE_REMOTE_FEATS_INFO: support 2M");
            }
            if (remote_feats[LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX1] & LE_SUPPORT_FEATURES_LE_CODED_PHY_MASK_BIT)
            {
                log_d("GAP_MSG_LE_REMOTE_FEATS_INFO: support CODED");
            }
        }
        break;
    }
    case GAP_MSG_LE_SCAN_INFO:
    {
        FORMATION_BINARY(cb_data, p_data->p_le_scan_info, T_LE_SCAN_INFO);
        log_d("GAP_MSG_LE_SCAN_INFO:adv_type 0x%x, bd_addr %02x:%02x:%02x:%02x:%02x:%02x, remote_addr_type %d, rssi %d, data_len %d",
              p_data->p_le_scan_info->adv_type,
              (p_data->p_le_scan_info->bd_addr)[5],
              (p_data->p_le_scan_info->bd_addr)[4],
              (p_data->p_le_scan_info->bd_addr)[3],
              (p_data->p_le_scan_info->bd_addr)[2],
              (p_data->p_le_scan_info->bd_addr)[1],
              (p_data->p_le_scan_info->bd_addr)[0],
              p_data->p_le_scan_info->remote_addr_type,
              p_data->p_le_scan_info->rssi,
              p_data->p_le_scan_info->data_len);
        break;
    }
    default:
        log_d("gapCallbackDefault: unhandled cb_type 0x%x", cb_type);
        break;
    }

    log_d("cb_data.dataLength: %d",   cb_data->dataLength);
    rpc_ble_gap_callback(cb_type, cb_data);

    return result;
}

/**
 * @brief    All the BT GAP MSG are pre-handled in this function.
 * @note     Then the event handling function shall be called according to the
 *           subtype of T_IO_MSG
 * @param[in] p_gap_msg Pointer to GAP msg
 * @return   void
 */
void ble_handle_gap_msg(T_IO_MSG *p_gap_msg)
{
    T_LE_GAP_MSG gap_msg;
    uint8_t conn_id;
    memcpy(&gap_msg, &p_gap_msg->u.param, sizeof(p_gap_msg->u.param));

    log_d("ble_central_app_handle_gap_msg: subtype %d", p_gap_msg->subtype);

    switch (p_gap_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
    {
        log_d("GAP_MSG_LE_DEV_STATE_CHANGE");
        ble_dev_state_evt_handler(gap_msg.msg_data.gap_dev_state_change.new_state,
                                  gap_msg.msg_data.gap_dev_state_change.cause);
    }
    break;
    case GAP_MSG_LE_CONN_STATE_CHANGE:
    {
        log_d("GAP_MSG_LE_CONN_STATE_CHANGE");
        ble_conn_state_evt_handler(gap_msg.msg_data.gap_conn_state_change.conn_id,
                                   (T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state,
                                   gap_msg.msg_data.gap_conn_state_change.disc_cause);
    }
    break;
    case GAP_MSG_LE_CONN_PARAM_UPDATE:
    {
        log_d("GAP_MSG_LE_CONN_PARAM_UPDATE");
        ble_param_update_evt_handler(gap_msg.msg_data.gap_conn_param_update.conn_id,
                                     gap_msg.msg_data.gap_conn_param_update.status,
                                     gap_msg.msg_data.gap_conn_param_update.cause);
    }
    break;
    case GAP_MSG_LE_CONN_MTU_INFO:
    {
        log_d("GAP_MSG_LE_CONN_MTU_INFO");
        ble_mtu_info_evt_handler(gap_msg.msg_data.gap_conn_mtu_info.conn_id,
                                 gap_msg.msg_data.gap_conn_mtu_info.mtu_size);
    }
    break;
    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
    {
        log_d("GAP_MSG_LE_AUTHEN_STATE_CHANGE");
        ble_authen_state_evt_handler(gap_msg.msg_data.gap_authen_state.conn_id,
                                     gap_msg.msg_data.gap_authen_state.new_state,
                                     gap_msg.msg_data.gap_authen_state.status);
    }
    break;
    case GAP_MSG_LE_BOND_PASSKEY_DISPLAY:
    {
        log_d("GAP_MSG_LE_BOND_PASSKEY_DISPLAY");
        conn_id = gap_msg.msg_data.gap_bond_just_work_conf.conn_id;
        le_bond_just_work_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
    }
    break;
    case GAP_MSG_LE_BOND_PASSKEY_INPUT:
    {
        log_d("GAP_MSG_LE_BOND_PASSKEY_INPUT");
        uint32_t display_value = 0;
        conn_id = gap_msg.msg_data.gap_bond_passkey_display.conn_id;
        le_bond_get_display_key(conn_id, &display_value);
        log_d("GAP_MSG_LE_BOND_PASSKEY_DISPLAY:passkey %d", display_value);
        le_bond_passkey_display_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        log_d("GAP_MSG_LE_BOND_PASSKEY_DISPLAY:passkey %d", display_value);
    }
    break;
    case GAP_MSG_LE_BOND_OOB_INPUT:
    {
        log_d("GAP_MSG_LE_BOND_OOB_INPUT");
        uint8_t oob_data[GAP_OOB_LEN] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        conn_id = gap_msg.msg_data.gap_bond_oob_input.conn_id;
        log_d("GAP_MSG_LE_BOND_OOB_INPUT\r\n");
        le_bond_set_param(GAP_PARAM_BOND_OOB_DATA, GAP_OOB_LEN, oob_data);
        le_bond_oob_input_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
    }
    break;
    case GAP_MSG_LE_BOND_USER_CONFIRMATION:
    {
        log_d("GAP_MSG_LE_BOND_USER_CONFIRMATION");
        uint32_t display_value = 0;
        conn_id = gap_msg.msg_data.gap_bond_user_conf.conn_id;
        le_bond_get_display_key(conn_id, &display_value);
        log_d("GAP_MSG_LE_BOND_USER_CONFIRMATION: passkey %ld\r\n", display_value);
        le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
    }
    break;
    case GAP_MSG_LE_BOND_JUST_WORK:
    {
        log_d("GAP_MSG_LE_BOND_JUST_WORK");
        conn_id = gap_msg.msg_data.gap_bond_just_work_conf.conn_id;
        le_bond_just_work_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        log_d("GAP_MSG_LE_BOND_JUST_WORK\r\n");
    }
    break;
    default:
        log_d("gapMsgHandlerDefault: unknown subtype %d", p_gap_msg->subtype);
        break;
    }

    // binary_t callback_io_msg;
    // callback_io_msg.dataLength = sizeof(T_IO_MSG);
    // log_d("callback_io_msg.dataLength: %d",   callback_io_msg.dataLength);
    // callback_io_msg.data = p_gap_msg;
    // rpc_ble_handle_gap_msg(&callback_io_msg);
}

/**
 * @brief    All the application messages are pre-handled in this function
 * @note     All the IO MSGs are sent to this function, then the event handling
 *           function shall be called according to the MSG type.
 * @param[in] io_msg  IO message data
 * @return   void
 */
void ble_handle_io_msg(T_IO_MSG io_msg)
{
    uint16_t msg_type = io_msg.type;

    switch (msg_type)
    {
    case IO_MSG_TYPE_BT_STATUS:
    {
        ble_handle_gap_msg(&io_msg);
    }
    break;
    default:
        break;
    }
}
