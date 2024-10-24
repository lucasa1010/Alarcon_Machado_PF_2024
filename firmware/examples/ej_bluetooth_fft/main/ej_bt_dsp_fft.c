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

/*==================[internal functions declaration]=========================*/
/**
 * @brief Función a ejecutarse ante un interrupción de recepción 
 * a través de la conexión BLE.
 * 
 * @param data      Puntero a array de datos recibidos
 * @param length    Longitud del array de datos recibidos
 */
void read_data(uint8_t * data, uint8_t length){ 
    printf("cadena:%s\r\n",(char*)data);
    BleSendString((char*)data);

    switch (data[0]){
        case 'G': xTaskNotifyGive(interfaz_task_handle); break;
        case 'R': xTaskNotifyGive(interfaz_task_handle); break;
        case 'C': xTaskNotifyGive(interfaz_task_handle); break;
    }
    /*
    if(data[0] == 'G') // Interrupcion del boton de inicio
        xTaskNotifyGive(interfaz_task_handle);
    

    if(data[0] == 'g') // Interrupcion del boton de inicio
        xTaskNotifyGive(interfaz_task_handle);
    

    if(data[0] == 'R') // Interrupcion del boton de stop
        xTaskNotifyGive(interfaz_task_handle);
    */
}

static void manejarInterfaz(void *pvParameter){
    while(true){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        printf("Deteccion\n");


        BleSendString((char*)"*P");   // Envio el caracter para la barra de progreso
        //BleSendString((char*)"*V");   // Envio el caracter para la barra de vida
    }
}
/*==================[external functions definition]==========================*/
void app_main(void){
    ble_config_t ble_configuration = { // Configuracion del BT
        "JAOQUIN",    // Nombre del dispositivo
        read_data       // Función a ejecutarse ante un interrupción de recepción a través de la conexión BLE.
    };

    xTaskCreate(&manejarInterfaz, "Interfaz", 2048, NULL, 5, &interfaz_task_handle);

    LedsInit();  
    BleInit(&ble_configuration);
    
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
