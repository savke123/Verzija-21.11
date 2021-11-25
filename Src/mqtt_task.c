/*
 * mqtt_task.c
 *
 *  Created on: 2020. 5. 19.
 *      Author: eziya76@gmail.com
 */

#include "main.h"
#include "mqtt_task.h"
#include "RTC.h"
#include "string.h"
#include "MPPT.h"
#include "funkcije.h"
#include "adc.h"
#include "iwdg.h"

#define SERVER_ADDR		"a36r1007e087sh-ats.iot.eu-west-1.amazonaws.com" //replace your endpoint here
#define MQTT_PORT		"8883"
struct Prekidac sw1;
struct Prekidac sw2;
struct Prekidac sw3;
struct Prekidac sw4;
struct Prekidac sw5;
struct Prekidac sw6;
TaskHandle_t Task3BtnHandle=NULL;
uint8_t NOffUsb1;
uint8_t NOffUsb2;
uint8_t NOffWch1;
uint8_t NOffWch2;
uint8_t NOffCh=0;
uint32_t previousMillis = 0;
uint32_t currentMillis = 0;

void EXTI15_10_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI15_10_IRQn 0 */
  /* USER CODE END EXTI15_10_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
  /* USER CODE BEGIN EXTI15_10_IRQn 1 */

  BaseType_t checkIfYieldIsrequired;
  checkIfYieldIsrequired=xTaskResumeFromISR(Task3BtnHandle);
  portYIELD_FROM_ISR(checkIfYieldIsrequired);

  /* USER CODE END EXTI15_10_IRQn 1 */
}

//mqtt subscribe task
void MqttClientSubTask(void const *argument)
{
	while(1)
	{
		if(!mqttClient.isconnected)
		{
			//try to connect to the broker
			MQTTDisconnect(&mqttClient);
			MqttConnectBroker();
			osDelay(1000);
		}
		else
		{
			/* !!! Need to be fixed
			 * mbedtls_ssl_conf_read_timeout has problem with accurate timeout
			 */
			MQTTYield(&mqttClient, 1000);
		}
	}
}

//mqtt publish task
void MqttClientPubTask(void const *argument)
{
	MQTTMessage message;
	float temp;
	uint8_t tempint;
	uint16_t Vbat;
	volatile struct Vreme time;
	volatile struct Poruka PorMppt;
	uint8_t FlegLDRTime=0;
	uint16_t memory;
	volatile float perc;
	uint8_t percint;
	uint16_t vBatMax=13500;
	static char *str=NULL;

	while(1)
	{
		temp=getTemp();
		tempint=temp/1;

		memory=xPortGetFreeHeapSize();
		printf("Pre poruka %d:\n",memory);

		time=getTime();
		if((time.sat>=18 && time.sat<=23) || (time.sat>=0 && time.sat<1)) 	//izbacen uslov time.min==0 greska!
		{
			LedRingOn();
			FlegLDRTime=1;
		}else
		{
			LedRingOff();
			FlegLDRTime=0;
		}

		PorMppt=getMppt();
		Vbat=atoi(PorMppt.V);

		if(Vbat<12000)
		{
			if(Vbat<11300)
			{
				if(Vbat<10800)
				{
					ChargersOff();
				}
				//SensorsOff();
			}else
			{
				//SensorsOn();
			}
			LedRingOff();
		}else
		{
			if(FlegLDRTime==1)
			{
				LedRingOn();
			}
			//SensorsOn();
		}
		HAL_IWDG_Refresh(&hiwdg);
		perc=((float)(Vbat)/vBatMax)*100;
		if(perc>=100)
		{
			perc=100;
		}
		percint=perc/1;
		if(mqttClient.isconnected)
		{
			//char *str=pvPortMalloc(370*sizeof(char));			//160 kanc test
			str=(char*)pvPortMalloc(512*sizeof(char));
			sprintf((char*)str,"{\n\"TempCelsius\" : %d, \n "
					"\"MainSwCur\" : %d, \n "
					"\"RouterCur\" : %d, \n \"12VExtCur\" : %d, \n "
					"\"Usb1Cur\" : %d, \n \"Usb2Cur\" : %d, \n "
					"\"Wch1Cur\" : %d, \n \"Wch2Cur\" : %d, \n "
					"\"Ldr1Cur\" : %d, \n \"Ldr2Cur\" : %d, \n "
					"\"SensCur\" : %d, \n \"5VExtCur\" : %d, \n "
					"\"PanVol[mV]\" : %s, \n "
					"\"BatVol[mV]\" : %s, \n "
					"\"BatPerc[%%]\" : %d, \n "
					"\"LoadCur[mA]\" : %s, \n "
					"\"PanPow[W]\" : %s, \n "
					"\"MaxPowToday[W]\" : %s, \n "
					"\"YieldToday[kWh]\" : %s, \n "
					"\"MainBatCur[mA]\" : %s, \n "
					"\"NumOfChg\" : %d, \n "
					"\"Vreme\" : \"%d : %d : %d\" \n}", tempint,sw1.I1,sw2.I1,sw2.I2,sw3.I1,sw3.I2,sw4.I1,sw4.I2,sw5.I1,sw5.I2,sw6.I1,sw6.I2,PorMppt.Vpv,PorMppt.V,percint,PorMppt.Il,PorMppt.Ppv,PorMppt.H21,PorMppt.H19,PorMppt.I,NOffCh,time.sat,time.min,time.sec);

			message.payload = (void*)str;
			message.payloadlen = strlen(str);
			MQTTPublish(&mqttClient, "test", &message); //publish a message to "test" topic

			memory=xPortGetFreeHeapSize();
			printf("Posle poruka: %d\n",memory);

			vPortFree((void*)str);
		}

		memory=xPortGetFreeHeapSize();
		printf("Posle free-a: %d\n",memory);
		osDelay(10000);
	}
}

void StartT3Btn(void const * argument)
{
	for(;;)
	{
		vTaskSuspend(NULL);
		ChargersOn();
		SensorsOn();
		currentMillis=HAL_GetTick();
		if((currentMillis - previousMillis) > 200)
		{
			NOffCh++;
		}
		previousMillis=HAL_GetTick();
		NOffUsb1=0;
		NOffUsb2=0;
		NOffWch1=0;
		NOffWch2=0;
		if(NOffCh>1000)
		{
			NOffCh=0;
		}
	}
}
void StartT4ADC(void const * argument)
{
	uint8_t prekidac=1;
	uint8_t kanal=0;

	for(;;)
	{
		for(kanal=0;kanal<2;kanal++)
		{
			do
			{
				if(prekidac==1 && kanal==0)   ///U7 Glavni
				{
					init_adc1_ch10();
					HAL_GPIO_WritePin(SEL0_12VIN_GPIO_Port, SEL0_12VIN_Pin,RESET);
					HAL_GPIO_WritePin(SEL1_12VIN_GPIO_Port, SEL1_12VIN_Pin,RESET);
					sw1.I1=DoADCon1();
				}
///////////////////////////////////////////////
				if(prekidac==2 && kanal==0)   ////// U8, kanal 1 Router, kanal 2 Aux12V
				{
					init_adc3_ch9();
					HAL_GPIO_WritePin(SEL0_RA12_GPIO_Port, SEL0_RA12_Pin,RESET);
					HAL_GPIO_WritePin(SEL1_RA12_GPIO_Port, SEL1_RA12_Pin,RESET);
					sw2.I1=DoADCon3();
				}
				if(prekidac==2 && kanal==1)
				{
					init_adc3_ch9();
					HAL_GPIO_WritePin(SEL0_RA12_GPIO_Port, SEL0_RA12_Pin, SET);
					HAL_GPIO_WritePin(SEL1_RA12_GPIO_Port, SEL1_RA12_Pin, RESET);
					sw2.I2=DoADCon3();
				}
//////////////////////////////////////////////
				if(prekidac==3 && kanal==0) 	///////////U9 USB 12V
				{
					init_adc1_ch13();
					HAL_GPIO_WritePin(SEL0_USB_GPIO_Port, SEL0_USB_Pin, RESET);
					HAL_GPIO_WritePin(SEL1_USB_GPIO_Port, SEL1_USB_Pin, RESET);
					sw3.I1=DoADCon1();
					if(sw3.I1<300)
					{
						NOffUsb1++;
						if(NOffUsb1==3)
						{
							HAL_GPIO_WritePin(USB0_CTRL_GPIO_Port, USB0_CTRL_Pin, RESET);
							NOffUsb1=0;
						}
					}else
					{
						NOffUsb1=0;
					}
				}
				if(prekidac==3 && kanal==1)
				{
					init_adc1_ch13();
					HAL_GPIO_WritePin(SEL0_USB_GPIO_Port, SEL0_USB_Pin, SET);
					HAL_GPIO_WritePin(SEL1_USB_GPIO_Port, SEL1_USB_Pin, RESET);
					sw3.I2=DoADCon1();
					if(sw3.I2<300)
					{
						NOffUsb2++;
						if(NOffUsb2==3)
						{
							HAL_GPIO_WritePin(USB1_CTRL_GPIO_Port, USB1_CTRL_Pin, RESET);
							NOffUsb2=0;
						}
					}else
					{
						NOffUsb2=0;
					}
				}
/////////////////////////////////////////////
				if(prekidac==4 && kanal==0)  	///////////// wich u10 9V
				{
					init_adc1_ch12();
					HAL_GPIO_WritePin(SEL0_WICH_GPIO_Port, SEL0_WICH_Pin, RESET);
					HAL_GPIO_WritePin(SEL1_WICH_GPIO_Port, SEL1_WICH_Pin, RESET);
					sw4.I1=DoADCon1();
//					if(sw4.I1<150)																	// Zbog testiranja na trgu, koristimo sens5V i sensor izlaze, pa nam WIch nije potrbno
//					{
//						NOffWch1++;
//						if(NOffWch1==4)
//						{
//							HAL_GPIO_WritePin(WICH0_CTRL_GPIO_Port, WICH0_CTRL_Pin, 0);
//							NOffWch1=0;
//						}
//					}else
//					{
//						NOffWch1=0;
//					}
				}
				if(prekidac==4 && kanal==1)
				{
					init_adc1_ch12();
					HAL_GPIO_WritePin(SEL0_WICH_GPIO_Port, SEL0_WICH_Pin, SET);
					HAL_GPIO_WritePin(SEL1_WICH_GPIO_Port, SEL1_WICH_Pin, RESET);
					sw4.I2=DoADCon1();
//					if(sw4.I2<150)
//					{
//						NOffWch2++;
//						if(NOffWch2==4)
//						{
//							HAL_GPIO_WritePin(WICH1_CTRL_GPIO_Port, WICH1_CTRL_Pin, 0);
//							NOffWch2=0;
//						}
//					}else
//					{
//						NOffWch2=0;
//					}
				}
///////////////////////////////////////////
				if(prekidac==5 && kanal==0)  ////u11 ledringovi
				{
					init_adc3_ch14();
					HAL_GPIO_WritePin(SEL0_LL_GPIO_Port, SEL0_LL_Pin, RESET);
					HAL_GPIO_WritePin(SEL1_LL_GPIO_Port, SEL1_LL_Pin, RESET);
					sw5.I1=DoADCon3();
				}
				if(prekidac==5 && kanal==1)
				{
					init_adc3_ch14();
					HAL_GPIO_WritePin(SEL0_LL_GPIO_Port, SEL0_LL_Pin, SET);
					HAL_GPIO_WritePin(SEL1_LL_GPIO_Port, SEL1_LL_Pin, RESET);
					sw5.I2=DoADCon3();
				}
///////////////////////////////////////////
				if(prekidac==6 && kanal==0)		//////////u12 sensori i 5V aux
				{
					init_adc1_ch9();
					HAL_GPIO_WritePin(SEL0_SA5_GPIO_Port, SEL0_SA5_Pin, RESET);
					HAL_GPIO_WritePin(SEL1_SA5_GPIO_Port, SEL1_SA5_Pin, RESET);
					sw6.I1=DoADCon1();
					if(sw6.I1<150)
					{
						NOffWch1++;
						if(NOffWch1==4)
						{
							HAL_GPIO_WritePin(SENSORS_CTRL_GPIO_Port, SENSORS_CTRL_Pin, RESET);
							NOffWch1=0;
						}
					}else
					{
						NOffWch1=0;
					}
				}
				if(prekidac==6 && kanal==1)
				{
					init_adc1_ch9();
					HAL_GPIO_WritePin(SEL0_SA5_GPIO_Port, SEL0_SA5_Pin, SET);
					HAL_GPIO_WritePin(SEL1_SA5_GPIO_Port, SEL1_SA5_Pin, RESET);
					sw6.I2=DoADCon1();
					if(sw6.I2<150)
					{
						NOffWch2++;
						if(NOffWch2==4)
						{
							HAL_GPIO_WritePin(AUX_5V_CTRL_GPIO_Port, AUX_5V_CTRL_Pin, RESET);
							NOffWch2=0;
						}
					}else
					{
						NOffWch2=0;
					}
				}
				prekidac++;
			}while(prekidac<=6);
			prekidac=1;
		}
		HAL_IWDG_Refresh(&hiwdg);
		osDelay(5000);
	}
}

int MqttConnectBroker()
{
	int ret;
	TaskHandle_t Task4ADCHandle=NULL;

	net_clear();
	ret = net_init(&net, SERVER_ADDR);
	if(ret != MQTT_SUCCESS)
	{
		printf("net_init failed.\n");
		return -1;
	}

	ret = net_connect(&net, SERVER_ADDR, MQTT_PORT);
	if(ret != MQTT_SUCCESS)
	{
		printf("net_connect failed.\n");
		return -1;
	}

	MQTTClientInit(&mqttClient, &net, 1000, sndBuffer, sizeof(sndBuffer), rcvBuffer, sizeof(rcvBuffer));

	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	data.willFlag = 0;
	data.MQTTVersion = 3;
	data.clientID.cstring = ""; //no client id required
	data.username.cstring = ""; //no user name required
	data.password.cstring = ""; //no password required
	data.keepAliveInterval = 60;
	data.cleansession = 1;

	ret = MQTTConnect(&mqttClient, &data);
	if(ret != MQTT_SUCCESS)
	{
		printf("MQTTConnect failed.\n");
		return ret;
	}

	ret = MQTTSubscribe(&mqttClient, "test", QOS0, MqttMessageArrived);
	if(ret != MQTT_SUCCESS)
	{
		printf("MQTTSubscribe failed.\n");
		return ret;
	}

	printf("MqttConnectBroker O.K.\n");
	xTaskCreate(StartT4ADC, "Task4ADC", 1024, (void*)0, osPriorityNormal, &Task4ADCHandle);
	xTaskCreate(StartT3Btn, "Task3Btn", 165, (void*)0, osPriorityNormal, &Task3BtnHandle);
	HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, 0);
	HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 0);
	HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, 0);

	return MQTT_SUCCESS;
}

void MqttMessageArrived(MessageData* msg)
{
	HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin); //toggle pin when new message arrived

	MQTTMessage* message = msg->message;
	memset(msgBuffer, 0, sizeof(msgBuffer));
	memcpy(msgBuffer, message->payload,message->payloadlen);

	printf("MQTT MSG[%d]:%s\n", (int)message->payloadlen, msgBuffer);
}
