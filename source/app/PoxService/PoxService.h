#ifndef __POX_SERVICE_H__
#define __POX_SERVICE_H__

#include "ble.h"
#include "ble_srv_common.h"
#include <stdint.h>
#include <stdbool.h>

#define BLE_UUID_POX_SERVICE 0x0001
#define BLE_POX_MAX_DATA_LEN (GATT_MTU_SIZE_DEFAULT - 3)

typedef void (*PoxDataHandler) (uint8_t data[], uint32_t length);

typedef struct {
    uint8_t                  uuid_type;               /**< UUID type for Nordic UART Service Base UUID. */
    uint16_t                 service_handle;          /**< Handle of Nordic UART Service (as provided by the SoftDevice). */
    ble_gatts_char_handles_t tx_handles;              /**< Handles related to the TX characteristic (as provided by the SoftDevice). */
    ble_gatts_char_handles_t rx_handles;              /**< Handles related to the RX characteristic (as provided by the SoftDevice). */
    uint16_t                 conn_handle;             /**< Handle of the current connection (as provided by the SoftDevice). BLE_CONN_HANDLE_INVALID if not in a connection. */
    bool                     is_notification_enabled; /**< Variable to indicate if the peer has enabled notification of the RX characteristic.*/
    PoxDataHandler           data_handler;            /**< Event handler to be called for handling received data. */
} PoxServiceStruct;

uint32_t poxServiceInit(PoxServiceStruct *pPox, PoxDataHandler poxDataHandler);
void poxServiceOnBleEvt(PoxServiceStruct *pPox, ble_evt_t *p_ble_evt);
uint32_t poxServiceSend(PoxServiceStruct *pPox, const uint8_t data[], uint16_t *pLength);


#endif
