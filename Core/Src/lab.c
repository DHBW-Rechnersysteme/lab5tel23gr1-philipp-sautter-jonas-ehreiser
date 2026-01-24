#include "Lab.h"
#include "semphr.h"
#include "stdbool.h"
TaskHandle_t xHandle = NULL;


void TaskA(void)
{
}



void Lab(void)
{
	xTaskCreate(
		TaskA,       					/* Function that implements the task. */
		"TaskA",          				/* Text name for the task. */
		configMINIMAL_STACK_SIZE,      	/* Stack size in words, not bytes. */
		( void * ) 1,    				/* Parameter passed into the task. */
		osPriorityNormal1,				/* Priority at which the task is created. */
		&xHandle );      				/* Used to pass out the created task's handle. */

}

void CDC_ReceiveCallBack(uint8_t *buf, uint32_t len)
{

}



