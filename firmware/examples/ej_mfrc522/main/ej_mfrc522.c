/*! @mainpage Ejemplo MFRC522
 *
 * \section genDesc General Description
 *
 * Este proyecto ejemplifica el uso del dispositivo MFRC522.
 *
 * \section hardConn Hardware Connection
 *
 * |   	MRFC522		|   ESP-EDU		|
 * |:--------------:|:--------------|
 * | 	SDO/MISO 	|	GPIO_22		|
 * | 	3V3		 	| 	3V3			|
 * | 	SCK		 	| 	GPIO_20		|
 * | 	SDI/MOSI 	| 	GPIO_21		|
 * | 	RESET	 	| 	GPIO_18		|
 * | 	CS		 	| 	GPIO_9		|
 * | 	GND		 	| 	GND			|
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 17/05/2024 | Document creation		                         |
 *
 * @author Juan Ignacio Cerrudo (juan.cerrudo@uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "uart_mcu.h"
#include "rfid_utils.h"
#include "gpio_mcu.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD 1000
/*==================[internal data definition]===============================*/
unsigned int last_user_ID;
// RFID structs
MFRC522Ptr_t mfrcInstance;
/*==================[internal functions declaration]=========================*/
/**
 * Executed every time the card reader detects a user in
 */
void userTapIn() {

//	show card UID
	UartSendString(UART_PC,"\nCard uid bytes: ");
	for (uint8_t i = 0; i < mfrcInstance->uid.size; i++) {
		UartSendString(UART_PC," 0X");
		UartSendString(UART_PC, (char*)UartItoa(mfrcInstance->uid.uidByte[i], 16));
		UartSendString(UART_PC," ");
	}
	UartSendString(UART_PC,"\n\r");
	// Convert the uid bytes to an integer, byte[0] is the MSB
	last_user_ID =
		(int)mfrcInstance->uid.uidByte[3] |
		(int)mfrcInstance->uid.uidByte[2] << 8 |
		(int)mfrcInstance->uid.uidByte[1] << 16 |
		(int)mfrcInstance->uid.uidByte[0] << 24;

	UartSendString(UART_PC,"Card Read user ID: ");
	UartSendString(UART_PC, (char*)UartItoa(last_user_ID, 10));
	UartSendString(UART_PC,"\n\r");


}
bool IR = false;

void cambioEstado_IR(){
	IR =! IR;
}

/*==================[external functions definition]==========================*/
void app_main(void){
	
	GPIOInit(GPIO_22, 0); // IN 0 OUT 1
	GPIOActivInt(GPIO_22, *cambioEstado_IR, false, NULL);

	while(1){
		printf("&d/r/n", IR);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
/*==================[end of file]============================================*/
