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


#define RELOAD_VALUE 16000000 //16,000,000
#ifdef DEBEG
void
__error__(char *pcFilename, uint32_t ui32Line)
{

}
#endif


volatile int edgeTimes[200]; 
volatile int edgeI = 0;
volatile unsigned char started = false;
volatile int repeat = 0;

void Edge_Handler (void) {
	//If SysTick has not already begun, clear it by writing to it to start at reload value
	GPIOIntClear(GPIO_PORTB_BASE, GPIO_PIN_7);
	if (started == false) {
		NVIC_ST_CURRENT_R = 0x00; 
		started = true;
		//repeat = 0;
	}
	//Record time of edge, then increment edgeI
	edgeTimes[edgeI] = ROM_SysTickValueGet(); 
	edgeI++;
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
	ROM_GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_7 );  //Configure pin as input
	//ROM_GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU); //Configure pull up resistor (Only needed for switches)
		
	//Configure chosen pin for interrupts
	ROM_GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_BOTH_EDGES); //Interrupt triggered on both edges (rising/falling distinguished in interrupt handler)

	//Enable interrupts (on pin, port, and master)
	GPIOIntEnable(GPIO_PORTB_BASE, GPIO_PIN_7);
	ROM_IntEnable(INT_GPIOB); 
	ROM_IntMasterEnable();
	
	ConfigureUART();

	UARTprintf("System Reset\n");

	//Set up SysTick 
	ROM_SysTickPeriodSet(RELOAD_VALUE);  //When SysTick is written to, this is written in as the reload value
	ROM_SysTickIntEnable(); //Default SysTick interrupt just reloads SysTick
	ROM_SysTickEnable();    //SysTick will always be running, it gets cleared at the start of the pulse

	int signalArr[90];
	int signalI=0;
	float gap = 0.0;
	float sigNum1 = 0.0;
	int sigNum = 0;
	int timesPressed = 0;
	ROM_SysTickIntDisable();
	//int repeat = 0;
	
	while(1){
		ROM_SysCtlSleep();
	
		if ((started == true) && (ROM_SysTickValueGet() < (RELOAD_VALUE - 2000000))) {//If the timer has already started and has counted down past 40ms (2M ticks)
			//repeat = 1;
			//Block all signals while decoding
			GPIOIntDisable(GPIO_PORTB_BASE, GPIO_PIN_7); 
			timesPressed++;
			//We know pulses start with a falling edge, so from edgeTime[0] (falling edge) to edgeTime [1] (rising edge), the signal must be low
			//And from edgeTime[1](rising) to edgeTime[2](falling), the signal must be high, so (edgeTime[odd] - edgeTime[even]) are high values and vice versa
			
			//Calculate the number of ticks in each gap by subtracting
			//Divide that by 11,275 ticks (.2255 ms) to get the number of quarters in each gap
			//We only care about the values of two quarters in each period (which we already know must be low/high for the gap)
			//(Number ticks in gap/11,725)/2 = Number of high/low values to add to signalArr for this gap
			for (int i=0; i < edgeI; i++) {
				gap = edgeTimes[i] - edgeTimes[i+1]; 		//Stores distance between first and second pulse
				sigNum1 = gap/22550; 										//Number of values in gap = (Number ticks in gap/11,725)/2
				float sigNum2 = roundf(sigNum1);
				sigNum = sigNum2;				
				
				for (int j=0; j < sigNum; j++){
					signalArr[signalI+j] = (i%2==0)? 0 : 1; //if it's edgeTime[even] - edgeTime[odd], append low values, and vice versa
				}
				signalI += sigNum;
			}

			/*for(int i = 0; i < (signalI+1); i++)
			{
				UARTprintf("SA[%d]: %d -- ", i, signalArr[i]);
			}*/
			signalI--;
			//UARTprintf("Signal I = %d --- Edge i = %d\n", signalI, edgeI);
			//UARTprintf("\nSigNum: %d\n", sigNum);
			//SignalArr is all set up and good to go
			//It should be of length signalI by this point
			

	int binIndex = 0;
	int signalIndexTwo = 0;
	char determinedValue[20];
	for(int i = 0; i < 20; i++)
		determinedValue[i] = '?';
	int dValue = 0;
	int binArray[50];
	//UARTprintf("DECODE INTS LOADED\n");
		
			for(int i = 0; i < 50; i++)
				binArray[i] = -2;

			while(signalIndexTwo < 84)
			{
				if((signalArr[signalIndexTwo] - signalArr[signalIndexTwo + 1]) == -1) //Rising edge, 0 to 128
				{
					binArray[binIndex] = 1;
					signalIndexTwo = signalIndexTwo + 2;
					binIndex++;
				}
				else if(signalArr[signalIndexTwo] - signalArr[signalIndexTwo + 1] == 1) //Falling edge, 128 to 0
				{
					binArray[binIndex] = 0;
					signalIndexTwo = signalIndexTwo + 2;
					binIndex++;
				}
				else
				{
					binArray[binIndex] = -1; //-1 if no change (no edge in period)
					signalIndexTwo = signalIndexTwo + 2;
					binIndex++;
				}
			} //Above loop will fill binArray with the values 1, 0, and -1 based on rising, falling, or neither

			binIndex = 0;
			int signalB = 0;
			for(int i = 0; i < 50; i++)
			{
				if(binArray[i] == -2)
					i = 50;
				else
					signalB++; //Holds # of elements, not address at last element
			}
			
			//UARTprintf("\n %d: signalB \n", binArray[26]);
			
			//for(int i = 0; i < signalB; i++)
				//UARTprintf("%d ",binArray[i]);
			
			binIndex = 0;
			//UARTprintf("\n*** Begin Decode ***\n");
			//if(binArray[signalB - 16] == 0) //Determine value that was pressed based on binArray
			//{
				if(binArray[signalB - 5] == 1 && binArray[signalB - 6] == 1)
				{
					//Determine between number buttons
					dValue = (8 * binArray[signalB - 4]) + (4 * binArray[signalB - 3]) + (2 * binArray[signalB - 2]) + (binArray[signalB - 1]);
					sprintf(determinedValue,"%d",dValue);
				}
				else if(binArray[signalB -5] == 0 && binArray[signalB - 6] == 0)
				{
					//Determine which Volume or Channel Button was pressed
					dValue = (8 * binArray[signalB - 4]) + (4 * binArray[signalB - 3]) + (2 * binArray[signalB - 2]) + (binArray[signalB - 1]);
					if(dValue == 2)
						strcpy(determinedValue, "Up Arrow or Channel Up");
					else if(dValue == 3)
						strcpy(determinedValue, "Down Arrow or Channel Down");
					else if(dValue == 5)
						strcpy(determinedValue, "Right Arrow or Volume Up");
					else if(dValue == 4)
						strcpy(determinedValue, "Left Arrow or Volume Down");
					else
						strcpy(determinedValue, "ERROR");
				}
			//}
			//else if(binArray[signalB - 16] == 1)
			{
				//Determine which arrow, mute, or enter was pressed
				dValue = (8 * binArray[signalB - 4]) + (4 * binArray[signalB - 3]) + (2 * binArray[signalB - 2]) + (binArray[signalB - 1]);
				/*if(dValue == 2)
					strcpy(determinedValue, "Up Arrow or Channel Up");
				else if(dValue == 3)
					strcpy(determinedValue, "Down Arrow or Channel Down");
				else if(dValue == 4)
					strcpy(determinedValue, "Left Arrow or Volume Down");
				else if(dValue == 5)
					strcpy(determinedValue, "Right Arrow or Volume Up");*/
				if(dValue == 11)
					strcpy(determinedValue, "Mute");
				else if(dValue == 6)
					strcpy(determinedValue, "6 or Enter");
				
			}
			//else
				//strcpy(determinedValue, "Loser");
			if(determinedValue[0] != '?' || determinedValue < 0)
				UARTprintf("You Pressed: %s\n", determinedValue);
	
			//Delay past redundancy pulses then re-enable interrupts
			ROM_SysCtlDelay (ROM_SysCtlClockGet()*.14); 
			GPIOIntEnable(GPIO_PORTB_BASE, GPIO_PIN_7);
			
			//Reset global variables (edgeTimes does not need to be reset, gets overwritten)
			edgeI = 0;
			started = false;
			//Reset other values
			signalI =0;
			for(int i = 0; i < 90; i++)
				signalArr[i] = 0;
			
			for(int i = 0; i < 200; i++)
				edgeTimes[i] = 0;
		}
		//UARTprintf("After If Loop \n");
	}

return 0;
}
