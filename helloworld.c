/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xil_exception.h"
#define ICCPMR_BASEADDR 0xF8F00104 // Interrupt Priority Mask Register
#define ICCICR_BASEADDR 0xF8F00100 // CPU Interface Control Register
#define ICDDCR_BASEADDR 0xF8F01000 // Distributor Control Register
#define ICDISER_BASEADDR 0xF8F01100 // Interrupt Set Enable Register
#define ICDICER_BASEADDR 0xF8F01180 // Interrupt Clear/Enable Register
#define ICDIPR_BASEADDR 0xF8F01400 // Interrupt Priority Register Base address
#define ICDIPTR_BASEADDR 0xF8F01800 // Interrupt Processor Targets Register
#define ICDICFR_BASEADDR 0xF8F01C00 // Interrupt Configuration Register
#define ICCIAR_BASEADDR 0xF8F0010C // Interrupt Acknowledge Register
#define ICCEOIR_BASEADDR 0xF8F00110 // End Of Interrupt Register
#define GPIO_MTDATA_OUT_0 0xE000A004 // Maskable data out in bank 0
#define GPIO_INT_DIS_0 0xE000A214 // Interrupt disable bank 0
#define GPIO_INT_EN_1 0xE000A250 // Interrupt enable bank 1
#define GPIO_INT_DIS_1 0xE000A254 // Interrupt disable bank 1
#define GPIO_INT_STAT_1 0xE000A258 // Interrupt status bank 1
#define GPIO_INT_TYPE_1 0xE000A25C // Interrupt type bank 1
#define GPIO_INT_POL_1 0xE000A260 // Interrupt polarity bank 1
#define GPIO_INT_ANY_1 0xE000A264 // Interrupt any edge sensitive bank 1
#define MIO_PIN_16 0xF8000740
#define MIO_PIN_17 0xF8000744
#define MIO_PIN_18 0xF8000748
#define MIO_PIN_50 0xF80007C8
#define MIO_PIN_51 0xF80007CC
#define GPIO_DIRM_0 0xE000A204 // Direction mode bank 0
#define GPIO_OUTE_0 0xE000A208 // Output enable bank 0
#define GPIO_DIRM_1 0xE000A244 // Direction mode bank 1
void Configure_I0();
void disable_interrupts();
void configure_GIC();
void enable_interrupts();
void Initialize_GPIO_Interrupts();
void IRQ_Handler(void *data);

int main()
{
	init_platform(); //initializes the platform, in the "platform.h" file, configures the UART
	Configure_I0();
	disable_interrupts();
	configure_GIC();
	Initialize_GPIO_Interrupts();// Initialize MIO LED
	Xil_ExceptionRegisterHandler(5, IRQ_Handler, NULL);// build-in
	// Set up an IRQ interrupt handler
	 enable_interrupts(); // Enable ARM Cortexv7 A9 interrupts
	 cleanup_platform();
	 return 0;
}


//configure button 4/5 as inputs and LED8 as the output
void Configure_I0()
{
	*((uint32_t *) 0xF8000000+0x8/4) = 0x0000DF0D; //Write unlock code to enable writing into system level
	//Control unlock Register
	*((uint32_t*) MIO_PIN_50) = 0x00000600; // BTN4 6th group of four is: 0110
	*((uint32_t*) MIO_PIN_51) = 0x00000600; //BTN5 6th group of four is: 0110
	*((uint32_t*) MIO_PIN_16) = 0x00001600; // RGB_LED_B
	*((uint32_t*) MIO_PIN_17) = 0x00001600; // RGB_LED_R
	*((uint32_t*) MIO_PIN_18) = 0x00001600; // RGB_LED_G 5th group of four is: 0001 6th group of four is: 0110
	*((uint32_t*) GPIO_DIRM_0) = 0x00070000; //direction mode for bank 0
	*((uint32_t*) GPIO_OUTE_0) = 0x00070000; //output enable for bank 0
	*((uint32_t*) GPIO_DIRM_1) = 0x00000000; //direction mode for bank 1
	return;

}


void disable_interrupts()
{
	uint32_t mode = 0xDF; // System mode [4:0] and IRQ disabled [7], D == 1101, F == 1111
	//what does this mean???
	uint32_t read_cpsr=0; // used to read previous CPSR value, read status register values
	uint32_t bit_mask = 0xFF; // used to clear bottom 8 bits
	__asm__ __volatile__("mrs %0, cpsr\n" : "=r" (read_cpsr) ); // execute the assembly instruction MSR
	__asm__ __volatile__("msr cpsr,%0\n" : : "r" ((read_cpsr & (~bit_mask))| mode)); // only change the
	//lower 8 bits
	return;
}


void configure_GIC(){
*((uint32_t*) ICDIPTR_BASEADDR+13) = 0x00000000;
*((uint32_t*) ICDICER_BASEADDR+1) = 0x00000000;
*((uint32_t*) ICDDCR_BASEADDR) = 0x0;
*((uint32_t*) ICDIPR_BASEADDR+13) = 0x000000A0;
*((uint32_t*) ICDIPTR_BASEADDR+13) = 0x00000001;
*((uint32_t*) ICDICFR_BASEADDR+3) = 0x55555555;
*((uint32_t *) ICDISER_BASEADDR+1) = 0x00100000;
*((uint32_t*) ICCPMR_BASEADDR) = 0xFF;
*((uint32_t*) ICCICR_BASEADDR) = 0x3;
*((uint32_t*) ICDDCR_BASEADDR) = 0x1;
return;
}


void enable_interrupts(){
uint32_t read_cpsr=0; // used to read previous CPSR value
uint32_t mode = 0x5F; // System mode [4:0] and IRQ enabled [7]
uint32_t bit_mask = 0xFF; // used to clear bottom 8 bits
__asm__ __volatile__("mrs %0, cpsr\n" : "=r" (read_cpsr) );
__asm__ __volatile__("msr cpsr,%0\n" : : "r" ((read_cpsr & (~bit_mask))| mode));
return;
}


void IRQ_Handler(void *data){
uint32_t GPIO_INT = *((uint32_t*) GPIO_INT_STAT_1);
uint32_t interrupt_ID = *((uint32_t*) ICCIAR_BASEADDR);
uint32_t button_press = 0xC0000 & GPIO_INT;
if (interrupt_ID == 52) { // check if interrupt is from the GPIO
if (button_press == 0x80000){
 //turn an LED ON
		*((uint32_t*) GPIO_MTDATA_OUT_0) = 0xFFF80004;
}
else if (button_press == 0x40000){
 //Turn an LED ON.
		*((uint32_t*) GPIO_MTDATA_OUT_0) = 0xFFF80002;
}
}
*((uint32_t*)GPIO_INT_STAT_1) = 0xFFFFFF; // clear the GPIO interrupt status, when it's a one it means it's all ones, the interrupt
//has been executed
*((uint32_t*)ICCEOIR_BASEADDR) = interrupt_ID; // clear the GIC GPIO interrupt.
return;
}


void Initialize_GPIO_Interrupts(){
*((uint32_t*) GPIO_INT_DIS_1) = 0xFFFFFFFF;
*((uint32_t*) GPIO_INT_DIS_0) = 0xFFFFFFFF;
*((uint32_t*) GPIO_INT_STAT_1) = 0xFFFFFFFF; // Clear Status register
*((uint32_t*) GPIO_INT_TYPE_1) = 0x0C0000; // Type of interrupt rising edge
*((uint32_t*) GPIO_INT_POL_1) = 0x0C0000; // Polarity of interrupt
*((uint32_t*) GPIO_INT_ANY_1) = 0x000000; // Interrupt any edge sensitivity
*((uint32_t*) GPIO_INT_EN_1) = 0x0C0000; // Enable interrupts in bank 0
return;
}
