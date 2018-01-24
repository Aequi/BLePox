/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 *
 * @defgroup ble_sdk_app_hids_mouse_main main.c
 * @{
 * @ingroup ble_sdk_app_hids_mouse
 * @brief HID Mouse Sample Application main file.
 *
 * This file contains is the source code for a sample application using the HID, Battery and Device
 * Information Service for implementing a simple mouse functionality. This application uses the
 * @ref app_scheduler.
 *
 * Also it would accept pairing requests from any peer device. This implementation of the
 * application will not know whether a connected central is a known device or not.
 */

#include "ble_stack.h"
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_soc.h"
#include "nrf_sdm.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_hids.h"
#include "ble_dis.h"
#include "ble_gap.h"
#include "ble_conn_params.h"
#include "gpio_hal.h"

//#include "bsp_btn_ble.h"
#include "app_scheduler.h"
#include "softdevice_handler_appsh.h"
#include "app_timer_appsh.h"
#include "device_manager.h"
#include "app_button.h"
#include "pstorage.h"
#include "app_trace.h"
#include "ble_advertising.h"
#include "nrf_delay.h"

//#include "bsp.h"
//#include "bsp_btn_ble.h"

#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                          /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/

#define CENTRAL_LINK_COUNT              0                                          /**< Number of central links used by the application. When changing this number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT           1                                          /**< Number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/

#define UART_TX_BUF_SIZE                256                                        /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                1                                          /**< UART RX buffer size. */

#define DEVICE_NAME                     "BlePox"                                    /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME               "NordicSemiconductor"                      /**< Manufacturer. Will be passed to Device Information Service. */

#define APP_TIMER_PRESCALER             0                                          /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE         4                                          /**< Size of timer operation queues. */

#define PNP_ID_VENDOR_ID_SOURCE         0x02                                        /**< Vendor ID Source. */
#define PNP_ID_VENDOR_ID                0xAAAA                                      /**< Vendor ID. */
#define PNP_ID_PRODUCT_ID               0xEEEE                                      /**< Product ID. */
#define PNP_ID_PRODUCT_VERSION          0x0001                                      /**< Product Version. */

/*lint -emacro(524, MIN_CONN_INTERVAL) // Loss of precision */
#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(7.5, UNIT_1_25_MS)            /**< Minimum connection interval (7.5 ms). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(15, UNIT_1_25_MS)             /**< Maximum connection interval (15 ms). */
#define SLAVE_LATENCY                   20                                          /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(3000, UNIT_10_MS)             /**< Connection supervisory timeout (3000 ms). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAM_UPDATE_COUNT     3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                  1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                  0                                           /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS              0                                           /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                        /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                           /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                          /**< Maximum encryption key size. */

#define INPUT_REPORT_COUNT              1                                           /**< Number of input reports in this application. */
#define INPUT_REP_BUTTONS_MOVEMENT_LEN  9                                           /**< Length of Mouse Input Report containing button data. */
#define INPUT_REP_MEDIA_PLAYER_LEN      7                                           /**< Length of Mouse Input Report containing media player data. */
#define INPUT_REP_CUSTOM_LEN            19                                          /**< Length of Mouse Input Report containing media player data. */
#define INPUT_REP_BUTTONS_MOVEMENT_IDX  0                                           /**< Index of Mouse Input Report containing button data. */
#define INPUT_REP_MPLAYER_IDX           1                                           /**< Index of Mouse Input Report containing media player data. */
#define INPUT_REP_CUSTOM_IDX            0                                           /**< Id of reference to Mouse Input Report containing button data. */

#define OUTPUT_REPORT_COUNT             1                                           /**< Number of output reports in this application. */
#define OUTPUT_REP_CUSTOM_LEN           19                                          /**< Size of custom output report in this application. */
#define OUTPUT_REP_CUSTOM_IDX           0                                           /**< Custom output report index in this application. */

#define BASE_USB_HID_SPEC_VERSION       0x0101                                      /**< Version number of base USB HID Specification implemented by this application. */

#define SCHED_MAX_EVENT_DATA_SIZE       MAX(APP_TIMER_SCHED_EVT_SIZE,\
                                            BLE_STACK_HANDLER_SCHED_EVT_SIZE)       /**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE                10                                          /**< Maximum number of events in the scheduler queue. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define APP_FEATURE_NOT_SUPPORTED       BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2        /**< Reply when unsupported features are requested. */

#define APP_ADV_FAST_INTERVAL           0x0028                                      /**< Fast advertising interval (in units of 0.625 ms. This value corresponds to 25 ms.). */
#define APP_ADV_SLOW_INTERVAL           0x0C80                                      /**< Slow advertising interval (in units of 0.625 ms. This value corrsponds to 2 seconds). */

#define APP_ADV_FAST_TIMEOUT            30                                          /**< The duration of the fast advertising period (in seconds). */
#define APP_ADV_SLOW_TIMEOUT            180                                         /**< The duration of the slow advertising period (in seconds). */

static ble_hids_t                       m_hids;                                     /**< Structure used to identify the HID service. */
static bool                             m_in_boot_mode = false;                     /**< Current protocol mode. */
static volatile bool                    isBleHidReportReady = false;
static uint16_t                         m_conn_handle = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current connection. */

static dm_application_instance_t        m_app_handle;                               /**< Application identifier allocated by device manager. */
static dm_handle_t                      m_bonded_peer_handle;                       /**< Device reference handle to the current connected peer. */

static ble_uuid_t m_adv_uuids[] = {{BLE_UUID_HUMAN_INTERFACE_DEVICE_SERVICE, BLE_UUID_TYPE_BLE}}; /**< Universally unique service identifiers. */
static uint8_t bleHidReportBuffer[INPUT_REP_CUSTOM_LEN + 1];

static void on_hids_evt(ble_hids_t * p_hids, ble_hids_evt_t * p_evt);
static void on_hid_rep_char_write(ble_hids_evt_t *p_evt);

static NRF_BleHidReportReadyCb bleHidReportReadyCb = NULL;

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for handling Service errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void service_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for handling advertising errors.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void ble_advertising_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_PULSE_OXIMETER_WRIST_WORN);
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing Device Information Service.
 */
static void dis_init(void)
{
    uint32_t         err_code;
    ble_dis_init_t   dis_init_obj;
    ble_dis_pnp_id_t pnp_id;

    pnp_id.vendor_id_source = PNP_ID_VENDOR_ID_SOURCE;
    pnp_id.vendor_id        = PNP_ID_VENDOR_ID;
    pnp_id.product_id       = PNP_ID_PRODUCT_ID;
    pnp_id.product_version  = PNP_ID_PRODUCT_VERSION;

    memset(&dis_init_obj, 0, sizeof(dis_init_obj));

    ble_srv_ascii_to_utf8(&dis_init_obj.manufact_name_str, MANUFACTURER_NAME);
    dis_init_obj.p_pnp_id = &pnp_id;

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&dis_init_obj.dis_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init_obj.dis_attr_md.write_perm);

    err_code = ble_dis_init(&dis_init_obj);
    APP_ERROR_CHECK(err_code);
}


static ble_hids_inp_rep_init_t     inp_rep_array[INPUT_REPORT_COUNT];
static ble_hids_outp_rep_init_t    outputReportInitStructArray[OUTPUT_REPORT_COUNT];

/**@brief Function for initializing HID Service.
 */
static void hids_init(void)
{
    uint32_t                    err_code;
    ble_hids_init_t             hids_init_obj;
    ble_hids_inp_rep_init_t     *p_input_report;
    uint8_t                     hid_info_flags;

    static uint8_t rep_map_data[] =
    {
        0x06, 0x00, 0xFF, // Usage Page (Vendor-Defined 1)
        0x09, 0x04,       // Usage (Vendor-Defined 1)
        0xA1, 0x01,       // Collection (Application)
        0x85, 0x0E,       //    Report Id (0E)
        0x95, 0x13,       //    Report Count (20)
        0x75, 0x08,       //    Report Size (8)
        0x15, 0x00,       //    Logical Minimum (0)
        0x26, 0xff, 0x00, //    Logical Maximum (255)
        0x09, 0x04,       //    Usage (Vendor-Define )
        0x91, 0x02,       //    Output (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
        0x09, 0x04,       //    Usage (Vendor-Define )
        0x81, 0x02,       //    Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
        0xC0,             // End Collection
    };

    memset(inp_rep_array, 0, sizeof(inp_rep_array));
    // Initialize HID Service.

    p_input_report                      = &inp_rep_array[INPUT_REP_CUSTOM_IDX];
    p_input_report->max_len             = INPUT_REP_CUSTOM_LEN;
    p_input_report->rep_ref.report_id   = 0x0E;
    p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.write_perm);

    memset(outputReportInitStructArray, 0, sizeof(outputReportInitStructArray));
    outputReportInitStructArray[OUTPUT_REP_CUSTOM_IDX].max_len = OUTPUT_REP_CUSTOM_LEN;
    outputReportInitStructArray[OUTPUT_REP_CUSTOM_IDX].rep_ref.report_id = 0x0E;
    outputReportInitStructArray[OUTPUT_REP_CUSTOM_IDX].rep_ref.report_type = BLE_HIDS_REP_TYPE_OUTPUT;

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&outputReportInitStructArray[OUTPUT_REP_CUSTOM_IDX].security_mode.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&outputReportInitStructArray[OUTPUT_REP_CUSTOM_IDX].security_mode.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&outputReportInitStructArray[OUTPUT_REP_CUSTOM_IDX].security_mode.write_perm);

    hid_info_flags = HID_INFO_FLAG_REMOTE_WAKE_MSK | HID_INFO_FLAG_NORMALLY_CONNECTABLE_MSK;

    memset(&hids_init_obj, 0, sizeof(hids_init_obj));

    hids_init_obj.evt_handler                    = on_hids_evt;
    hids_init_obj.error_handler                  = service_error_handler;
    hids_init_obj.is_kb                          = false;
    hids_init_obj.is_mouse                       = false;
    hids_init_obj.inp_rep_count                  = INPUT_REPORT_COUNT;
    hids_init_obj.p_inp_rep_array                = inp_rep_array;
    hids_init_obj.outp_rep_count                 = OUTPUT_REPORT_COUNT;
    hids_init_obj.p_outp_rep_array               = outputReportInitStructArray;
    hids_init_obj.feature_rep_count              = 0;
    hids_init_obj.p_feature_rep_array            = NULL;
    hids_init_obj.rep_map.data_len               = sizeof(rep_map_data);
    hids_init_obj.rep_map.p_data                 = rep_map_data;
    hids_init_obj.hid_information.bcd_hid        = BASE_USB_HID_SPEC_VERSION;
    hids_init_obj.hid_information.b_country_code = 0;
    hids_init_obj.hid_information.flags          = hid_info_flags;
    hids_init_obj.included_services_count        = 0;
    hids_init_obj.p_included_services_array      = NULL;

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hids_init_obj.rep_map.security_mode.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hids_init_obj.rep_map.security_mode.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hids_init_obj.hid_information.security_mode.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hids_init_obj.hid_information.security_mode.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
        &hids_init_obj.security_mode_boot_mouse_inp_rep.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
        &hids_init_obj.security_mode_boot_mouse_inp_rep.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
        &hids_init_obj.security_mode_boot_mouse_inp_rep.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hids_init_obj.security_mode_protocol.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hids_init_obj.security_mode_protocol.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hids_init_obj.security_mode_ctrl_point.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hids_init_obj.security_mode_ctrl_point.write_perm);

    err_code = ble_hids_init(&m_hids, &hids_init_obj);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    dis_init();
    hids_init();
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAM_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = NULL;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    gpioPwrOn(false);
    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    uint32_t err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling HID events.
 *
 * @details This function will be called for all HID events which are passed to the application.
 *
 * @param[in]   p_hids  HID service structure.
 * @param[in]   p_evt   Event received from the HID service.
 */
static void on_hids_evt(ble_hids_t * p_hids, ble_hids_evt_t *p_evt)
{
    switch (p_evt->evt_type)
    {
        case BLE_HIDS_EVT_BOOT_MODE_ENTERED:
            m_in_boot_mode = true;
            break;

        case BLE_HIDS_EVT_REPORT_MODE_ENTERED:
            m_in_boot_mode = false;
            break;

        case BLE_HIDS_EVT_REP_CHAR_WRITE:
            on_hid_rep_char_write(p_evt);
            break;

        case BLE_HIDS_EVT_NOTIF_ENABLED:
        {
            dm_service_context_t service_context;
            service_context.service_type        = DM_PROTOCOL_CNTXT_GATT_SRVR_ID;
            service_context.context_data.len    = 0;
            service_context.context_data.p_data = NULL;
            if (m_in_boot_mode)
            {
                // Protocol mode is Boot Protocol mode.
                if (
                    p_evt->params.notification.char_id.uuid == BLE_UUID_BOOT_MOUSE_INPUT_REPORT_CHAR
                   )
                {
                    // The notification of boot mouse input report has been enabled.
                    // Save the system attribute (CCCD) information into the flash.
                    uint32_t err_code;

                    err_code = dm_service_context_set(&m_bonded_peer_handle, &service_context);
                    if (err_code != NRF_ERROR_INVALID_STATE)
                    {
                        APP_ERROR_CHECK(err_code);
                    }
                    else
                    {
                        // The system attributes could not be written to the flash because
                        // the connected central is not a new central. The system attributes
                        // will only be written to flash only when disconnected from this central.
                        // Do nothing now.
                    }
                }
                else
                {
                    // Do nothing.
                }
            }
            else if ((p_evt->params.notification.char_id.rep_index == INPUT_REP_BUTTONS_MOVEMENT_IDX) && (p_evt->params.notification.char_id.rep_type == BLE_HIDS_REP_TYPE_INPUT))
            {
                // The protocol mode is Report Protocol mode. And the CCCD for the report ID
                // INPUT_REP_MOVEMENT_INDEX is changed. Since this application uses this report to
                // send mouse movements, it is now time to store all the CCCD information (system
                // attributes) into the flash.
                uint32_t err_code;

                err_code = dm_service_context_set(&m_bonded_peer_handle, &service_context);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
                else
                {
                    // The system attributes could not be written to the flash because
                    // the connected central is not a new central. The system attributes
                    // will only be written to flash only when disconnected from this central.
                    // Do nothing now.
                }
            }
            else
            {
                // The notification of the report that was enabled by the central is not interesting
                // to this application. So do nothing.
            }
            break;
        }

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_DIRECTED:
            break;
        case BLE_ADV_EVT_FAST:
            break;
        case BLE_ADV_EVT_SLOW:
            break;
        case BLE_ADV_EVT_FAST_WHITELIST:
            break;
        case BLE_ADV_EVT_SLOW_WHITELIST:
            err_code = ble_advertising_restart_without_whitelist();
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;

        case BLE_ADV_EVT_WHITELIST_REQUEST:
        {
            ble_gap_whitelist_t whitelist;
            ble_gap_addr_t    * p_whitelist_addr[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
            ble_gap_irk_t     * p_whitelist_irk[BLE_GAP_WHITELIST_IRK_MAX_COUNT];

            whitelist.addr_count = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;
            whitelist.irk_count  = BLE_GAP_WHITELIST_IRK_MAX_COUNT;
            whitelist.pp_addrs   = p_whitelist_addr;
            whitelist.pp_irks    = p_whitelist_irk;

            err_code = dm_whitelist_create(&m_app_handle, &whitelist);
            APP_ERROR_CHECK(err_code);

            err_code = ble_advertising_whitelist_reply(&whitelist);
            APP_ERROR_CHECK(err_code);
            break;
        }
        case BLE_ADV_EVT_PEER_ADDR_REQUEST:
        {
            ble_gap_addr_t peer_address;

            // Only Give peer address if we have a handle to the bonded peer.
            if(m_bonded_peer_handle.appl_id != DM_INVALID_ID)
            {

                err_code = dm_peer_addr_get(&m_bonded_peer_handle, &peer_address);
                if (err_code != (NRF_ERROR_NOT_FOUND | DEVICE_MANAGER_ERR_BASE))
                {
                    APP_ERROR_CHECK(err_code);

                    err_code = ble_advertising_peer_addr_reply(&peer_address);
                    APP_ERROR_CHECK(err_code);
                }

            }
            break;
        }
        default:
            break;
    }
}


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t                              err_code;
    ble_gatts_rw_authorize_reply_params_t auth_reply;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_EVT_USER_MEM_REQUEST:
            err_code = sd_ble_user_mem_reply(m_conn_handle, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
            if(p_ble_evt->evt.gatts_evt.params.authorize_request.type
               != BLE_GATTS_AUTHORIZE_TYPE_INVALID)
            {
                if ((p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.op
                     == BLE_GATTS_OP_PREP_WRITE_REQ)
                    || (p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.op
                     == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW)
                    || (p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.op
                     == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL))
                {
                    if (p_ble_evt->evt.gatts_evt.params.authorize_request.type
                        == BLE_GATTS_AUTHORIZE_TYPE_WRITE)
                    {
                    auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
                    }
                    else
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
                    }
                    auth_reply.params.write.gatt_status = APP_FEATURE_NOT_SUPPORTED;
                    err_code = sd_ble_gatts_rw_authorize_reply(m_conn_handle,&auth_reply);
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        case BLE_GATTC_EVT_TIMEOUT:
        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server and Client timeout events.
            err_code = sd_ble_gap_disconnect(m_conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a BLE stack
 *          event has been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    dm_ble_evt_handler(p_ble_evt);
    on_ble_evt(p_ble_evt);
    ble_advertising_on_ble_evt(p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
    ble_hids_on_ble_evt(&m_hids, p_ble_evt);
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in]   sys_evt   System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    pstorage_sys_event_handler(sys_evt);
    ble_advertising_on_sys_evt(sys_evt);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;

    #define NRF_CLOCK_LFCLKSRC      {.source        = NRF_CLOCK_LF_SRC_SYNTH,            \
                                 .rc_ctiv       = 0,                                \
                                 .rc_temp_ctiv  = 0,                                \
                                 .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM}

    nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;

    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_APPSH_INIT(&clock_lf_cfg, true);

    ble_enable_params_t ble_enable_params;
    err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
                                                    PERIPHERAL_LINK_COUNT,
                                                    &ble_enable_params);
    APP_ERROR_CHECK(err_code);

    //Check the ram settings against the used number of links
    CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT,PERIPHERAL_LINK_COUNT);

    // Enable BLE stack.
    err_code = softdevice_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t       err_code;
    uint8_t        adv_flags;
    ble_advdata_t  advdata;

    // Build and set advertising data
    memset(&advdata, 0, sizeof(advdata));

    adv_flags                       = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;
    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    advdata.flags                   = adv_flags;
    advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = m_adv_uuids;

    ble_adv_modes_config_t options =
    {
        BLE_ADV_WHITELIST_ENABLED,
        BLE_ADV_DIRECTED_ENABLED,
        BLE_ADV_DIRECTED_SLOW_DISABLED, 0,0,
        BLE_ADV_FAST_ENABLED, APP_ADV_FAST_INTERVAL, APP_ADV_FAST_TIMEOUT,
        BLE_ADV_SLOW_ENABLED, APP_ADV_SLOW_INTERVAL, APP_ADV_SLOW_TIMEOUT
    };

    err_code = ble_advertising_init(&advdata, NULL, &options, on_adv_evt, ble_advertising_error_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the Event Scheduler initialization.
 */
void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}


/**@brief Function for sending a Mouse Movement.
 *
 * @param[in]   x_delta   Horizontal movement.
 * @param[in]   y_delta   Vertical movement.
 */
bool NRF_BleSendMousePos(uint8_t dXlow, uint8_t dXhigh, uint8_t dYlow, uint8_t dYhigh)
{
    if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
        return true;

    uint32_t err_code;

    if (m_in_boot_mode)
    {
        extern uint32_t ble_hids_boot_mouse_inp_rep_send(ble_hids_t * p_hids,
                                          uint8_t      buttons,
                                          int8_t       x_delta,
                                          int8_t       y_delta,
                                          uint16_t     optional_data_len,
                                          uint8_t    * p_optional_data);
        err_code = ble_hids_boot_mouse_inp_rep_send(&m_hids,
                                                    0x00,
                                                    (int8_t)dXlow,
                                                    (int8_t)dYlow,
                                                    0,
                                                    NULL);
    }
    else
    {
        uint8_t buffer[INPUT_REP_BUTTONS_MOVEMENT_LEN];
        memset(buffer, 0, sizeof(buffer));

        buffer[5] = dXlow;
        buffer[6] = dXhigh;
        buffer[7] = dYlow;
        buffer[8] = dYhigh;

        err_code = ble_hids_inp_rep_send(&m_hids,
                                         INPUT_REP_BUTTONS_MOVEMENT_IDX,
                                         INPUT_REP_BUTTONS_MOVEMENT_LEN,
                                         buffer);
    }

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != BLE_ERROR_NO_TX_PACKETS) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
    )
    {
        APP_ERROR_HANDLER(err_code);
    }
    return (err_code == NRF_SUCCESS);
}


/**@brief Function for handling the Device Manager events.
 *
 * @param[in]   p_evt   Data associated to the device manager event.
 */
static uint32_t device_manager_evt_handler(dm_handle_t const * p_handle,
                                           dm_event_t const  * p_event,
                                           ret_code_t        event_result)
{
    APP_ERROR_CHECK(event_result);

    switch (p_event->event_id)
    {
        case DM_EVT_DEVICE_CONTEXT_LOADED: // Fall through.
        case DM_EVT_SECURITY_SETUP_COMPLETE:
            m_bonded_peer_handle = (*p_handle);
            break;
    }

    return NRF_SUCCESS;
}


/**@brief Function for the Device Manager initialization.
 *
 * @param[in] erase_bonds  Indicates whether bonding information should be cleared from
 *                         persistent storage during initialization of the Device Manager.
 */
static void device_manager_init(bool erase_bonds)
{
    uint32_t               err_code;
    dm_init_param_t        init_param = {.clear_persistent_data = erase_bonds};
    dm_application_param_t register_param;

    // Initialize peer device handle.
    err_code = dm_handle_initialize(&m_bonded_peer_handle);
    APP_ERROR_CHECK(err_code);

    // Initialize persistent storage module.
    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);

    err_code = dm_init(&init_param);
    APP_ERROR_CHECK(err_code);

    memset(&register_param.sec_param, 0, sizeof(ble_gap_sec_params_t));

    register_param.sec_param.bond         = SEC_PARAM_BOND;
    register_param.sec_param.mitm         = SEC_PARAM_MITM;
    register_param.sec_param.lesc         = SEC_PARAM_LESC;
    register_param.sec_param.keypress     = SEC_PARAM_KEYPRESS;
    register_param.sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    register_param.sec_param.oob          = SEC_PARAM_OOB;
    register_param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    register_param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    register_param.evt_handler            = device_manager_evt_handler;
    register_param.service_type           = DM_PROTOCOL_CNTXT_GATT_SRVR_ID;

    err_code = dm_register(&m_app_handle, &register_param);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the HID Report Characteristic Write event.
 *
 * @param[in]   p_evt   HID service event.
 */
static void on_hid_rep_char_write(ble_hids_evt_t *p_evt)
{
    if (p_evt->params.char_write.char_id.rep_type == BLE_HIDS_REP_TYPE_OUTPUT)
    {
        uint8_t report_index = p_evt->params.char_write.char_id.rep_index;

        bleHidReportBuffer[0] = outputReportInitStructArray[report_index].rep_ref.report_id;
        ble_hids_outp_rep_get(&m_hids, report_index, p_evt->params.char_write.len, 0, &bleHidReportBuffer[1]);
        isBleHidReportReady = true;
    }
}


/**@brief Function for application main entry.
 */
void NRF_BleStart(bool eraseBonds)
{
    APP_TIMER_APPSH_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, true);
    ble_stack_init();

    device_manager_init(eraseBonds);
    gap_params_init();
    advertising_init();
    services_init();
    conn_params_init();
    uint32_t err_code = sd_ble_gap_tx_power_set(4);
    err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}


void NRF_BleStop(void)
{
    // Stop any impending connection parameters update.
    uint32_t err_code = ble_conn_params_stop();
    APP_ERROR_CHECK(err_code);
    softdevice_handler_sd_disable();
}

void NRF_BleProcess(void)
{
    app_sched_execute();
    if (isBleHidReportReady) {
        if (bleHidReportReadyCb)
            bleHidReportReadyCb(bleHidReportBuffer, OUTPUT_REP_CUSTOM_LEN);

        isBleHidReportReady = false;
    }
}

bool NRF_BleSendHidReport(const uint8_t report[], uint32_t length)
{
    return (ble_hids_inp_rep_send(&m_hids, INPUT_REP_CUSTOM_IDX, INPUT_REP_CUSTOM_LEN, (uint8_t *)report) == NRF_SUCCESS);
}

void NRF_BleSetupHidReportReadyCb(NRF_BleHidReportReadyCb callBack)
{
    bleHidReportReadyCb = callBack;
}
// 1123
