#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#define LED_Base_Address 0x4BB00000
# define BTN_Base_Address 0x4BB02004
void turnOnLED();
int main(){
 init_platform();
 while( (*((uint32_t*) BTN_Base_Address) && (1))==0){ // (*unint32_t) BTN_Base_Address means cast it as a pointer and the first * means get is value
 ; // wait on BTN0
 }
 turnOnLED();
 print("Hello World\n\r");
 cleanup_platform();
 return 0;
}
void turnOnLED(){
*((uint32_t*) LED_Base_Address) = 0x0000000F;
*((uint32_t*) LED_Base_Address+1) = 0x0000000F;
return;
}
