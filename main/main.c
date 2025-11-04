#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"

// 简单治具固件：把 DUT 串口数据原样透传到 USB-Serial-JTAG（PC 识别的 COM 口）
// 这样你的 Python GUI 直接打开治具的 COM 口即可看到 DUT 打印的 "SELFTEST SUMMARY: { ... }" JSON
// 后续可在此工程逐步增加：RS485 回显、CAN 回显、8路 ADC 读取与上报、DTR/RTS 控制 DUT EN/IO0 等

#define JIG_UART_NUM       UART_NUM_1   // 与 DUT 相连的 UART 口
#define JIG_UART_BAUD      115200
// 根据你的接线修改下列引脚
#ifndef JIG_UART_TX_PIN
#define JIG_UART_TX_PIN    18           // 治具 -> DUT（如需回发）
#endif
#ifndef JIG_UART_RX_PIN
#define JIG_UART_RX_PIN    17           // DUT -> 治具，读取 DUT 日志
#endif

static const char *TAG = "JIG";

static void uart_passthrough_task(void *arg)
{
    // 配置 UART1（或你选择的 UART）
    uart_config_t cfg = {
        .baud_rate = JIG_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(JIG_UART_NUM, 4 * 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(JIG_UART_NUM, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(JIG_UART_NUM, JIG_UART_TX_PIN, JIG_UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    uint8_t buf[1024];
    ESP_LOGI(TAG, "UART%d ready: RX=%d TX=%d, baud=%d. Forwarding to USB console...",
             JIG_UART_NUM, JIG_UART_RX_PIN, JIG_UART_TX_PIN, JIG_UART_BAUD);

    // 主循环：读取 DUT 串口并原样打印到 stdout（USB-Serial-JTAG）
    // 注意：这里不添加前缀，避免干扰 Python GUI 的正则查找
    while (1) {
        int n = uart_read_bytes(JIG_UART_NUM, buf, sizeof(buf), 20 / portTICK_PERIOD_MS);
        if (n > 0) {
            // 原样输出到 USB-Serial-JTAG（stdio）
            fwrite(buf, 1, n, stdout);
            fflush(stdout);
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Test Jig firmware starting (minimal passthrough)");

    // S3 的缺省控制台是 USB-Serial-JTAG，PC 插上即出现一个 COM 口
    // 打开该 COM 口即可接收这里的 stdout 输出

    xTaskCreate(uart_passthrough_task, "uart_passthrough", 4096, NULL, 8, NULL);
}
