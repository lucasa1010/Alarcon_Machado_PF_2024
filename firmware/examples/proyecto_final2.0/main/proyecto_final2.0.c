/*! @mainpage Proyecto final Alarcon Machado 2C 2024
 *
 * @section genDesc General Description
 *
 * Este proyecto se ha realizado para regularizar la materia Electronica Programable el
 * segundo cuatrimestre del anio 2024.
 * Consiste en un juego de reaccion de encendido de luces que mide
 * el tiempo que uno demora en acercar su mano el sensor correcto. 
 * En el caso de agotarse el tiempo para seleccionar el sensor se emitira
 * un pitido y se asignara un nuevo sensor para seleccionar
 * 
 *@section hardConn Hardware Connection
 *
 * |   	BUZZER		|   ESP-EDU		|
 * |:--------------:|:--------------|
 * | 	VCC      	|	 GPIO_20    |
 * | 	GND		 	| 	  GND		|
 * 
 * |   	SENSOR		|   ESP-EDU		|
 * |:--------------:|:--------------|
 * | 	VCC		 	| 	  +5V       |
 * | 	GND		 	| 	  GND	    |
 * |  Din(LEDs)  	|	 GPIO_9	    |
 * | Signal(IR)		| 	 GPIO_22	|
 * 
 * 
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/11/2024 | Entrega de documento	                         |
 *
 * @author Lucas Alarcon (lucas.alarcon@ingenieria.uner.edu.ar)
 * @author Joaquin Machado (joaquin.machado@ingenieria.uner.edu.ar)
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "neopixel_stripe.h"
#include "ble_mcu.h"
#include "analog_io_mcu.h"
#include "timer_mcu.h"

/*==================[macros and definitions]=================================*/
#define TIEMPO_MEDICION 1000        /*!< Periodo de tiempo de medición en ms */
#define TIEMPO_CLOCK 1000           /*!< Periodo del clock en us */
#define GPIO_LEDS GPIO_9            /*!< GPIO de conexión tira neopixel */
#define GPIO_IR GPIO_22             /*!< LED de aviso de sensado IR */
#define GPIO_BUZZER GPIO_20         /*!< GPIO de conexión del buzzer */
#define SENSOR_R  785               /*!< Tensión asociada a sensor rojo en mV */
#define SENSOR_O  2353              /*!< Tensión asociada a sensor naranja en mV */
#define SENSOR_G  2753              /*!< Tensión asociada a sensor verde en mV */
#define SENSOR_B  1360              /*!< Tensión asociada a sensor azul en mV */
#define CANTIDADdeSENSORES 4        /*!< Cantidad de sensores disponibles */
#define TIEMPOdeESPERA 6000         /*!< Ventana de tiempo en que se acepta la medicion antes de perder */

/*==================[internal data definition]===============================*/
TaskHandle_t controlador_task_handle = NULL; /*!< Handle de la tarea de control */
bool deteccionIR = false; /*!< Indica si hubo una deteccion del sensor infrarrojo */
uint16_t voltajeMedido = 0; /*!< Voltaje medido en el sensor activo */
bool medidaAcertada = true; /*!< Indica si el sensor seleccionado fue el correcto */
neopixel_color_t cantidadLeds [CANTIDADdeSENSORES*4]; /*!< Arreglo que almacena el color de cada LED de neopixel y conoce la cantida de LEDs*/
int sensor = -1; /*!< Sensor seleccionado */
uint16_t tensionSensorPrendido = 0; /*!< Tensión asociada al sensor activado */
int sensorAnterior = -1; /*!< Último sensor seleccionado */
uint64_t conteo_en_ms = 0; /*!< Conteo de tiempo en ms */
bool pierde = false; /*!< Indica si se acaba el tiempo para seleccionar el sensor prendido */

/*==================[internal functions declaration]=========================*/

/*! 
 * @brief Función del timer para control del proceso
 */
void funcTimerControlador(){
    vTaskNotifyGiveFromISR(controlador_task_handle, pdFALSE);
}

/*! 
 * @brief Función del timer de tiempo 
 */
void funcTimerTiempo(){
    conteo_en_ms++;
    if (conteo_en_ms > TIEMPOdeESPERA) 
        pierde = true;
}

/*! 
 * @brief Maneja la activación de los LEDs 
 */
static void manejoDeLEDs(){
    while (sensor == sensorAnterior){
        sensor = rand() % (CANTIDADdeSENSORES); // Obtiene un sensor aleatorio
    }
    for(int i = sensor*4; i < (sensor*4 + 4); i++){
        switch(sensor) {
            case 0: NeoPixelSetPixel(i, NEOPIXEL_COLOR_RED); break;
            case 1: NeoPixelSetPixel(i, NEOPIXEL_COLOR_BLUE); break;
            case 2: NeoPixelSetPixel(i, NEOPIXEL_COLOR_GREEN); break;
            case 3: NeoPixelSetPixel(i, NEOPIXEL_COLOR_ORANGE); break;
        }
    }
    sensorAnterior = sensor;
}

/*! 
 * @brief Activa el evento del sensor IR
 */
static void cambioIRaActivo(){
    deteccionIR = true;
}

/*! 
 * @brief Obtiene la tensión del sensor activado
 */
void obtenerSensorPrendido(){
    switch (sensor){
        case 0: tensionSensorPrendido = SENSOR_R; break;
        case 1: tensionSensorPrendido = SENSOR_B; break;
        case 2: tensionSensorPrendido = SENSOR_G; break;
        case 3: tensionSensorPrendido = SENSOR_O; break;
    }
}

/*! 
 * @brief Apaga todos los LEDs de la tira de neopixel
 */
void apagarLeds(){
    for(int i = 0 ; i < CANTIDADdeSENSORES*4; i++){
        cantidadLeds[i] = 0;
    }
}

/*! 
 * @brief Tarea de control principal
 * @param pvParameter Parámetro de la tarea (no utilizado)
 */
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
        if(medidaAcertada){ // Si el sensor seleccionado es el correcto...
            manejoDeLEDs();
            obtenerSensorPrendido();
            medidaAcertada = false;
        }
        if (deteccionIR){ // Si hay una deteccion del sensor IR...
            AnalogInputReadSingle(CH1, &voltajeMedido);   // Se obtiene el voltaje en el sensor 

            if(voltajeMedido > (tensionSensorPrendido - 100) && voltajeMedido < (tensionSensorPrendido + 100)){
                apagarLeds();
                medidaAcertada = true;
                printf("Tiempo medido: %.2f seg\r\n", (float)conteo_en_ms / 1000);
                conteo_en_ms = 0;
            }
            deteccionIR = false;
        }
    }
}

/*==================[external functions definition]==========================*/

/*! 
 * @brief Función principal de la aplicación
 */
void app_main(void){
    NeoPixelInit(GPIO_LEDS, CANTIDADdeSENSORES * 4, cantidadLeds);  // Inicializa la tira NeoPixel
    GPIOActivInt(GPIO_IR, *cambioIRaActivo, true, NULL); // Configura interrupción del IR
    GPIOInit(GPIO_BUZZER, GPIO_OUTPUT);     // Inicializa el GPIO al que se conecta el buzzer

    analog_input_config_t analogIn = { // Configuracion ADC
        .input = CH1, 
        .mode = ADC_SINGLE 
        }; AnalogInputInit(&analogIn); // Inicialización del ADC

    timer_config_t timer_controlador = { 
        .timer = TIMER_A, 
        .period = TIEMPO_MEDICION, 
        .func_p = funcTimerControlador 
        }; TimerInit(&timer_controlador);   

    timer_config_t timer_tiempo = { 
        .timer = TIMER_B, 
        .period = TIEMPO_CLOCK, 
        .func_p = funcTimerTiempo 
        }; TimerInit(&timer_tiempo);

    // Creacion de tarea
    xTaskCreate(&controlar, "Controlador", 4096, NULL, 5, &controlador_task_handle);

    // Inicio de timers
    TimerStart(timer_controlador.timer);
    TimerStart(timer_tiempo.timer);
}

/*==================[end of file]============================================*/
