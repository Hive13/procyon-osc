#include "inc/hw_ethernet.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_udma.h"
#include "driverlib/debug.h"
#include "driverlib/ethernet.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/udma.h"
#include "driverlib/ssi.h"
#include "driverlib/pin_map.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"

#include "shiftbrite.h"

// Is this the best way to do this? It requires another file to have inited
// properly.
extern int delay_usec;

void shiftbrite_command(int cmd, int red, int green, int blue) {
    // Make sure we are latched low initially
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0x00);

    ROM_SSIDataPut(SSI0_BASE, cmd << 6 | blue >> 4);
    while(ROM_SSIBusy(SSI0_BASE));
    ROM_SSIDataPut(SSI0_BASE, blue << 4 | red >> 6);
    while(ROM_SSIBusy(SSI0_BASE));
    ROM_SSIDataPut(SSI0_BASE, red << 2 | green >> 8);
    while(ROM_SSIBusy(SSI0_BASE));
    ROM_SSIDataPut(SSI0_BASE, green);
    while(ROM_SSIBusy(SSI0_BASE));
}

void shiftbrite_latch(void) {
    // Latch high and then back low (make sure to pull it low
    // before the rising edge of the next clock)
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0xFF);
    SysCtlDelay(delay_usec);
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0x00);
}


