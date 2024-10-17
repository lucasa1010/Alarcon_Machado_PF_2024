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
#include "stdlib.h"

/*==================[macros and definitions]=================================*/
#define TIEMPO_REFRESCO_PANTALLA 50000 // VER TIEMPOS
#define TIEMPO_MEDICION 1000 // 1 ms de refresco 
#define GPIO_IR GPIO_18
TaskHandle_t interfaz_task_handle = NULL;
TaskHandle_t controlador_task_handle = NULL;
bool IR = false; //analiza si se dispara el evento
uint16_t voltaje = 0; //voltaje asociado al sensor que se midio
#define SENSOR1 1
#define SENSOR2 2
#define SENSOR3 3
#define SENSOR4 4
#define CANTIDADSENSORES 4
bool medidaAcertada = true; 
/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/
void funcTimerInterfaz(){
    vTaskNotifyGiveFromISR(interfaz_task_handle, pdFALSE);
}

void funcTimerControlador(){
    vTaskNotifyGiveFromISR(controlador_task_handle, pdFALSE);
}

static void manejoDeLEDsyBuzzers(){
    int sensor = rand() % (CANTIDADSENSORES+1); //Obtengo el sensor a encender
    NeoPixelSetPixel(uint16_t pixel, neopixel_color_t color); // prende sensor en un color

}


static void cambioEstado_IR(){
    IR =! IR;
}



static void medir(){

}

static void controlar(void *pvParameter){
    while(1){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if(medidaAcertada == true){
            manejoDeLEDsyBuzzers();
        } 
        if (IR == true){
            medir();
            cambioEstado_IR();
        }
    }
}

static void manejarInterfaz(void *pvParameter){
    while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	}
}

/*==================[external functions definition]==========================*/
void app_main(void){

    NeoPixelInit(gpio_t pin, uint16_t len, neopixel_color_t *color_array);
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

    GPIOActivInt(GPIO_IR, *cambioEstado_IR, false, NULL); //lanza el evento de que el IR midio

    // Inicio de los timers
    TimerStart(timer_interfaz.timer);
    TimerStart(timer_controlador.timer);
}
/*==================[end of file]============================================*/