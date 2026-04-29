
#include "action.h"
#include "asm/thread.h"
#include "esp_a2dp_legacy_api.h"
#include "esp_bluedroid_hci.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "gpio-x1000.h"
#include "panic.h"
#include "semaphore.h"
#include "splash.h"
#include "thread.h"
#include "bcm43430a1_firmware.h"
#include "tick.h"
#include "uart-x1000.h"
#include <osi/assert.h>
#include <stdint.h>
#include <string.h>

void hci_send_uart(uint8_t *data, uint16_t len) {
    uart_tx(PORT_UART0, data, len);
}

bool hci_check_send_available_uart(void) {
    return true;
}

const esp_bluedroid_hci_driver_callbacks_t *hci_callbacks;
struct semaphore hci_callbacks_sem;

esp_err_t hci_register_host_callback_uart(const esp_bluedroid_hci_driver_callbacks_t *callback) {
    if (hci_callbacks != NULL) {
        return ESP_FAIL;
    }
    hci_callbacks = callback;
    semaphore_release(&hci_callbacks_sem);
    return ESP_OK;
}

#define HCI_RECV_BUF_SIZE 1026

void bluetooth_hci_task(void)
{
    uint8_t buf[HCI_RECV_BUF_SIZE];
    int fd = open("/hci_log.txt", O_WRONLY | O_CREAT | O_TRUNC);
    uart_init(PORT_UART0, 115200);
    gpio_set_level(GPIO_BT_REG_ON_HW4, 1);

    fdprintf(fd, "01 03 0C 00\n");
    uart_tx(PORT_UART0, "\x01\x03\x0c\x00", 4);
    size_t read_size = uart_rx(PORT_UART0, buf, HCI_RECV_BUF_SIZE);
    for (size_t i = 0; i < read_size; i++) {
        fdprintf(fd, " %02X", buf[i]);
    }
    fdprintf(fd, "\n");

    // TODO(skyevg): figure out why the chip doesn't like re-bauding before init
    // fdprintf(fd, "01 18 FC 06 00 00 C0 C6 2D 00\n");
    // uart_tx(PORT_UART0, "\x01\x18\xfc\x06\x00\x00\xc0\xc6\x2d\x00", 10);
    // read_size = uart_rx(PORT_UART0, buf, HCI_RECV_BUF_SIZE);
    // for (size_t i = 0; i < read_size; i++) {
    //     fdprintf(fd, " %02X", buf[i]);
    // }
    // fdprintf(fd, "\n");
    
    // uart_set_baud(PORT_UART0, 3000000);

    fdprintf(fd, "01 2E FC 00\n");
    uart_tx(PORT_UART0, "\x01\x2e\xfc\x00", 4);
    read_size = uart_rx(PORT_UART0, buf, HCI_RECV_BUF_SIZE);
    for (size_t i = 0; i < read_size; i++) {
        fdprintf(fd, " %02X", buf[i]);
    }
    fdprintf(fd, "\n");

    sleep(HZ / 20);

    if (uart_pending_rx(PORT_UART0)) {
        fdprintf(fd, "pend");
        read_size = uart_rx(PORT_UART0, buf, HCI_RECV_BUF_SIZE);
        for (size_t i = 0; i < read_size; i++) {
            fdprintf(fd, " %02X", buf[i]);
        }
        fdprintf(fd, "\n");
    }

    for (int i = 0; i < HCD_NUM_INIT_COMMANDS; i++) {    
        fdprintf(fd, "hcd %d\n", i);
        uart_tx(PORT_UART0, hcd_init_commands[i], ((size_t) hcd_init_commands[i][3]) + 4);
        read_size = uart_rx(PORT_UART0, buf, HCI_RECV_BUF_SIZE);
        for (size_t i = 0; i < read_size; i++) {
            fdprintf(fd, " %02x", buf[i]);
        }
        fdprintf(fd, "\n");
    }

    // TODO(skyevg): figure out why the chip doesn't like re-bauding before init
    // uart_set_baud(PORT_UART0, 115200);

    fdprintf(fd, "01 03 0C 00\n");
    uart_tx(PORT_UART0, "\x01\x03\x0c\x00", 4);
    read_size = uart_rx(PORT_UART0, buf, HCI_RECV_BUF_SIZE);
    for (size_t i = 0; i < read_size; i++) {
        fdprintf(fd, " %02X", buf[i]);
    }
    fdprintf(fd, "\n");

    fdprintf(fd, "01 18 FC 06 00 00 C0 C6 2D 00\n");
    uart_tx(PORT_UART0, "\x01\x18\xfc\x06\x00\x00\xc0\xc6\x2d\x00", 10);
    read_size = uart_rx(PORT_UART0, buf, HCI_RECV_BUF_SIZE);
    for (size_t i = 0; i < read_size; i++) {
        fdprintf(fd, " %02X", buf[i]);
    }
    fdprintf(fd, "\n");

    uart_set_baud(PORT_UART0, 3000000);

    fdprintf(fd, "01 14 0C 00\n");
    uart_tx(PORT_UART0, "\x01\x14\x0c\x00", 4);
    read_size = uart_rx(PORT_UART0, buf, HCI_RECV_BUF_SIZE);
    for (size_t i = 0; i < read_size; i++) {
        fdprintf(fd, " %02X", buf[i]);
    }
    fdprintf(fd, "\n");

    fdprintf(fd, "01 09 10 00\n");
    uart_tx(PORT_UART0, "\x01\x09\x10\x00", 4);
    read_size = uart_rx(PORT_UART0, buf, HCI_RECV_BUF_SIZE);
    for (size_t i = 0; i < read_size; i++) {
        fdprintf(fd, " %02X", buf[i]);
    }
    fdprintf(fd, "\n");

    close(fd);

    hci_callbacks = NULL;
    semaphore_init(&hci_callbacks_sem, 1, 0);

    esp_bluedroid_hci_driver_operations_t operations = {
        .send = hci_send_uart,
        .check_send_available = hci_check_send_available_uart,
        .register_host_callback = hci_register_host_callback_uart,
    };
    esp_bluedroid_attach_hci_driver(&operations);
    esp_bluedroid_config_t cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    esp_err_t ret;
    if ((ret = esp_bluedroid_init_with_cfg(&cfg)) != ESP_OK) {
        panicf("%s initialize bluedroid failed: %d", __func__, ret);
    }

    semaphore_wait(&hci_callbacks_sem, TIMEOUT_BLOCK);

    thread_set_priority(thread_self(), PRIORITY_REALTIME);

    while (true) {
        read_size = uart_rx(PORT_UART0, buf, HCI_RECV_BUF_SIZE);
        hci_callbacks->notify_host_recv(buf, read_size);
    }
}

uint32_t hci_stack[(DEFAULT_STACK_SIZE + HCI_RECV_BUF_SIZE + 1024) / sizeof(uint32_t)];

void bluetooth_init(void)
{
    create_thread(bluetooth_hci_task, hci_stack, (DEFAULT_STACK_SIZE + HCI_RECV_BUF_SIZE + 1024), 0, "bt_hci", PRIORITY_BACKGROUND);
}

bool bluetooth_enable_discover(void)
{
    esp_err_t ret;
    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        panicf("%s enable bluedroid failed: %d", __func__, ret);
    }
    if ((ret = esp_bt_gap_set_device_name("Rockbox")) != ESP_OK) {
        panicf("%s set device name failed: %d", __func__, ret);
    }
    if ((ret = esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE)) != ESP_OK) {
        panicf("%s set scan mode failed: %d", __func__, ret);
    }
    esp_a2d_source_register_data_callback(NULL);
    return true;
}