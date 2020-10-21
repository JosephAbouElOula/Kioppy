#include "Arduino.h"
#include "driver/uart.h"
#include "ArduinoLog.h"
#include "Global.h"
#include "MQTT/MQTT.h"
#include "Hardware/Hardware.h"
#include "Nextion.h"
#include "NVS/KIOPPY_NVS.h"
#include "Medicine/Medicine.h"

// #define LCD_SERIAL UART_NUM_1
#define LCD_SERIAL Serial1
#define LCD_RX 19
#define LCD_TX 18
#define LCD_UART_BUFFER_SIZE (UART_FIFO_LEN + 1)
#define LCD_UART_QUEUE_SIZE 32
#define LCD_RECEIVE_TASK_PRIORITY 1
#define LCD_TRANSMIT_TASK_PRIORITY 2

QueueHandle_t lcdUartRxQueue;
QueueHandle_t lcdUartTxQueue;

TaskHandle_t lcdUartRxHandle;
TaskHandle_t lcdUartTxHandle;

String lockCode;
ScannedMed scannedMed = NO_MED_SCANNED;

void setLockCode(String code)
{
	nvsSaveStr(nvsConf.lockCode.key, code.c_str());

	lockCode = code;
}

void lcdSendCommand(String str)
{
	Log.verbose("Message to Send %s" CR, str.c_str()); // + String ((char) 0xFF + (char) 0xFF + (char) 0xFF ));
	// LCD_SERIAL.print(str);
	xQueueSendToBack(lcdUartTxQueue, &str, pdMS_TO_TICKS(500));
}

static void lcd_uart_tx_task(void *pvParameters)
{
	String strCommand = "";

	while (1)
	{
		if (xQueueReceive(lcdUartTxQueue, (void *)&strCommand, (portTickType)portMAX_DELAY))
		{
			// Log.verbose("Message to Send From Queue %s" CR, strCommand.c_str()); // + String ((char) 0xFF + (char) 0xFF + (char) 0xFF ));
			LCD_SERIAL.print(strCommand);
			LCD_SERIAL.write(0xFF);
			LCD_SERIAL.write(0xFF);
			LCD_SERIAL.write(0xFF);
		}
		// vTaskDelay(pdMS_TO_TICKS(100));
	}
}

static void lcd_uart_Queue_task(void *pvParameters)
{
	String newCode;
	String expDate;
	String strData;
	static String Barcode;

	// static String SSID;
	// static String PWD;

	String qty;

	while (1)
	{
		if (xQueueReceive(lcdUartRxQueue, (void *)&strData, (portTickType)portMAX_DELAY))
		{
			Log.verbose("msg Received %s" CR, strData.c_str());

			if (strData.charAt(0) == 'N')
			{
				// New PassCode was set

				newCode = strData.substring(1);
				setLockCode(newCode);
				nvsSaveU8(nvsConf.needs_setup.key, 0);

				Log.verbose("New Code set to %s" CR, newCode.c_str());
			}
			else if (strData.charAt(0) == 'U')
			{
				// unlock the door
				Log.verbose("Unlock Door" CR);
				unlockDoor();
			}
			else if (strData.charAt(0) == 'R')
			{
				//Remove medicine from the cabinet
				Barcode = strData.substring(1, strData.length() - 1);
				mqttStruct_t removeMedicine;
				removeMedicine.msgType = REMOVE_MED;
				removeMedicine.barcode = Barcode;
				Log.verbose("Barcode %s" CR, removeMedicine.barcode.c_str());
				xQueueSendToBack(mqttQ, &removeMedicine, pdMS_TO_TICKS(500));
				//TODO: do we need a new screen after removing?
			}
			else if (strData.charAt(0) == 'A')
			{

				//Add medicine to the cabinet
				expDate = strData.substring(1);
				mqttStruct_t addMed;
				addMed.msgType = PUT_MED;
				addMed.barcode = Barcode;
				addMed.expDate = expDate;
				xQueueSendToBack(mqttQ, &addMed, pdMS_TO_TICKS(500));
			}
			else if (strData.charAt(0) == 'T')
			{
				// Take medicine from the cabinet
				//Barcode already sent
				float tmp = strData.substring(1).toFloat();
				// qty = strData.substring(1);
				tmp = tmp / 100;
				mqttStruct_t takeMed;
				takeMed.msgType = TAKE_MED;
				takeMed.barcode = Barcode;
				takeMed.qty = tmp;

				xQueueSendToBack(mqttQ, &takeMed, pdMS_TO_TICKS(500));
			}
			else if (strData.charAt(0) == 'S')
			{
				cancelScan = false;
				//Medicine Scanned, waiting for name from Mqtt

				if (strData.charAt(1) == 'A')
				{
					//Add Med
					scannedMed = PUT_MED_SCANNED;
					Medicine *med = NULL;
					Barcode = strData.substring(2, strData.length() - 1);
					Log.verbose("BARCODE is %s" CR, (char *)Barcode.c_str());
					if (allMedicines->getMedicineByBarcode((char *)Barcode.c_str(), *med) == 1)
					{
						//medicine found
						Log.error("BARCODE found");
					}
					else
					{
						//Medicine not found
						lcdSendCommand("page MedNotFound");
					}
				}
				else if (strData.charAt(1) == 'T')
				{
					//Take Med
					scannedMed = TAKE_MED_SCANNED;
				}
				Barcode = strData.substring(2, strData.length() - 1);
				Medicine *med = NULL;
				if (allMedicines->getMedicineByBarcode((char *)Barcode.c_str(), *med) == 1)
				{
					//medicine found
					if (scannedMed == TAKE_MED_SCANNED)
					{
						if (med->getType() == PILLS)
						{
							lcdSendCommand("TakeMed.v_type.val=0");
						}
						else
						{
							lcdSendCommand("TakeMed.v_type.val=1");
						}
						lcdSendCommand("TakeMed.v_name.txt=\"" + (String)med->getDescription() + "\"");
						//  vTaskDelay(pdMS_TO_TICKS(50));
						lcdSendCommand("page TakeMed");
					}
					else if (scannedMed == PUT_MED_SCANNED)
					{
						lcdSendCommand("ExpDate.t_name.txt=\"" + (String)med->getDescription() + "\"");
						lcdSendCommand("page ExpDate");
					}
				}
				else
				{
					//Medicine not found
				}

				// mqttStruct_t getMedName;
				// getMedName.msgType = SCAN_MED;
				// getMedName.barcode = Barcode;
				// xQueueSendToBack(mqttQ, &getMedName, pdMS_TO_TICKS(500));
			}

			else if (strData.charAt(0) == 'W')
			{
				//SSID
				// wifiSSID = strData.substring(1).c_str();
				strcpy(nvsConf.wifiSSID.value, strData.substring(1).c_str());
			}
			else if (strData.charAt(0) == 'P')
			{
				//Wifi Password
				// wifiPassword = strData.substring(1);
				strcpy(nvsConf.wifiPassword.value, strData.substring(1).c_str());

				nvsSaveStr(nvsConf.wifiSSID.key, nvsConf.wifiSSID.value);
				nvsSaveStr(nvsConf.wifiPassword.key, nvsConf.wifiPassword.value);

				monitoringQueueAdd(WIFI_RECOONECT_EVT);
				// WiFi.begin(SSID.c_str(), PWD.c_str());
			}
			else if (strData.charAt(0) == 'F')
			{
				//Factory Reset
				nvsSaveU8(nvsConf.blwr.key, 1);
				nvsSaveU8(nvsConf.needs_setup.key, 1);
				ESP.restart();
			}
			else if (strData.charAt(0) == 'B')
			{
				//Blower
				if (strData.charAt(1) == 'E')
				{
					nvsSaveU8(nvsConf.blwr.key, 1);
					blwrEnable = 1;
				}
				else
				{
					nvsSaveU8(nvsConf.blwr.key, 0);
					blwrEnable = 0;
					turnFanOff();
				}
			}
			else if (strData.charAt(0) == 'C')
			{
				//Cancel Scan
				cancelScan = true;
			}
			else if (strData.charAt(0) == 'X')
			{
				//New Medicine
				MedicineParams_t med = {};
				strcpy(med.barcode, Barcode.c_str());
				strcpy(med.description, strData.substring(10).c_str());
				med.type = strData.charAt(1);
				med.qty = 100;
				Log.verbose("Adding Medicines" CR);
				Log.verbose("-->Barcode: %s" CR, med.barcode);
				Log.verbose("-->description: %s" CR, med.description);
				allMedicines->createNewMedicine(&med);
				allMedicines->listMedicines();
				// mqttStruct_t notFoundMed;
				// notFoundMed.msgType = NEW_MED;
				// notFoundMed.barcode = Barcode;
				// if (strData.charAt(1) == '1')
				// {
				// 	notFoundMed.form = "pills";
				// }
				// else
				// {
				// 	notFoundMed.form = "syrup";
				// }
				// notFoundMed.expDate = strData.substring(2, 10);
				// notFoundMed.name = strData.substring(10);
				// xQueueSendToBack(mqttQ, &notFoundMed, pdMS_TO_TICKS(500));
			}
		}
	}
}

static void readLCDSerial(void *pvParameters)
{
	String tmpSerial;
	while (1)
	{
		if (LCD_SERIAL.available())
		{
			tmpSerial = LCD_SERIAL.readString();
			xQueueSendToBack(lcdUartRxQueue, &tmpSerial, pdMS_TO_TICKS(500));
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void nextionUartInit(void)
{

	// LCD_SERIAL.begin(9600, SERIAL_8N1, LCD_RX, LCD_TX);
	LCD_SERIAL.begin(115200, SERIAL_8N1, LCD_RX, LCD_TX);
	lcdUartRxQueue = xQueueCreate(10, 50);
	lcdUartTxQueue = xQueueCreate(10, 30);
	xTaskCreate(readLCDSerial, "LCD_UART_RX", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
	xTaskCreate(lcd_uart_Queue_task, "lcd_uart_Queue_task", 4096, NULL, LCD_TRANSMIT_TASK_PRIORITY, &lcdUartRxHandle);
	xTaskCreate(lcd_uart_tx_task, "lcd_uart_tx_task", 2048, NULL, LCD_TRANSMIT_TASK_PRIORITY, &lcdUartTxHandle);

	lcdSendCommand("rest");
	vTaskDelay(pdMS_TO_TICKS(150));
	Log.verbose("Needs Setup %d" CR, nvsConf.needs_setup.value);

	lcdSendCommand("needs_setup.val=" + String(nvsConf.needs_setup.value));
	vTaskDelay(pdMS_TO_TICKS(350));
	lcdSendCommand("needs_setup.val=" + String(nvsConf.needs_setup.value));
	vTaskDelay(pdMS_TO_TICKS(150));
	if (!nvsConf.needs_setup.value)
	{
		//read the stored code in the EEPROM
		String tmp = nvsConf.lockCode.value;

		lcdSendCommand("lock_code.txt=\"" + tmp + "\"");

		if (doorIsOpen())
		{
			vTaskDelay(pdMS_TO_TICKS(150));
			lcdSendCommand("v_door.val=1");
		}
	}
	// lcdSendCommand("page page0");
	/*   uart_config_t uartConfig = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_param_config(LCD_SERIAL, &uartConfig);
    uart_set_pin(LCD_SERIAL, LCD_TX, LCD_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(LCD_SERIAL, LCD_UART_BUFFER_SIZE, 0, LCD_UART_QUEUE_SIZE, &lcdUartRxQueue, 0);

    //TODO: need to update based on TX format
    lcdUartTxQueue = xQueueCreate(LCD_UART_QUEUE_SIZE, sizeof(int));

    xTaskCreate(lcd_uart_rx_task, "lcd_uart_rx_task", 2048, NULL, LCD_RECEIVE_TASK_PRIORITY, &lcdUartRxHandle);
    xTaskCreate(lcd_uart_tx_task, "lcd_uart_tx_task", 2048, NULL, LCD_TRANSMIT_TASK_PRIORITY, &lcdUartTxHandle);*/

	// LCD_SERIAL.begin(9600, SERIAL_8N1, LCD_RX, LCD_TX);
	// xTaskCreate(readLCDSerial, "LCD_UART_RX", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}