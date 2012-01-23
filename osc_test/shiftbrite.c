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
#include "uip-conf.h"

#include <math.h>

#include "shiftbrite.h"

// Is this the best way to do this? It requires another file to have inited
// properly.
extern int delay_usec;
static unsigned char shiftbrite_image[3*SHIFTBRITE_MAX_X*SHIFTBRITE_MAX_Y];
// Dot correction values
static int correct_r = 65;
static int correct_g = 50;
static int correct_b = 50;

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

void shiftbrite_set_dot_correction(int r, int g, int b) {
    correct_r = r;
    correct_g = g;
    correct_b = b;
}

void shiftbrite_delay_latch(int lights) {
    float usec = 20.0/1000 + 5.0/1000.0 * lights;
    SysCtlDelay(delay_usec * ceil(usec));
    shiftbrite_latch();
}

void shiftbrite_latch(void) {
    // Latch high and then back low (make sure to pull it low
    // before the rising edge of the next clock)
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0xFF);
    SysCtlDelay(delay_usec);
    ROM_GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0x00);
}

void shiftbrite_push_image(unsigned char * img, unsigned int x, unsigned int y) {
    int row, col;
    
    for(col = x - 1; col >= 0; --col) {
        if (col % 2) {
            for(row = y - 1; row >= 0; --row) {
                unsigned char * offset = img + 3*(x*col + row);
                shiftbrite_command(0, offset[0], offset[1], offset[2]);
            }
        } else {
             for(row = 0; row < y; ++row) {
                unsigned char * offset = img + 3*(x*col + row);
                shiftbrite_command(0, offset[0], offset[1], offset[2]);
            }
        }
    }
    shiftbrite_delay_latch(x*y);
    shiftbrite_latch();
}

void shiftbrite_refresh() {
    shiftbrite_dot_correct(SHIFTBRITE_MAX_X*SHIFTBRITE_MAX_Y);
    shiftbrite_push_image(shiftbrite_image, SHIFTBRITE_MAX_X, SHIFTBRITE_MAX_Y);
}

unsigned char * shiftbrite_get_image(int * x_out, int * y_out) {
    if (x_out != NULL) {
        *x_out = SHIFTBRITE_MAX_X;
    }
    if (y_out != NULL) {
        *y_out = SHIFTBRITE_MAX_Y;
    }
    return shiftbrite_image;
}

void shiftbrite_dot_correct(int lights) {
    int j;
    for(j = 0; j < lights; ++j) {
        shiftbrite_command(1, correct_r, correct_g, correct_b);
    }
    shiftbrite_delay_latch(lights);
}

