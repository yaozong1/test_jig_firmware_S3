// SPDX-License-Identifier: MIT
// Jig firmware: CDC(USB) <-> UART bridge to DUT U0TXD/U0RXD, with DTR/RTS -> EN/IO0 mapping

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
// Use built-in USB-Serial-JTAG instead of TinyUSB CDC to avoid extra deps
#include "driver/usb_serial_jtag.h"
#include "sdkconfig.h"

// 与 DUT 相连的 UART 口
#define JIG_UART_NUM       UART_NUM_1
#define JIG_UART_BAUD      115200
// 根据你的接线修改下列引脚（默认与之前一致）
#ifndef JIG_UART_TX_PIN
#define JIG_UART_TX_PIN    18    // 治具 -> DUT (接 DUT U0RXD GPIO44)
#endif
#ifndef JIG_UART_RX_PIN
#define JIG_UART_RX_PIN    17    // DUT  -> 治具 (接 DUT U0TXD GPIO43)
#endif

// 通过 CDC 的 DTR/RTS 控制 DUT 进入下载/复位
#ifndef DUT_EN_PIN
#define DUT_EN_PIN         4
#endif
#ifndef DUT_IO0_PIN
#define DUT_IO0_PIN        5
#endif

static const char *TAG = "JIG";

// UART -> CDC 回送到 PC
static void uart_to_usb_task(void *arg)
{
    uint8_t buf[512];
    while (1) {
        int n = uart_read_bytes(JIG_UART_NUM, buf, sizeof(buf), 0);
        if (n > 0) {
            // 写到 USB-Serial-JTAG
            usb_serial_jtag_write_bytes(buf, n, 0);
        } else {
            vTaskDelay(1);
        }
    }
}

static inline void jig_delay_ticks(int ticks)
{
    if (ticks < 1) ticks = 1;
    vTaskDelay(ticks);
}

static void usb_write_str(const char *s)
{
    if (!s) return;
    size_t len = strlen(s);
    if (len) {
        usb_serial_jtag_write_bytes((const uint8_t *)s, len, 0);
    }
}

static void dut_enter_bootloader(void)
{
    // IO0 拉低，EN 低->高 进入下载模式，然后释放 IO0
    gpio_set_level(DUT_IO0_PIN, 0);
    jig_delay_ticks(1);
    gpio_set_level(DUT_EN_PIN, 0);
    jig_delay_ticks(5);
    gpio_set_level(DUT_EN_PIN, 1);
    jig_delay_ticks(12);
    gpio_set_level(DUT_IO0_PIN, 1);
}

static void dut_run_normal(void)
{
    // IO0 拉高，EN 低->高 普通启动
    gpio_set_level(DUT_IO0_PIN, 1);
    jig_delay_ticks(1);
    gpio_set_level(DUT_EN_PIN, 0);
    jig_delay_ticks(5);
    gpio_set_level(DUT_EN_PIN, 1);
}

// 从 USB -> UART 的搬运任务（带命令解析）
static void usb_to_uart_task(void *arg)
{
    uint8_t buf[512];
    char cmd[64];
    int cmd_len = 0;
    bool in_cmd = false;  // 仅当收到前缀 '!' 时拦截一行作为命令

    while (1) {
        int n = usb_serial_jtag_read_bytes(buf, sizeof(buf), 0);
        if (n > 0) {
            for (int i = 0; i < n; ++i) {
                uint8_t b = buf[i];
                if (!in_cmd) {
                    if (b == '!') {
                        in_cmd = true;
                        cmd_len = 0;
                        continue;  // 不透传 '!'
                    }
                    // 非命令，直接透传到 DUT UART
                    uart_write_bytes(JIG_UART_NUM, (const char *)&b, 1);
                } else {
                    if (b == '\r' || b == '\n') {
                        cmd[cmd_len] = '\0';
                        // 解析命令（全部使用大写进行匹配）
                        if (strcmp(cmd, "BOOT") == 0) {
                            usb_write_str("JIG: BOOT start\r\n");
                            dut_enter_bootloader();
                            usb_write_str("JIG: BOOT OK (DUT ready to flash)\r\n");
                        } else if (strcmp(cmd, "RUN") == 0) {
                            usb_write_str("JIG: RUN start\r\n");
                            dut_run_normal();
                            usb_write_str("JIG: RUN OK (DUT reset to app)\r\n");
                        } else if (strcmp(cmd, "RST") == 0) {
                            usb_write_str("JIG: RST start\r\n");
                            // 简单复位：保持 IO0 当前电平，仅脉冲 EN
                            int io0 = gpio_get_level(DUT_IO0_PIN);
                            (void)io0;
                            gpio_set_level(DUT_EN_PIN, 0);
                            jig_delay_ticks(5);
                            gpio_set_level(DUT_EN_PIN, 1);
                            usb_write_str("JIG: RST OK\r\n");
                        } else {
                            usb_write_str("JIG: UNKNOWN CMD\r\n");
                        }
                        in_cmd = false;
                        cmd_len = 0;
                    } else {
                        if (cmd_len < (int)sizeof(cmd) - 1) {
                            // 规范化为大写，简化比较
                            if (b >= 'a' && b <= 'z') b = (uint8_t)(b - 'a' + 'A');
                            cmd[cmd_len++] = (char)b;
                        }
                    }
                }
            }
        } else {
            vTaskDelay(1);
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Jig starting: CDC<->UART bridge, UART%d RX=%d TX=%d, EN=%d IO0=%d",
             JIG_UART_NUM, JIG_UART_RX_PIN, JIG_UART_TX_PIN, DUT_EN_PIN, DUT_IO0_PIN);

    // 控制脚：默认拉高（运行模式）
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << DUT_EN_PIN) | (1ULL << DUT_IO0_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io));
    gpio_set_level(DUT_EN_PIN, 1);
    gpio_set_level(DUT_IO0_PIN, 1);

    // UART 初始化
    uart_config_t cfg = {
        .baud_rate = JIG_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(JIG_UART_NUM, 4096, 2048, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(JIG_UART_NUM, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(JIG_UART_NUM, JIG_UART_TX_PIN, JIG_UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // 安装 USB-Serial-JTAG 驱动（PC 看到的是一个 COM 口）
    usb_serial_jtag_driver_config_t usj_cfg = {
        .tx_buffer_size = 1024,
        .rx_buffer_size = 1024,
    };
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usj_cfg));

    ESP_LOGI(TAG, "USB-Serial-JTAG ready. Open this COM to talk with DUT. esptool can use it, DTR/RTS control to be mapped.");

    // 向 PC 打一行提示，便于 GUI 识别
    usb_write_str("JIG: READY\r\n");

    // 从 USB -> UART 的搬运（轮询读取）
    xTaskCreate(usb_to_uart_task, "usb_to_uart", 4096, NULL, 8, NULL);

    // 从 UART -> USB 的搬运
    xTaskCreate(uart_to_usb_task, "uart_to_usb", 4096, NULL, 8, NULL);
}
