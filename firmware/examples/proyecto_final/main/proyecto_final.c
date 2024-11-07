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
#include "neopixel_stripe.h"
//#include "buzzer.h"
//#include "buzzer_melodies.h"
#include "time.h"
#include "stdio.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
/*==================[macros and definitions]=================================*/
#define TIEMPO_REFRESCO_PANTALLA 1 // VER TIEMPOS
#define TIEMPO_MEDICION 1000 // 1 ms de refresco 
#define GPIO_LEDS GPIO_9 // maneja los leds
#define GPIO_IR GPIO_22 // Lee la tension de los IR
TaskHandle_t interfaz_task_handle = NULL;
TaskHandle_t controlador_task_handle = NULL;
bool IR = false; //analiza si se dispara el evento
uint16_t voltaje = 0; //voltaje asociado al sensor que se midio
#define SENSORR 0.78   //estan por orden de linea es decir el rojo es el primero y asi
#define SENSORA 2.35
#define SENSORV 2.75
#define SENSORN 1.36 // tension asociada a cada sensor en particular
#define CANTIDADSENSORES 4
bool medidaAcertada = true; 
neopixel_color_t cantidadLeds [CANTIDADSENSORES*4];
u_int16_t tiempoInicio = 0;
u_int16_t tiempoMedido = 0;
int sensor = 0;
u_int16_t sensorPrendido = 0;
/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/
void funcTimerInterfaz(){
    vTaskNotifyGiveFromISR(interfaz_task_handle, pdFALSE);
}

void funcTimerControlador(){
    vTaskNotifyGiveFromISR(controlador_task_handle, pdFALSE);
}

static void manejoDeLEDsyBuzzers(){
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    sensor = rand() % (CANTIDADSENSORES); //Obtengo el sensor a encender
    for(int i = sensor*4; i<(sensor*4+4); i++){
        if(sensor==0){
            NeoPixelSetPixel(i, NEOPIXEL_COLOR_RED); // prende sensor en un color
        }
        if(sensor==1){
            NeoPixelSetPixel(i, NEOPIXEL_COLOR_BLUE); 
        }
        if(sensor==2){
            NeoPixelSetPixel(i, NEOPIXEL_COLOR_GREEN);
        }
        if(sensor==3){
            NeoPixelSetPixel(i, NEOPIXEL_COLOR_ORANGE);
        }
    }
    //BuzzerOn(); // prendo el buzzer
}


static void cambioEstado_IR(){
    IR =! IR;
}

void obtenerSensorPrendido(){    //Asigna la tension del bloque activado a la variable sensorPrendido
    switch (sensor){
    case 0: sensorPrendido = SENSORR; break;
    case 1: sensorPrendido = SENSORA; break;
    case 2: sensorPrendido = SENSORV; break;
    case 3: sensorPrendido = SENSORN; break;
    }
}

void apagarLeds(){
    for(int i = 0 ; i<CANTIDADSENSORES*4; i++){
        cantidadLeds[i]=0;
    }
}

static void medir(){
    tiempoMedido = ( ((u_int16_t) clock()) / CLOCKS_PER_SEC) - tiempoInicio; //calcula el tiempo
// ver donde que se cambia medida acertiva
}

static void controlar(void *pvParameter){
    while(1){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if(medidaAcertada == true){ // Si selecciona el sensor correcto
            manejoDeLEDsyBuzzers();
            obtenerSensorPrendido();
            tiempoInicio = (u_int16_t) clock(); //Incia el tiempo 
            tiempoInicio = tiempoInicio / CLOCKS_PER_SEC;  //Se pasa a segundos
        } 
        if (IR == true){ //Asociarlo a niveles de tension
            AnalogInputReadSingle(GPIO_IR, &voltaje);   //se obtiene el voltaje en el sensor 
            printf("%d\n", voltaje);
            if(voltaje == sensorPrendido){
                medir(); //obtiene la diferencia de tiempos
                cambioEstado_IR(); //cambia el estado del infrarojo
                apagarLeds();
                //BuzzerOff(); //Apago el BUZZER
                medidaAcertada = true;
            }
            cambioEstado_IR(); //cambia el estado del infrarojo
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
    
    // Inicializacion de los perifericos
    NeoPixelInit(GPIO_LEDS, CANTIDADSENSORES*4, cantidadLeds);
    //BuzzerInit(GPIO_IR);
    //BuzzerSetFrec(NOTE_C3); // se setea el tono con el que suena el buzzer
	// analog_input_config_t conversorAD = {
    //     .input = CH1,
    //     .mode = ADC_SINGLE,
    // };
    // AnalogInputInit(&conversorAD);
    // AnalogOutputInit();
	// Inicialización de timers 
    // timer_config_t timer_controlador = {
    //     .timer = TIMER_A,
    //     .period = TIEMPO_MEDICION,
    //     .func_p = funcTimerControlador,
    //     .param_p = NULL
    // };
    // TimerInit(&timer_controlador);

    // timer_config_t timer_interfaz = {
    //     .timer = TIMER_B,
    //     .period = TIEMPO_REFRESCO_PANTALLA,
    //     .func_p = funcTimerInterfaz,
    //     .param_p = NULL
    // };
    // TimerInit(&timer_interfaz);

    // Creacion de tareas
    //xTaskCreate(&controlar, "Controlador", 2048, NULL, 5, &controlador_task_handle);
    //xTaskCreate(&manejarInterfaz, "Interfaz", 512, NULL, 5, &interfaz_task_handle);

    // xTaskCreate(&manejoDeLEDsyBuzzers, "manejoDeLEDsyBuzzers", 512, NULL, 5, &interfaz_task_handle);


    //GPIOActivInt(GPIO_IR, *cambioEstado_IR, false, NULL); //lanza el evento de que el IR midio

    // Inicio de los timers
    // TimerStart(timer_interfaz.timer);
    //TimerStart(timer_controlador.timer);
serial_config_t my_uart = {
    .port = UART_PC,
    .baud_rate = 57600,
    .func_p = NULL,
    .param_p = NULL
    };
    UartInit(&my_uart);

    while (1){
        GPIOActivInt(GPIO_IR, *cambioEstado_IR, false, NULL); //lanza el evento de que el IR midio
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
/*==================[end of file]============================================*/