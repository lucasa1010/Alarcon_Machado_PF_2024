 /*! @mainpage Ejemplo Bluetooth - FFT
 *
 * @section genDesc General Description
 *
 * Este proyecto ejemplifica el uso del módulo de comunicación 
 * Bluetooth Low Energy (BLE), junto con el de cálculo de la FFT 
 * de una señal.
 * Permite graficar en una aplicación móvil la FFT de una señal. 
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 02/04/2024 | Document creation		                         |
 *
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "neopixel_stripe.h"
#include "ble_mcu.h"
#include "delay_mcu.h"
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
#include "buzzer.h"
#include "buzzer_melodies.h"
#include "time.h"
#include "stdio.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
/*==================[macros and definitions]=================================*/
#define TIEMPO_REFRESCO_PANTALLA 1 // VER TIEMPOS
#define TIEMPO_MEDICION 1000 // antes estaba en 100000
#define TIEMPO_uS 1000       // cada cuanto cuenta el clock en us
#define GPIO_LEDS GPIO_9
#define GPIO_IR GPIO_22     // LED de aviso de sensado IR
#define GPIO_BUZZER GPIO_20
#define SENSORR  785  // 0.78 V
#define SENSORN  2353 // 2.35
#define SENSORV  2753 // 2.75
#define SENSORA  1360 // 1.36
#define CANTIDADSENSORES 4
/*==================[internal data definition]===============================*/
TaskHandle_t interfaz_task_handle = NULL;
TaskHandle_t clock_task_handle = NULL;
TaskHandle_t controlador_task_handle = NULL;
bool IR = false; //analiza si se dispara el evento
uint16_t voltaje = 0; //voltaje asociado al sensor que se midio
bool medidaAcertada = true; // Indica si el sensor seleccionado fue el correcto
neopixel_color_t cantidadLeds [CANTIDADSENSORES*4];
clock_t tiempoInicio = 0;
clock_t tiempoFinal = 0;
float tiempoMedido = 0;
int sensor = -1;
u_int16_t sensorPrendido = 0;
int sensor_anterior = -1;
uint64_t conteo_en_ms = 0;
float conteo_en_seg;
bool pierde = false;
/*==================[internal functions declaration]=========================*/
// void funcTimerInterfaz(){
//     vTaskNotifyGiveFromISR(clock_task_handle, pdFALSE);
// }

void funcTimerControlador(){
    vTaskNotifyGiveFromISR(controlador_task_handle, pdFALSE);
}

void funcTimerTiempo(){
    conteo_en_ms++;

    if (conteo_en_ms > 6000)
        pierde = true;
}

static void manejoDeLEDsyBuzzers(){
    
    while (sensor == sensor_anterior){
        sensor = rand() % (CANTIDADSENSORES); //Obtengo el sensor a encender
    }

    for(int i = sensor*4; i<(sensor*4+4); i++){
        if(sensor == 0){
            NeoPixelSetPixel(i, NEOPIXEL_COLOR_RED); // prende sensor en un color
        }
        if(sensor == 1){
            NeoPixelSetPixel(i, NEOPIXEL_COLOR_BLUE); 
        }
        if(sensor == 2){
            NeoPixelSetPixel(i, NEOPIXEL_COLOR_GREEN);
        }
        if(sensor == 3){
            NeoPixelSetPixel(i, NEOPIXEL_COLOR_ORANGE);
        }
    }
    sensor_anterior = sensor;
   
    //BuzzerOn(); // prendo el buzzer
}


static void cambioIRaActivo(){
    IR = true;
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

static void controlar(void *pvParameter){
    while(1){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (pierde){
            NeoPixelAllColor(NEOPIXEL_COLOR_RED);
            GPIOOn(GPIO_BUZZER);

            vTaskDelay(3000 / portTICK_PERIOD_MS);

            GPIOOff(GPIO_BUZZER);
            apagarLeds();
            conteo_en_ms = 0;
            medidaAcertada = true;
            pierde = false;
        }

        if(medidaAcertada == true){ // Si selecciona el sensor correcto
            manejoDeLEDsyBuzzers();
            obtenerSensorPrendido();
            medidaAcertada = false;
        }

        if (IR == true){ //Asociarlo a niveles de tension
            AnalogInputReadSingle(CH1, &voltaje);   //se obtiene el voltaje en el sensor 

            if(voltaje > (sensorPrendido-100) && voltaje < (sensorPrendido+100)){
                apagarLeds();
                medidaAcertada = true;
                conteo_en_seg= (float)conteo_en_ms/1000;
                printf("Tiempo medido: %.2f seg\r\n", conteo_en_seg);
                conteo_en_ms=0;
            }

            IR = false;
        }

    }
}

/*==================[external functions definition]==========================*/
void app_main(void){
    
    // Inicializacion de los perifericos
    NeoPixelInit(GPIO_LEDS, CANTIDADSENSORES*4, cantidadLeds);
    GPIOActivInt(GPIO_IR, *cambioIRaActivo, true, NULL); //lanza el evento de que el IR midio
    LedsInit();
    GPIOInit(GPIO_BUZZER, GPIO_OUTPUT);

    // Configuracion de ADC
	analog_input_config_t analogIn = {
		.input = CH1,
		.mode = ADC_SINGLE
	};
	AnalogInputInit(&analogIn); 	// Inicializacion ADC

	// Inicialización de timers 
    timer_config_t timer_controlador = {
        .timer = TIMER_A,
        .period = TIEMPO_MEDICION,
        .func_p = funcTimerControlador,
        .param_p = NULL
    };
    TimerInit(&timer_controlador);

    timer_config_t timer_tiempo = {
        .timer = TIMER_B,
        .period = TIEMPO_uS,
        .func_p = funcTimerTiempo,
        .param_p = NULL
    };
    TimerInit(&timer_tiempo);

    /*timer_config_t timer_interfaz = {
        .timer = TIMER_B,
        .period = TIEMPO_REFRESCO_PANTALLA,
        .func_p = funcTimerInterfaz,
        .param_p = NULL
    };
    TimerInit(&timer_interfaz);*/

    // Creacion de tareas
    xTaskCreate(&controlar, "Controlador", 4096, NULL, 5, &controlador_task_handle);
    //xTaskCreate(&manejarInterfaz, "Interfaz", 512, NULL, 5, &interfaz_task_handle);

    // Inicio de los timers
    //TimerStart(timer_interfaz.timer);
    TimerStart(timer_controlador.timer);
    TimerStart(timer_tiempo.timer);

}
/*==================[end of file]============================================*/