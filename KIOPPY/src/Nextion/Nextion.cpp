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

#define NEXTION_NEW_PASSCODE 'N'
#define NEXTION_UNLOCK 'U'
#define NEXTION_REMOVE_MEDICINE 'R'
#define NEXTION_ADD_MEDICINE 'A'
#define NEXTION_TAKE_MEDICINE 'T'
#define NEXTION_SCAN_MEDICINE 'S'
#define NEXTION_NEW_MEDICINE 'X'
#define NEXTION_DELETE_MEDICINE 'D'
#define NEXTION_WIFI_SSID 'W'
#define NEXTION_WIFI_PWD 'P'
#define NEXTION_FAC_RST 'F'
#define NEXTION_BLWR 'B'
#define NEXTION_ENABLE 'E'
#define NEXTION_CNCL 'C'

QueueHandle_t lcdUartRxQueue;
QueueHandle_t lcdUartTxQueue;

TaskHandle_t lcdUartRxHandle;
TaskHandle_t lcdUartTxHandle;

String lockCode;
ScannedMed scannedMed = NO_MED_SCANNED;
uint8_t medID = 0;
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

			if (strData.charAt(0) == NEXTION_NEW_PASSCODE)
			{
				// New PassCode was set

				newCode = strData.substring(1);
				setLockCode(newCode);
				nvsSaveU8(nvsConf.needs_setup.key, 0);

				Log.verbose("New Code set to %s" CR, newCode.c_str());
			}
			else if (strData.charAt(0) == NEXTION_UNLOCK)
			{
				// unlock the door
				Log.verbose("Unlock Door" CR);
				unlockDoor();
			}
			else if (strData.charAt(0) == NEXTION_REMOVE_MEDICINE)
			{
				//Remove medicine from the cabinet
				Barcode = strData.substring(1, strData.length() - 1);
				mqttStruct_t removeMedicine;
				removeMedicine.msgType = REMOVE_MED;
				removeMedicine.barcode = Barcode;
				Log.verbose("Barcode %s" CR, removeMedicine.barcode.c_str());
				// xQueueSendToBack(mqttQ, &removeMedicine, pdMS_TO_TICKS(500));
				mqttSendMessage(removeMedicine);
				allMedicines->listMedicines();
				//TODO: do we need a new screen after removing?
			}
			else if (strData.charAt(0) == NEXTION_ADD_MEDICINE)
			{

				allMedicines->allMedList[medID].setQty(allMedicines->allMedList[medID].getQty() + allMedicines->allMedList[medID].getInitialQty(), 1);
				//Add medicine to the cabinet
				expDate = strData.substring(1);
				mqttStruct_t addMed;
				addMed.msgType = PUT_MED;
				addMed.barcode = Barcode;
				addMed.expDate = expDate;
				// xQueueSendToBack(mqttQ, &addMed, pdMS_TO_TICKS(500));
				mqttSendMessage(addMed);
				allMedicines->listMedicines();
			}
			else if (strData.charAt(0) == NEXTION_TAKE_MEDICINE)
			{
				// Take medicine from the cabinet
				//Barcode already sent
				float tmp = strData.substring(1).toFloat();

				allMedicines->allMedList[medID].setQty(allMedicines->allMedList[medID].getQty() - tmp, 1);
				// qty = strData.substring(1);
				tmp = tmp / 100;
				mqttStruct_t takeMed;
				takeMed.msgType = TAKE_MED;
				takeMed.barcode = Barcode;
				takeMed.qty = tmp;

				// xQueueSendToBack(mqttQ, &takeMed, pdMS_TO_TICKS(500));
				mqttSendMessage(takeMed);
				allMedicines->listMedicines();
			}
			else if (strData.charAt(0) == NEXTION_SCAN_MEDICINE)
			{
				cancelScan = false;

				//Medicine Scanned, waiting for name from Mqtt
				uint8_t i = 0;
				bool medFound = false;
				Barcode = strData.substring(2, strData.length() - 1);
				for (i = 0; i < allMedicines->getCounter(); ++i)
				{
					if (strcmp(allMedicines->allMedList[i].getBarcode(), (char *)Barcode.c_str()) == 0)
					{
						Log.verbose("Med Found" CR);
						medID = i;
						medFound = true;
						break;
					}
				}
				if (medFound == true)
				{
					if (strData.charAt(1) == NEXTION_ADD_MEDICINE)
					{
						//Add Med
							allMedicines->listMedicines();
						scannedMed = PUT_MED_SCANNED;
						lcdSendCommand("ExpDate.t_name.txt=\"" + (String)allMedicines->allMedList[i].getDescription() + "\"");
						lcdSendCommand("page ExpDate");
					}
					else if (strData.charAt(1) == NEXTION_TAKE_MEDICINE)
					{
						//Take Med
							allMedicines->listMedicines();
						scannedMed = TAKE_MED_SCANNED;
						lcdSendCommand("TakeMed.t_barcode.txt=\"" + (String)Barcode + "\"");
						if (allMedicines->allMedList[i].getType() == PILLS)
						{
							lcdSendCommand("TakeMed.v_type.val=0");
						}
						else
						{
							lcdSendCommand("TakeMed.v_type.val=1");
						}
						lcdSendCommand("TakeMed.v_name.txt=\"" + (String)allMedicines->allMedList[i].getDescription() + "\"");
						//  vTaskDelay(pdMS_TO_TICKS(50));
						lcdSendCommand("page TakeMed");
						
					}
					else if (strData.charAt(1) == NEXTION_REMOVE_MEDICINE)
					{
							allMedicines->listMedicines();
						//Remove Medicine
						lcdSendCommand("ConfMed.t_name.txt=\"" + (String)allMedicines->allMedList[i].getDescription() + "\"");
						lcdSendCommand("page ConfMed");
						allMedicines->allMedList[i].setQty(0, 1);
						//Remove medicine from the cabinet

						mqttStruct_t removeMedicine;
						removeMedicine.msgType = REMOVE_MED;
						removeMedicine.barcode = Barcode;
						Log.verbose("Barcode %s" CR, removeMedicine.barcode.c_str());
						// xQueueSendToBack(mqttQ, &removeMedicine, pdMS_TO_TICKS(500));
						mqttSendMessage(removeMedicine);
							allMedicines->listMedicines();
					}
						else if (strData.charAt(1) == NEXTION_DELETE_MEDICINE)
					{
						allMedicines->listMedicines();
						//Remove Medicine
						lcdSendCommand("ConfMed.t_name.txt=\"" + (String)allMedicines->allMedList[i].getDescription() + "\"");
						lcdSendCommand("page ConfMed");
						// allMedicines->allMedList[i].setQty(0, 1);
						allMedicines->deleteMedicine(i);
						//Remove medicine from the cabinet

						mqttStruct_t deleteMed;
						deleteMed.msgType = DELETE_MED;
						deleteMed.barcode = Barcode;
						Log.verbose("Barcode %s" CR, deleteMed.barcode.c_str());
						// xQueueSendToBack(mqttQ, &deleteMed, pdMS_TO_TICKS(500));
						mqttSendMessage(deleteMed);
							allMedicines->listMedicines();
					}
					//Medicine Scanned, waiting for name from
				}

				else
				{
					//Medicine not found
					
					lcdSendCommand("page MedNotFound");
				}

				// mqttStruct_t getMedName;
				// getMedName.msgType = SCAN_MED;
				// getMedName.barcode = Barcode;
				// xQueueSendToBack(mqttQ, &getMedName, pdMS_TO_TICKS(500));
			}

			else if (strData.charAt(0) == NEXTION_WIFI_SSID)
			{
				//SSID
				// wifiSSID = strData.substring(1).c_str();
				if (strData.length() == 1)
				{
					scanWiFi();
				}
				else
				{
					strcpy(nvsConf.wifiSSID.value, strData.substring(1).c_str());
				}
			}
			else if (strData.charAt(0) == NEXTION_WIFI_PWD)
			{
				//Wifi Password
				// wifiPassword = strData.substring(1);
				strcpy(nvsConf.wifiPassword.value, strData.substring(1).c_str());

				nvsSaveStr(nvsConf.wifiSSID.key, nvsConf.wifiSSID.value);
				nvsSaveStr(nvsConf.wifiPassword.key, nvsConf.wifiPassword.value);

				// monitoringQueueAdd(WIFI_RECOONECT_EVT);
				monitoringQueueAdd(WIFI_BLOCKING_CONNECT_EVT);
				// WiFi.begin(SSID.c_str(), PWD.c_str());
			}
			else if (strData.charAt(0) == NEXTION_FAC_RST)
			{
				//Factory Reset
				nvsSaveU8(nvsConf.blwr.key, 1);
				nvsSaveU8(nvsConf.needs_setup.key, 1);
				nvsSaveStr(nvsConf.wifiSSID.key, "");
				nvsSaveStr(nvsConf.wifiPassword.key, "");
				nvsSaveU8(nvsConf.medCnt.key, 0);

				ESP.restart();
			}
			else if (strData.charAt(0) == NEXTION_BLWR)
			{
				//Blower
				if (strData.charAt(1) == NEXTION_ENABLE)
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
			else if (strData.charAt(0) == NEXTION_CNCL)
			{
				//Cancel Scan
				cancelScan = true;
			}
			else if (strData.charAt(0) == NEXTION_NEW_MEDICINE)
			{
				//New Medicine
				String tmp;
				MedicineParams_t med = {};
				strcpy(med.barcode, Barcode.c_str());
				strcpy(med.description, strData.substring(12).c_str());
				tmp = strData.substring(2, 4).c_str();
				med.qty = tmp.toInt()*100;
				if (strData.charAt(1)=='0'){
					Log.verbose("type 0");
					med.type = 0;
				} else {
					Log.verbose("type 1");
					med.type=1;
				}
				strcpy(med.expDate, strData.substring(4, 12).c_str());

				Log.verbose("Adding Medicines" CR);
				Log.verbose("-->Barcode: %s" CR, med.barcode);
				Log.verbose("-->description: %s" CR, med.description);
				Log.verbose("-->expDate: %s" CR, med.expDate);
				Log.verbose("-->Qty: %d" CR, med.qty);
				Log.verbose("-->Type: %d" CR, med.type);
				allMedicines->createNewMedicine(&med);
				allMedicines->listMedicines();
				mqttStruct_t notFoundMed;
				notFoundMed.msgType = NEW_MED;
				notFoundMed.barcode = Barcode;
				if (strData.charAt(1) == '0')
				{
					notFoundMed.form = "pills";
				}
				else
				{
					notFoundMed.form = "syrup";
				}
				notFoundMed.expDate = med.expDate;
				notFoundMed.name = med.description;
				notFoundMed.qty=med.qty;
				xQueueSendToBack(mqttQ, &notFoundMed, pdMS_TO_TICKS(500));
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