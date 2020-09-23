#include "Arduino.h"
#include "driver/uart.h"
#include "ArduinoLog.h"
#include "Global.h"
#include "MQTT/MQTT.h"
#include "Hardware/Hardware.h"
#include "Nextion.h"
#include "Eeprom/eeprom.h"

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
	writeStringToEEPROM(LOCK_CODE_ADDRESS, code);
	lockCode = code;
}

// static void lcd_uart_rx_task(void *pvParameters)
// {

// 	uart_event_t event;
// 	static char data[LCD_UART_BUFFER_SIZE] = {0};

// 	while (1)
// 	{
// 		if (xQueueReceive(lcdUartRxQueue, (void *)&event, (portTickType)portMAX_DELAY))
// 		{
// 			Log.verbose("msg Received" CR);
// 			/*	uart_read_bytes(STM_UART_NUM, (uint8_t*) data, STM_UART_BUFFER_SIZE, 2 / portTICK_RATE_MS);
// 			// Log anything received before it is decoded
// 			ESP_LOGV(STM_TAG, "received: 0x %.2hhx %.2hhx %.2hhx %.2hhx %.2hhx %.2hhx %.2hhx %.2hhx",
// 					data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
// 			decodeFrame(data);*/
// 		}
// 	}
// }

// static void lcd_uart_tx_task(void *pvParameters)
// {

// 	//stmUartFrame_t message;

// 	while (1)
// 	{
// 		//if (xQueueReceive(lcdUartTxQueue, (void*)&message, (portTickType)portMAX_DELAY)) {

// 		//	lcdUartTransmit(message.command, (void*)message.payload.b, message.payloadSize);
// 		//	if (message.blocking) lcdUartTxBlocking--;
// 		//}
// 	}
// }
void lcdSendCommand(String str)
{
	Log.verbose("Message to Send %s" CR, str.c_str()); // + String ((char) 0xFF + (char) 0xFF + (char) 0xFF ));
	// LCD_SERIAL.print(str);
	xQueueSendToBack(lcdUartTxQueue, &str, portMAX_DELAY);
}

static void lcd_uart_tx_task(void *pvParameters)
{
	String strCommand = "";

	while (1)
	{
		if (xQueueReceive(lcdUartTxQueue, (void *)&strCommand, (portTickType)portMAX_DELAY))
		{
			Log.verbose("Message to Send From Queue %s" CR, strCommand.c_str()); // + String ((char) 0xFF + (char) 0xFF + (char) 0xFF ));
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
				EEPROM.write(SETUP_ADDRESS, 0);
				EEPROM.commit();
				Log.verbose("New Code set to %s" CR, newCode.c_str());
				uint8_t needs_setup = EEPROM.read(SETUP_ADDRESS);
				Log.verbose("Needs Setup %d" CR, needs_setup);
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
				xQueueSendToBack(mqttQ, &removeMedicine, portMAX_DELAY);
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
				xQueueSendToBack(mqttQ, &addMed, portMAX_DELAY);
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

				xQueueSendToBack(mqttQ, &takeMed, portMAX_DELAY);
			}
			else if (strData.charAt(0) == 'S')
			{
				cancelScan = false;
				//Medicine Scanned, waiting for name from Mqtt

				if (strData.charAt(1) == 'A')
				{
					//Add Med
					scannedMed = PUT_MED_SCANNED;
				}
				else if (strData.charAt(1) == 'T')
				{
					//Take Med
					scannedMed = TAKE_MED_SCANNED;
				}
				Barcode = strData.substring(2, strData.length() - 1);

				mqttStruct_t getMedName;
				getMedName.msgType = SCAN_MED;
				getMedName.barcode = Barcode;
				xQueueSendToBack(mqttQ, &getMedName, portMAX_DELAY);
			}

			else if (strData.charAt(0) == 'W')
			{
				//SSID
				wifiSSID = strData.substring(1);
			}
			else if (strData.charAt(0) == 'P')
			{
				//Wifi Password
				wifiPassword = strData.substring(1);
				writeStringToEEPROM(SSID_ADDRESS, wifiSSID);
				writeStringToEEPROM(PASSWORD_ADDRESS, wifiPassword);
				monitoringQueueAdd(WIFI_RECOONECT_EVT);
				// WiFi.begin(SSID.c_str(), PWD.c_str());
			}
			else if (strData.charAt(0) == 'F')
			{
				//Factory Reset
				EEPROM.write(BLOWER_ADDRESS, 1);
				EEPROM.write(SETUP_ADDRESS, 1);
				EEPROM.commit();
				ESP.restart();
			}
			else if (strData.charAt(0) == 'B')
			{
				//Blower
				if (strData.charAt(1) == 'E')
				{
					EEPROM.write(BLOWER_ADDRESS, 1);
					EEPROM.commit();
					blwrEnable = 1;
				}
				else
				{

					EEPROM.write(BLOWER_ADDRESS, 0);
					EEPROM.commit();
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
				mqttStruct_t notFoundMed;
				notFoundMed.msgType = NEW_MED;
				notFoundMed.barcode = Barcode;
				if (strData.charAt(1) == '1')
				{
					notFoundMed.form = "pills";
				}
				else
				{
					notFoundMed.form = "syrup";
				}
				notFoundMed.expDate = strData.substring(2, 10);
				notFoundMed.name = strData.substring(10);
				xQueueSendToBack(mqttQ, &notFoundMed, portMAX_DELAY);
			}
		}
	}

	// switch (strData.charAt(0))
	// {
	// case 'M':
	// {
	// 	// Main Screen
	// 	nextScreen = strData.charAt(1);

	// 	if (nextScreen == 'C')
	// 	{
	// 		//Change Code screen
	// 		lcdSendCommand("page 1");
	// 	}
	// 	else if (nextScreen == 'W')
	// 	{
	// 		// monitoringQueueAdd(CHANGE_WIFI);
	// 	}
	// 	else
	// 	{
	// 		if (doorIsOpen())
	// 		{
	// 			if (nextScreen == 'T')
	// 			{
	// 				lcdSendCommand("page Take_Med");
	// 			}
	// 			else if (nextScreen == 'A')
	// 			{
	// 				lcdSendCommand("page Add_Med");
	// 			}
	// 			else
	// 			{
	// 				Log.error("Unknown Command from Nextion");
	// 			}
	// 		}
	// 		else
	// 		{
	// 			//Door is closed
	// 			lcdSendCommand("page 1");
	// 		}
	// 	}
	// 	break;
	// }

	// case 'C':
	// {
	// 	/* code */
	// 	if (nextScreen == 'C')
	// 	{
	// 		//change code
	// 		if (intNewCode == 0)
	// 		{
	// 			lcdSendCommand("t5.txt=\"Enter your new Code and press 'OK'\"");
	// 			lcdSendCommand("t0.txt=\"\"");
	// 			intNewCode++;
	// 		}
	// 		else if (intNewCode == 1)
	// 		{
	// 			lcdSendCommand("t5.txt=\"Re-enter your new Code and press 'OK'\"");
	// 			lcdSendCommand("t0.txt=\"\"");
	// 			newCode = strData.substring(1);
	// 			Log.verbose("New Code %s" CR, newCode.c_str());
	// 			intNewCode++;
	// 		}
	// 		else if (intNewCode == 2)
	// 		{
	// 			Log.verbose("Pftt %s, %s" CR, newCode, strData.substring(1).c_str());
	// 			if (strData.substring(1).equals(newCode))
	// 			{
	// 				//code was changed
	// 				lcdSendCommand("page Code_Changed");
	// 				setLockCode(newCode);
	// 				intNewCode = 0;
	// 			}
	// 			else
	// 			{
	// 				Log.verbose("New Code %s" CR, strData.substring(1).c_str());
	// 				lcdSendCommand("page Code_Error");
	// 				// lcdSendCommand("t0.txt=\"Codes do not match!\"");
	// 			}
	// 		}
	// 	}
	// 	else if (strData.substring(1).equals(lockCode))
	// 	{
	// 		//Code is correct

	// 		if (nextScreen == 'A')
	// 		{
	// 			unlockDoor();
	// 			lcdSendCommand("page Add_Med");
	// 		}
	// 		else if (nextScreen == 'T')
	// 		{
	// 			unlockDoor();
	// 			lcdSendCommand("page Take_Med");
	// 		}
	// 	}
	// 	else
	// 	{

	// 		lcdSendCommand("t0.txt=\"\"");
	// 		lcdSendCommand("va1.val=0");
	// 		// lcdSendCommand("va0.txt=\"C" + (String)strData.charAt(1) + "\"");
	// 		lcdSendCommand("vis t1,1");
	// 	}
	// 	break;
	// }

	// case 'A':
	// {
	// 	Barcode = strData.substring(1, strData.indexOf(';') - 1);
	// 	expDate = (strData.substring(strData.indexOf(';') + 1));
	// 	// Log.verbose("Index %d" CR, strData.indexOf(';'));
	// 	// Log.verbose("exp Date %s" CR,strData.substring(strData.indexOf(';') + 1));
	// 	mqttStruct_t putMedicine;
	// 	putMedicine.msgType = PUT_MED; //msgType
	// 	// strcpy(putMedicine.barcode, Barcode.c_str()); //barcode
	// 	// strcpy(putMedicine.expDate, expDate.c_str()); //expDate
	// 	putMedicine.barcode = Barcode;
	// 	putMedicine.expDate = expDate;
	// 	xQueueSendToBack(mqttQ, &putMedicine, portMAX_DELAY);
	// 	break;
	// }

	// case 'T':
	// {
	// 	Barcode = strData.substring(1, strData.indexOf(';') - 1);
	// 	qty = (strData.substring(strData.indexOf(';') + 1)).toInt();
	// 	mqttStruct_t takeMedicine;
	// 	takeMedicine.msgType = TAKE_MED; //msgType
	// 	// strcpy(takeMedicine.barcode, Barcode.c_str()); //barcode
	// 	takeMedicine.barcode = Barcode;
	// 	takeMedicine.qty = qty;
	// 	xQueueSendToBack(mqttQ, &takeMedicine, portMAX_DELAY);
	// 	break;
	// }

	// case 'S':
	// {
	// 	//Smart Config Stop
	// 	boolSmartConfigStop = true;
	// 	break;
	// }
	// default:
	// {
	// 	break;
	// }
	// }
}

static void readLCDSerial(void *pvParameters)
{
	String tmpSerial;
	while (1)
	{
		if (LCD_SERIAL.available())
		{
			tmpSerial = LCD_SERIAL.readString();
			xQueueSendToBack(lcdUartRxQueue, &tmpSerial, portMAX_DELAY);
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
	xTaskCreate(lcd_uart_Queue_task, "lcd_uart_Queue_task", 2048, NULL, LCD_TRANSMIT_TASK_PRIORITY, &lcdUartRxHandle);
	xTaskCreate(lcd_uart_tx_task, "lcd_uart_tx_task", 2048, NULL, LCD_TRANSMIT_TASK_PRIORITY, &lcdUartTxHandle);
	EEPROM.begin(512);
	uint8_t needs_setup = EEPROM.read(SETUP_ADDRESS);
	lcdSendCommand("rest");
	vTaskDelay(pdMS_TO_TICKS(150));
	Log.verbose("Needs Setup %d" CR, needs_setup);
	if (needs_setup == 255)
	{
		//EEPROM is empty
		needs_setup = 1;
	}
	Log.verbose("Needs Setup %d" CR, needs_setup);

	lcdSendCommand("needs_setup.val=" + String(needs_setup));
	vTaskDelay(pdMS_TO_TICKS(350));
	lcdSendCommand("needs_setup.val=" + String(needs_setup));
	vTaskDelay(pdMS_TO_TICKS(150));
	if (!needs_setup)
	{
		//read the stored code in the EEPROM
		lockCode = readStringFromEEPROM(LOCK_CODE_ADDRESS);
		if (lockCode == "")
		{
			setLockCode("1234");
		}
		lcdSendCommand("lock_code.txt=\"" + lockCode + "\"");

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