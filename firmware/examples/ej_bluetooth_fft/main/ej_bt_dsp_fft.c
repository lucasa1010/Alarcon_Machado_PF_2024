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
/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD 500
#define LED_BT	            LED_1
#define BUFFER_SIZE         256
#define SAMPLE_FREQ	        220
/*==================[internal data definition]===============================*/
TaskHandle_t interfaz_task_handle = NULL;
//uint8_t boton;
char msg[30];
bool inicio = false;
uint8_t vida;
uint8_t progreso;
uint8_t nivel = 1;
/*==================[internal functions declaration]=========================*/
void setBarras(){
    sprintf(msg, "*P%d", 0); // Actualiza la barra de progreso
    BleSendString(msg);

    printf(msg, "*V%d", 100);     // Actualiza la barra de vida
    BleSendString(msg);
}

/**
 * @brief Función a ejecutarse ante un interrupción de recepción 
 * a través de la conexión BLE.
 * 
 * @param data      Puntero a array de datos recibidos
 * @param length    Longitud del array de datos recibidos
 */
void read_data(uint8_t * data, uint8_t length){ 

    //sprintf(msg, "%s\r\n", data);   // Guarda en msg el mensaje recibido
    
    if(data[0] == 'G'){ // Interrupcion del boton de inicio
        inicio = true;
        setBarras();
        xTaskNotifyGive(interfaz_task_handle);
    }

    if(data[0] == 'R'){ // Interrupcion del boton de stop
        inicio = false;
        xTaskNotifyGive(interfaz_task_handle);
    }

    if(data[0] == '1'){ // Seleccion de nivel
        nivel = 1;
        xTaskNotifyGive(interfaz_task_handle);
    }

    if(data[0] == '2'){ // Seleccion de nivel
        nivel = 2;
        xTaskNotifyGive(interfaz_task_handle);
    }

    if(data[0] == '3'){ // Seleccion de nivel
        nivel = 3;
        xTaskNotifyGive(interfaz_task_handle);
    }
}

static void manejarInterfaz(void *pvParameter){
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        BleSendString(msg); // Muestra el boton tocado en el celular

        if (inicio){
            progreso = progreso + 10;
            sprintf(msg, "*P%d", progreso); // Actualiza la barra de progreso
            BleSendString(msg);
        }
        else{
            vida = vida - 25;   
            sprintf(msg, "*V%d", vida);     // Actualiza la barra de vida
            BleSendString(msg);
        }
    }
}
/*==================[external functions definition]==========================*/
void app_main(void){
    ble_config_t ble_configuration = { // Configuracion del BT
        "JOAQUIN",    // Nombre del dispositivo
        read_data       // Función a ejecutarse ante un interrupción de recepción a través de la conexión BLE.
    };

    xTaskCreate(&manejarInterfaz, "Interfaz", 2048, NULL, 5, &interfaz_task_handle); // Crea tarea Interfaz

    LedsInit();  // Inicializa LEDs
    BleInit(&ble_configuration);    // Inicializa BT
    
    while(1){
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        switch(BleStatus()){    // Chequea el estado del BT para prender un LED segun este conectado, desconectado o apagado
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
}

/*==================[end of file]============================================*/
