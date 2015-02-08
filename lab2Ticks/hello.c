//Colin Lee, Chris Bird 
//EEC 172 A01, Embedded Systems 
//Winter 2015

#include "stdint.h"
#include "stdbool.h"
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_nvic.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "math.h"

#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif


volatile int iTick = 0;
volatile int signalArr [84];
volatile int signalIndex = 0;
volatile unsigned char busy = false;
//volatile unsigned char hitSH = false; //Debug
//volatile unsigned char hitPH = false; //Debug

void SysTick_Handler (void){
	//hitSH = true; //Debug
	iTick++;
	
	if (iTick % 2 == 1) { //if iTick is odd
		signalArr[signalIndex] = ROM_GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_7); 
		signalIndex++;
	}
}

void Pulse_Handler(void) {
	//hitPH = true; //Debug
	GPIOIntClear(GPIO_PORTB_BASE, GPIO_PIN_7);
	GPIOIntDisable(GPIO_PORTB_BASE, GPIO_PIN_7);
	
	//Start SysTick and enable SysTick interrupts
	ROM_SysTickPeriodSet(ROM_SysCtlClockGet()*.0002255);
	ROM_SysTickIntEnable();
	ROM_SysTickEnable();
	busy = true;
}


void ConfigureUART(void)
{
	//Configure UART
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
	ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
	ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
	UARTStdioConfig(0, 115200, 16000000);
}

int main (void) {
	//Set system clock to 50 Mhz
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
	
	//Enable GPIO port and set it configure it as input 
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);  //Enable port
	ROM_GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_7);  //Configure pin as input
	//ROM_GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU); //Configure pull up resistor (Only needed for switches)
	
	
	//Configure chosen pin for interrupts
	ROM_GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_FALLING_EDGE); //Interrupt triggered on falling edge (first edge of pulse)
	
	//Enable interrupts (on pin, port, and master)
	GPIOIntEnable(GPIO_PORTB_BASE, GPIO_PIN_7);
	ROM_IntEnable(INT_GPIOB); 
	ROM_IntMasterEnable();
	
	ConfigureUART();
	
	//UARTprintf("Shit finally happened\n");
	int binArray[45];

	for(int i = 0; i < 45; i++)
		binArray[i] = -2;

	int binIndex = 0;
	int signalIndexTwo = 0;
	int sizeOf = 0;
	char determinedValue[20];
	int dValue = 0;
	UARTprintf("DECODE INTS LOADED\n");
		
	while(1){
		//UARTprintf("PinRead: %d\n", ROM_GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_7));
		//UARTprintf("Inside While Loop\n");
		ROM_SysCtlSleep();
		//UARTprintf("Looping??\n");
		//UARTprintf("Busy Signal: %u", busy);
		if(busy)
			UARTprintf("iTick Value: %d\n", iTick);
		/*
		if (hitSH) {
			UARTprintf("SysTick Handler hit\n"); //Debug
			hitSH = false;
		}
		if (hitPH) {
			UARTprintf("Pulse Handler hit\n"); // Debug
			hitPH = false;
		}*/
		
		if (busy && iTick >= 84){ //if the signal has ended (~40ms)
			//UARTprintf("Inside Decode Loop\n");
			//Decode signal
			ROM_SysTickIntDisable();
			ROM_SysTickDisable();
			
			binIndex = 0;
			sizeOf = 0;
			dValue = 0;
			signalIndexTwo = 0;
			binIndex = 0; //Index through newly created binArray with actual 0 or 1 values (-1) for no value
			for(int i = 0; i < 84; i++)
				UARTprintf("%d ", signalArr[i]);
			
			for(int i = 0; i < 42; i++)
				binArray[i] = -2;

			while(signalIndexTwo < 84)
			{
				if((signalArr[signalIndexTwo] - signalArr[signalIndexTwo + 1]) == -128) //Rising edge, 0 to 128
				{
					binArray[binIndex] = 1;
					signalIndexTwo = signalIndexTwo + 2;
					binIndex++;
				}
				else if(signalArr[signalIndexTwo] - signalArr[signalIndexTwo + 1] == 128) //Falling edge, 128 to 0
				{
					binArray[binIndex] = 0;
					signalIndexTwo = signalIndexTwo + 2;
					binIndex++;
				}
				else
				{
					binArray[binIndex] = -1; //-1 if no value
					signalIndexTwo = signalIndexTwo + 2;
					binIndex++;
				}
			} //Above loop will fill binArray with the values 1, 0, and -1 based on rising, falling, or neither

			for(int i = 0; i < 42; i++)
			{
				if(binArray[i] == -2) 
				{
					i = 42;
				}
				else
					sizeOf++;
			}
			binIndex = 0;
			UARTprintf("\n\n");
			for(int i = 0; i < 42; i++)
				UARTprintf("%d ",binArray[i]);
			
			binIndex = 0;
			UARTprintf("\n*** Begin Decode ***\n");
			if(binArray[sizeOf - 17] == 0) //Determine value that was pressed based on binArray
			{
				if(binArray[sizeOf - 6] == 1 && binArray[sizeOf - 7] == 1)
				{
					//Determine between number buttons
					dValue = (8 * binArray[sizeOf - 4]) + (4 * binArray[sizeOf - 3]) + (2 * binArray[sizeOf - 2]) + (binArray[sizeOf - 1]);
					//atoi(dValue, determinedValue,10); //It'll be here
					sprintf(determinedValue,"%d",dValue);
					UARTprintf("Is this what it is printing?");
				}
				else if(binArray[sizeOf -6] == 0 && binArray[sizeOf - 7] == 0)
				{
					//Determine which Volume or Channel Button was pressed
					dValue = (8 * binArray[sizeOf - 4]) + (4 * binArray[sizeOf - 3]) + (2 * binArray[sizeOf - 2]) + (binArray[sizeOf - 1]);
					if(dValue == 2)
						strcpy(determinedValue, "Channel Up");
					else if(dValue == 3)
						strcpy(determinedValue, "Channel Down");
					else if(dValue == 5)
						strcpy(determinedValue, "Volume Up");
					else if(dValue == 4)
						strcpy(determinedValue, "Volume Down");
					else
						strcpy(determinedValue, "ERROR");
				}
			}
			else if(binArray[sizeOf - 17] == 1)
			{
				//Determine which arrow, mute, or enter was pressed
				dValue = (8 * binArray[sizeOf - 4]) + (4 * binArray[sizeOf - 3]) + (2 * binArray[sizeOf - 2]) + (binArray[sizeOf - 1]);
				if(dValue == 2)
					strcpy(determinedValue, "Up Arrow");
				else if(dValue == 3)
					strcpy(determinedValue, "Down Arrow");
				else if(dValue == 4)
					strcpy(determinedValue, "Left Arrow");
				else if(dValue == 5)
					strcpy(determinedValue, "Right Arrow");
				else if(dValue == 11)
					strcpy(determinedValue, "Mute");
				else if(dValue == 6)
					strcpy(determinedValue, "Enter");
				else
					strcpy(determinedValue, "ERROR");
			}
			else
				strcpy(determinedValue, "Loser");

			UARTprintf("You Pressed: %s\n", determinedValue);
			
			ROM_SysCtlDelay(ROM_SysCtlClockGet()/3);

			GPIOIntEnable(GPIO_PORTB_BASE, GPIO_PIN_7); //Re-enable detection for new signals
			busy = false;    //Reset all volatile global variables (signalArr can be overwritten)
			iTick = 0;       
			signalIndex = 0;
		}
	}

return 0;
}
