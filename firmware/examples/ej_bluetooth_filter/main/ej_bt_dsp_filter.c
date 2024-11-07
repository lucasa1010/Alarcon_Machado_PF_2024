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
//#include "buzzer.h"
//#include "buzzer_melodies.h"
#include "time.h"
#include "stdio.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
/*==================[macros and definitions]=================================*/

#define TIEMPO_REFRESCO_PANTALLA 1 // VER TIEMPOS
#define TIEMPO_MEDICION 1000 // antes estaba en 100000
#define GPIO_LEDS GPIO_9
#define GPIO_IR GPIO_22
#define SENSORR  785  // 0.78 V
#define SENSORN  2353 // 2.35
#define SENSORV  2753 // 2.75
#define SENSORA  1360 // 1.36
#define CANTIDADSENSORES 4
#define LED_BT LED_2

/*==================[internal data definition]===============================*/

TaskHandle_t interfaz_task_handle = NULL;
TaskHandle_t controlador_task_handle = NULL;
bool IR = false;    //analiza si se dispara el evento
uint16_t voltaje = 0; //voltaje asociado al sensor que se midio
bool medidaAcertada = false;     // Inidca si el sensor seleccionado fue el correcto
bool medidaIncorrecta = false;
neopixel_color_t cantidadLeds [CANTIDADSENSORES*4];     // Cantidad de LEDs
clock_t tiempoInicio = 0;   // Tiempo 0 en que se prende el sensor
clock_t tiempoFinal = 0;    // Tiempo final en el que se selecciona el sensor correcto
u_int16_t tiempoMedido = 0; // TimepoFinal - TiempInicio
int sensor = -1;
u_int16_t sensorPrendido = 0;   
int sensor_anterior = -1;
bool inicio = true;
uint8_t vida;
uint8_t progreso;

/*==================[internal functions declaration]=========================*/

void funcTimerInterfaz(){
    vTaskNotifyGiveFromISR(interfaz_task_handle, pdFALSE);
}

void funcTimerControlador(){
    vTaskNotifyGiveFromISR(controlador_task_handle, pdFALSE);
}

void setBarras(){   // Actualiza las barras a las condiciones de inicio
    char msg[10];
    sprintf(msg, "*P%d", 0); // Actualiza la barra de progreso
    BleSendString(msg);

    printf(msg, "*V%d", 100);     // Actualiza la barra de vida
    BleSendString(msg);
}

void read_data(uint8_t * data, uint8_t length){  
    if(data[0] == 'G'){ // Interrupcion del boton de inicio
        inicio = true;
        setBarras();
        progreso = 0;
        vida = 100;
    }

    if(data[0] == 'R'){ // Interrupcion del boton de stop
        inicio = false;
    }
}

static void manejoDeLEDsyBuzzers(){
    
    while (sensor == sensor_anterior){
        sensor = rand() % (CANTIDADSENSORES); //Obtengo el sensor a encender
    }

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
    sensor_anterior = sensor;
    //BuzzerOn(); // prendo el buzzer
}


static void activar_IR(){
    IR = true;
    LedOff(LED_1);
}

void obtenerSensorPrendido(){    //Asigna la tension del bloque activado a la variable sensorPrendido
    switch (sensor){
        case 0: sensorPrendido = SENSORR; break;
        case 1: sensorPrendido = SENSORA; break;
        case 2: sensorPrendido = SENSORV; break;
        case 3: sensorPrendido = SENSORN; break;
    }
    printf("Tension del sensor prendido: %d\r\n",sensorPrendido);
}

void apagarLeds(){
    for(int i = 0 ; i<CANTIDADSENSORES*4; i++){
        cantidadLeds[i]=0;
    }
}

static void medir_tiempo(){
    tiempoMedido = ((u_int16_t)(tiempoInicio - tiempoFinal)) / CLOCKS_PER_SEC; //calcula el tiempo
}

static void manejarInterfaz(void *pvParameter){
    setBarras();

    while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (medidaAcertada){
            char msg[10];
            progreso = progreso + 10;
            sprintf(msg, "*P%d", progreso); // Actualiza la barra de progreso
            BleSendString(msg);
        }

        if (medidaIncorrecta){
            char msg[10];
            vida = vida - 25;   
            sprintf(msg, "*V%d", vida);     // Actualiza la barra de vida
            BleSendString(msg);
        }
	}
}

static void controlar(void *pvParameter){
    xTaskNotifyGive(interfaz_task_handle);

    manejoDeLEDsyBuzzers();
    obtenerSensorPrendido();
    tiempoInicio = clock(); //Inicia el tiempo

    while(1){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (IR == true){    // Si selecciona algun sensor

            AnalogInputReadSingle(CH1, &voltaje);   //se obtiene el voltaje asociado al sensor seleccionado
            printf("%d\n",voltaje);

            if(voltaje > (sensorPrendido-100) && voltaje < (sensorPrendido+100)){  // Si (...): el sensor elegido es correcto
                medidaAcertada = true;
                tiempoFinal = clock(); //obtiene el tiempo final
                medir_tiempo(); //obtiene la diferencia de tiempos
                LedOn(LED_1);
                apagarLeds();
                //BuzzerOff(); //Apago el BUZZER
                printf("Tiempo medido: %d\r\n", tiempoMedido);

                xTaskNotifyGive(interfaz_task_handle);
            }

            else{
                medidaIncorrecta = true;

                xTaskNotifyGive(interfaz_task_handle);
            }

            IR = false;  //cambia el estado del infrarojo
        }

        if(medidaAcertada == true){ // Si selecciona el sensor correcto cambia a otro sensor
            manejoDeLEDsyBuzzers();
            obtenerSensorPrendido();
            tiempoInicio = clock(); //Incia el tiempo 

            medidaAcertada = false;
        }
    }
}



/*==================[external functions definition]==========================*/
void app_main(void){
    
    // Inicializacion de los perifericos
    NeoPixelInit(GPIO_LEDS, CANTIDADSENSORES*4, cantidadLeds);
    GPIOActivInt(GPIO_IR, *activar_IR, true, NULL); //lanza el evento de que el IR midio
    //BuzzerInit(GPIO_IR);
    //BuzzerSetFrec(NOTE_C3); // se setea el tono con el que suena el buzzer

    // Configuracion e inicializacion de ADC
	analog_input_config_t analogIn = {
		.input = CH1,
		.mode = ADC_SINGLE
	}; AnalogInputInit(&analogIn); 	

	// Inicialización y configuracion de timers 
    timer_config_t timer_controlador = {
        .timer = TIMER_A,
        .period = TIEMPO_MEDICION,
        .func_p = funcTimerControlador,
        .param_p = NULL
    }; TimerInit(&timer_controlador);

    /*
    timer_config_t timer_interfaz = {
        .timer = TIMER_B,
        .period = TIEMPO_REFRESCO_PANTALLA,
        .func_p = funcTimerInterfaz,
        .param_p = NULL
    }; TimerInit(&timer_interfaz);
    */

    // Creacion de tareas
    xTaskCreate(&controlar, "Controlador", 4096, NULL, 5, &controlador_task_handle);
    xTaskCreate(&manejarInterfaz, "Interfaz", 4096, NULL, 5, &interfaz_task_handle);

    // Inicio de los timers
    //TimerStart(timer_interfaz.timer);
    TimerStart(timer_controlador.timer);

/*
    // Configuracion e inicializacion BT
    ble_config_t ble_configuration = { // Configuracion del BT
        "JOAQUIN y LUCAS",    // Nombre del dispositivo
        read_data       // Función a ejecutarse ante un interrupción de recepción a través de la conexión BLE.
    }; BleInit(&ble_configuration);    // Inicializa BT

    while(1){       // Chequea el estado del BT para prender un LED segun este conectado, desconectado o apagado
        vTaskDelay(500 / portTICK_PERIOD_MS);
        switch(BleStatus()){    
            case BLE_OFF:
                LedOff(LED_BT);
            break;
            case BLE_DISCONNECTED:
                LedToggle(LED_BT);
            break;
            case BLE_CONNECTED:
                LedOn(LED_BT);
            break;
        }
    }
*/

}
/*==================[end of file]============================================*/