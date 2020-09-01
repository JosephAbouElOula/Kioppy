
#include "Arduino.h"
#include "driver/uart.h"
#include "ArduinoLog.h"
// #include "Nextion/Nextion.h"
#include "Global.h" 

// #define BARCODE_SERIAL UART_NUM_2
#define Barcode_Serial Serial2
#define BARCODE_RX 22
#define BARCODE_TX 21

#define BARCODE_UART_BUFFER_SIZE (UART_FIFO_LEN + 1)
#define BARCODE_UART_QUEUE_SIZE 32
#define BARCODE_RECEIVE_TASK_PRIORITY 1
#define BARCODE_TRANSMIT_TASK_PRIORITY 2

QueueHandle_t barcodeUartRxQueue;
TaskHandle_t barcodeUartRxHandle;
String detectedBarcode; 

// static void barcode_uart_rx_task(void *pvParameters)
// {

//     uart_event_t event;
//     static char data[BARCODE_UART_BUFFER_SIZE] = {0};

//     while (1)
//     {
//         if (xQueueReceive(barcodeUartRxQueue, (void *)&event, (portTickType)portMAX_DELAY))
//         {
//             Log.verbose("Barcode Scanned" CR);
//             /*	uart_read_bytes(STM_UART_NUM, (uint8_t*) data, STM_UART_BUFFER_SIZE, 2 / portTICK_RATE_MS);
// 			// Log anything received before it is decoded
// 			ESP_LOGV(STM_TAG, "received: 0x %.2hhx %.2hhx %.2hhx %.2hhx %.2hhx %.2hhx %.2hhx %.2hhx",
// 					data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
// 			decodeFrame(data);*/
//         }
//     }
// }

static void readBarcodeSerial(void *pvParameters)
{
    while (1)
    {
        if (Barcode_Serial.available())
        {
            detectedBarcode= Barcode_Serial.readString();
            Log.verbose("Barcode Scanned %S" CR, detectedBarcode.c_str());
            lcdSendCommand("t_barcode.txt=\"" + detectedBarcode + "\"");
        }
        vTaskDelay(100 /portTICK_PERIOD_MS);
    }
}

void barcodeUartInit(void)
{
 
    Barcode_Serial.begin(9600, SERIAL_8N1, BARCODE_RX, BARCODE_TX);
    detectedBarcode="";
   xTaskCreate(readBarcodeSerial, "BARCODE_UART_RX", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    /*
    uart_config_t uartConfig = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_param_config(BARCODE_SERIAL, &uartConfig);
    uart_set_pin(BARCODE_SERIAL, BARCODE_TX, BARCODE_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(BARCODE_SERIAL, BARCODE_UART_BUFFER_SIZE, 0, BARCODE_UART_QUEUE_SIZE, &barcodeUartRxQueue, 0);

    //TODO: not sending aything
    // barcodeUartTxQueue = xQueueCreate(LCD_UART_QUEUE_SIZE, sizeof(int));

    xTaskCreate(barcode_uart_rx_task, "lcd_uart_rx_task", 2048, NULL, BARCODE_RECEIVE_TASK_PRIORITY, &barcodeUartRxHandle);*/
    // xTaskCreate(lcd_uart_tx_task, "lcd_uart_tx_task", 2048, NULL, LCD_TRANSMIT_TASK_PRIORITY, &lcdUartTxHandle);

    // LCD_SERIAL.begin(9600, SERIAL_8N1, LCD_RX, LCD_TX);
    // xTaskCreate(readLCDSerial, "LCD_UART_RX", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}