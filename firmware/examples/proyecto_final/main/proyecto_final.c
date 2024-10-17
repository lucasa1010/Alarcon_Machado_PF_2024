/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Lucas Alarcon (lucasalarcon872@gmail.com)
 * @author Joaquin Machado (joaquin.machado@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "hc_sr04.h"
#include "lcditse0803.h"
#include "switch.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
/*==================[macros and definitions]=================================*/
#define TIEMPO_REFRESCO_PANTALLA 50000 // VER TIEMPOS
#define TIEMPO_MEDICION 50000

TaskHandle_t interfaz_task_handle = NULL;
TaskHandle_t controlador_task_handle = NULL;
/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/
static void manejoDeLEDs(){

}
static void manejoDeBuzzers(){

}
static void medir(){
	
}

void funcTimerInterfaz(){
    vTaskNotifyGiveFromISR(interfaz_task_handle, pdFALSE);
}

void funcTimerControlador(){
    vTaskNotifyGiveFromISR(controlador_task_handle, pdFALSE);
}

static void controlar(void *pvParameter){
    while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		
		medir();
		manejoDeLEDs();
		manejoDeBuzzers();
	}
}

static void manejarInterfaz(void *pvParameter){
    while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	}
}

/*==================[external functions definition]==========================*/
void app_main(void){

	// Inicialización de timers 
    timer_config_t timer_controlador = {
        .timer = TIMER_A,
        .period = TIEMPO_MEDICION,
        .func_p = funcTimerControlador,
        .param_p = NULL
    };
    TimerInit(&timer_controlador);

    timer_config_t timer_interfaz = {
        .timer = TIMER_B,
        .period = TIEMPO_REFRESCO_PANTALLA,
        .func_p = funcTimerInterfaz,
        .param_p = NULL
    };
    TimerInit(&timer_interfaz);

    // Creacion de tareas
    xTaskCreate(&controlar, "Controlador", 2048, NULL, 5, &controlador_task_handle);
    xTaskCreate(&manejarInterfaz, "Interfaz", 512, NULL, 5, &interfaz_task_handle);

    // Inicio de los timers
    TimerStart(timer_interfaz.timer);
    TimerStart(timer_controlador.timer);
}
/*==================[end of file]============================================*/