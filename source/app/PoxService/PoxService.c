#include "sdk_common.h"
#include "PoxService.h"
#include "ble_srv_common.h"

#define BLE_UUID_POX_TX_CHARACTERISTIC 0x0002
#define BLE_UUID_POX_RX_CHARACTERISTIC 0x0003

#define BLE_POX_MAX_RX_CHAR_LEN        BLE_POX_MAX_DATA_LEN
#define BLE_POX_MAX_TX_CHAR_LEN        BLE_POX_MAX_DATA_LEN

#define POX_BASE_UUID                  {{0xAA, 0xEE, 0xAA, 0xEE, 0xAA, 0xEE, 0xAA, 0xEE, 0xAA, 0xEE, 0xAA, 0xEE, 0xAA, 0xEE, 0xAA, 0xEE}} /**< Used vendor specific UUID. */


/**@brief Function for adding RX characteristic.
 *
 * @param[in] p_nus       Nordic UART Service structure.
 * @param[in] p_nus_init  Information needed to initialize the service.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t rx_char_add(PoxServiceStruct *pPox)
{
    /**@snippet [Adding proprietary characteristic to S110 SoftDevice] */
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.notify = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md;
    char_md.p_sccd_md         = NULL;

    ble_uuid.type = pPox->uuid_type;
    ble_uuid.uuid = BLE_UUID_POX_RX_CHARACTERISTIC;

    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    attr_md.vloc    = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth = 0;
    attr_md.wr_auth = 0;
    attr_md.vlen    = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = sizeof(uint8_t);
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = BLE_POX_MAX_RX_CHAR_LEN;

    return sd_ble_gatts_characteristic_add(pPox->service_handle, &char_md, &attr_char_value, &pPox->rx_handles);
    /**@snippet [Adding proprietary characteristic to S110 SoftDevice] */
}


/**@brief Function for adding TX characteristic.
 *
 * @param[in] p_nus       Nordic UART Service structure.
 * @param[in] p_nus_init  Information needed to initialize the service.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t tx_char_add(PoxServiceStruct *pPox)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.write         = 1;
    char_md.char_props.write_wo_resp = 1;
    char_md.p_char_user_desc         = NULL;
    char_md.p_char_pf                = NULL;
    char_md.p_user_desc_md           = NULL;
    char_md.p_cccd_md                = NULL;
    char_md.p_sccd_md                = NULL;

    ble_uuid.type = pPox->uuid_type;
    ble_uuid.uuid = BLE_UUID_POX_TX_CHARACTERISTIC;

    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    attr_md.vloc    = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth = 0;
    attr_md.wr_auth = 0;
    attr_md.vlen    = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = 1;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = BLE_POX_MAX_TX_CHAR_LEN;

    return sd_ble_gatts_characteristic_add(pPox->service_handle, &char_md, &attr_char_value, &pPox->tx_handles);
}


void poxServiceOnBleEvt(PoxServiceStruct *pPox, ble_evt_t *p_ble_evt)
{
    if ((pPox == NULL) || (p_ble_evt == NULL)) {
        return;
    }
    ble_gatts_evt_write_t *p_evt_write;
    switch (p_ble_evt->header.evt_id) {
    case BLE_GAP_EVT_CONNECTED:
        pPox->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
        break;

    case BLE_GAP_EVT_DISCONNECTED:
        pPox->conn_handle = BLE_CONN_HANDLE_INVALID;
        break;

    case BLE_GATTS_EVT_WRITE:
        p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

        if ((p_evt_write->handle == pPox->rx_handles.cccd_handle) && (p_evt_write->len == 2)) {
            if (ble_srv_is_notification_enabled(p_evt_write->data)) {
                pPox->is_notification_enabled = true;
            } else {
                pPox->is_notification_enabled = false;
            }
        } else if ((p_evt_write->handle == pPox->tx_handles.value_handle) && (pPox->data_handler != NULL)) {
            pPox->data_handler(p_evt_write->data, p_evt_write->len);
        } else {
            // Do Nothing. This event is not relevant for this service.
        }

        break;

    default:
        // No implementation needed.
        break;
    }
}


uint32_t poxServiceInit(PoxServiceStruct *pPox, PoxDataHandler poxDataHandler)
{
    uint32_t      err_code;
    ble_uuid_t    ble_uuid;
    ble_uuid128_t nus_base_uuid = POX_BASE_UUID;

    VERIFY_PARAM_NOT_NULL(pPox);
    VERIFY_PARAM_NOT_NULL(poxDataHandler);

    // Initialize the service structure.
    pPox->conn_handle             = BLE_CONN_HANDLE_INVALID;
    pPox->data_handler            = poxDataHandler;
    pPox->is_notification_enabled = false;

    /**@snippet [Adding proprietary Service to S110 SoftDevice] */
    // Add a custom base UUID.
    err_code = sd_ble_uuid_vs_add(&nus_base_uuid, &pPox->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = pPox->uuid_type;
    ble_uuid.uuid = BLE_UUID_POX_SERVICE;

    // Add the service.
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &pPox->service_handle);
    /**@snippet [Adding proprietary Service to S110 SoftDevice] */
    VERIFY_SUCCESS(err_code);

    // Add the RX Characteristic.
    err_code = rx_char_add(pPox);
    VERIFY_SUCCESS(err_code);

    // Add the TX Characteristic.
    err_code = tx_char_add(pPox);
    VERIFY_SUCCESS(err_code);

    return NRF_SUCCESS;
}


uint32_t poxServiceSend(PoxServiceStruct *pPox, const uint8_t data[], uint16_t *pLength)
{
    ble_gatts_hvx_params_t hvx_params;

    VERIFY_PARAM_NOT_NULL(pPox);
    VERIFY_PARAM_NOT_NULL(pLength);
    VERIFY_PARAM_NOT_NULL(data);

    if ((pPox->conn_handle == BLE_CONN_HANDLE_INVALID) || (!pPox->is_notification_enabled)) {
        return NRF_ERROR_INVALID_STATE;
    }

    if (*pLength > BLE_POX_MAX_DATA_LEN) {
        return NRF_ERROR_INVALID_PARAM;
    }

    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = pPox->rx_handles.value_handle;
    hvx_params.p_data = (uint8_t *) data;
    hvx_params.p_len  = pLength;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;

    return sd_ble_gatts_hvx(pPox->conn_handle, &hvx_params);
}
