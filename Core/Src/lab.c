#include "Lab.h"
#include "semphr.h"
#include "stdbool.h"

TaskHandle_t xHandle = NULL;
SemaphoreHandle_t xTimeSemaphore = NULL;

// Globale Uhrzeit-Variablen

uint8_t hours = 0;
uint8_t minutes = 0;
uint8_t seconds = 0;
uint16_t milliseconds = 0;

// Status-Flags

bool is_set = false;
bool is_running = false;


uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);

void TaskA(void)
{

    vTaskSuspend(NULL);  // Pausiert Task zu beginn

    TickType_t xLastWakeTime = xTaskGetTickCount();  // setzt momentanen Tick Wert des Systems als Referenz
    const TickType_t xFrequency = pdMS_TO_TICKS(1);  // setzt 1ms == 1 tick für FreeRtos

    while(1)
    {
        if (!is_running) {
            vTaskSuspend(NULL);
            xLastWakeTime = xTaskGetTickCount();
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency); // delay von exaktem Startpunkt aus mit Länge der Frequenz also 1ms (1 Tick)

        if(xSemaphoreTake(xTimeSemaphore, portMAX_DELAY) == pdTRUE) { // nur wenn Semaphore genommen werden kann  wird die Schleife gestartet

            bool send_update = false;
            milliseconds++;

            if(milliseconds >= 1000) {
                milliseconds = 0;
                seconds++;
                send_update = true;

                if(seconds >= 60) {
                    seconds = 0;
                    minutes++;
                    if(minutes >= 60) {
                        minutes = 0;
                        hours++;
                        if(hours >= 24) hours = 0;
                    }
                }
            }

            if(send_update && is_running) {

                char time_msg[30];
                sprintf(time_msg, "%02d:%02d:%02d:%03d\r\n", hours, minutes, seconds, milliseconds);
                CDC_Transmit_FS((uint8_t *) time_msg, strlen(time_msg));
            }

            xSemaphoreGive(xTimeSemaphore);  // gibt Semaphore wird frei
        }
    }
}

// Initialisierung
void Lab(void)
{
    xTimeSemaphore = xSemaphoreCreateBinary();  // initialisiert Semaphore
    xSemaphoreGive(xTimeSemaphore);

	xTaskCreate(								// initialisiert Task
		(TaskFunction_t)TaskA,
		"TaskA",
		configMINIMAL_STACK_SIZE + 256,
		( void * ) 1,
		osPriorityNormal1,
		&xHandle );
}

//Callback für USB
void CDC_ReceiveCallBack(uint8_t *buf, uint32_t len)
{
    char* cmd = (char*)buf;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // "start" auswerten
    if (len >= 5 && strncmp(cmd, "start", 5) == 0) {
        if (is_running) {
            char msg[] = "Fehler: Uhr laeuft bereits\r\n";
            CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
        } else if (!is_set) {
            char msg[] = "Fehler: Uhr nicht initial gesetzt\r\n";
            CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
        } else {
            is_running = true;
            char msg[] = "Uhr gestartet\r\n";
            CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
            xTaskResumeFromISR(xHandle);
        }
    }
    // "stop" auswerten
    else if (len >= 4 && strncmp(cmd, "stop", 4) == 0) {
        if (!is_running) {
            char msg[] = "Fehler: Uhr ist schon gestoppt\r\n";
            CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
        } else {
            is_running = false;
            char msg[] = "Uhr gestoppt\r\n";
            CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
        }
    }
    // "set hh:mm:ss:msmsms" auswerten
    else if (len >= 16 && strncmp(cmd, "set ", 4) == 0) {

        int h = (buf[4] - '0') * 10 + (buf[5] - '0');
        int m = (buf[7] - '0') * 10 + (buf[8] - '0');
        int s = (buf[10] - '0') * 10 + (buf[11] - '0');
        int ms = (buf[13] - '0') * 100 + (buf[14] - '0') * 10 + (buf[15] - '0');

        if (xSemaphoreTakeFromISR(xTimeSemaphore, &xHigherPriorityTaskWoken) == pdTRUE) {
            hours = h; minutes = m; seconds = s; milliseconds = ms;
            is_set = true;
            xSemaphoreGiveFromISR(xTimeSemaphore, &xHigherPriorityTaskWoken);

            char msg[] = "Uhrzeit gesetzt. Bereit zum Starten.\r\n";
            CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
        }
    } else {
        char msg[] = "Fehler: Unbekannter Befehl\r\n";
        CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
