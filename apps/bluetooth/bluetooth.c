
#include "esp_bluedroid_hci.h"
#include "esp_bt_main.h"
#include "panic.h"
#include <assert.h>
#include <osi/assert.h>

void hci_send_stub(uint8_t *data, uint16_t len) {
}

bool hci_check_send_available_stub(void) {
    return false;
}

esp_err_t hci_register_host_callback_stub(const esp_bluedroid_hci_driver_callbacks_t *callback) {
    return ESP_OK;
}

void INIT_ATTR bluetooth_init(void)
{
    esp_bluedroid_hci_driver_operations_t operations = {
        .send = hci_send_stub,
        .check_send_available = hci_check_send_available_stub,
        .register_host_callback = hci_register_host_callback_stub,
    };
    esp_bluedroid_attach_hci_driver(&operations);
    esp_bluedroid_config_t cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    esp_err_t ret;
    if ((ret = esp_bluedroid_init_with_cfg(&cfg)) != ESP_OK) {
        panicf("%s initialize bluedroid failed: %d", __func__, ret);
    }
}
