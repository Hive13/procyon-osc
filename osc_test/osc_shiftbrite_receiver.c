#include "osc_shiftbrite_receiver.h"
#include "shiftbrite.h"
#include "utils/uartstdio.h"
#include "uip-conf.h"
#include "string.h"

// The size of the screen in X and Y pixels; -1 if unknown.
static int shiftbrite_x = -1, shiftbrite_y = -1;
// The buffer which holds the screen image, shiftbrite_x*shiftbrite_y*3
// elements in size.
static unsigned char * shiftbrite_image = NULL;

void shiftbrite_osc_init() {
    if (shiftbrite_image != NULL) {
        UARTprintf("[WARNING] shiftbrite_osc_init called twice?\n");
    }
    shiftbrite_image = shiftbrite_get_image(&shiftbrite_x, &shiftbrite_y);
    if (shiftbrite_image == NULL) {
        UARTprintf("[ERROR] shiftbrite_get_image returned null pointer!\n");
    } else {
        int i;
        for(i = 0; i < shiftbrite_x*shiftbrite_y*3; ++i)
        {
            // Initialize to simple test pattern:
            shiftbrite_image[i] = i % 255;    
        }
       UARTprintf("shiftbrite_osc_init successful! Screen is %dx%d\n", shiftbrite_x, shiftbrite_y);
    }
}

void shiftbrite_osc_int_callback(int argNum, int value) {
    unsigned int px = 0, py = 0, component;
    UARTprintf("Got arg %d, int %d\n", argNum, value);
    
    // We treat argument number as being a linearized version of the pixel
    // location. Extract it out into X and Y pixel location.
    // We first divide by 3 because it's given as R,G,B triplets.
    px = ((argNum-1)/3) % shiftbrite_x;
    py = ((argNum-1)/3) / shiftbrite_y;
    component = (argNum-1) % 3;
    // N.B. We don't actually need to do this computation. It's just for
    // diagnostic purposes. The array uses the same linear indices, so all
    // we need to do is subtract one; at no point do we use direct X, Y,
    // or component indices on the array.
    UARTprintf("Linear coordinate looks like (%d,%d), component %d\n", px, py, component);
    if (argNum >= (shiftbrite_x*shiftbrite_y*3)) {
        UARTprintf("Too many arguments given in OSC message!\n");
    } else {
        //shiftbrite_image[argNum - 1] = value % 255;
    }

    // While a straight memcpy from the packet isn't possible because all of
    // our values are 32-bit, not 8-bit (and even using RGBA typetag which
    // uses 8 bits per channel, we still would have to throw out the alpha
    // channel first), we could optimize things a bit by just doing iteration
    // while we process the packet since things arrive in order anyhow.
} 

void shiftbrite_osc_blob_callback(int argNum, char * value, int length) {
    int maxLength = shiftbrite_x * shiftbrite_y * 3;
    if (length >= maxLength) {
        UARTprintf("[WARNING] Discarding %d extra bytes at end of image.\n", length - maxLength + 1);
        length = maxLength;
    } else if (length <= maxLength) {
        UARTprintf("[WARNING] Image too short by %d bytes.\n", maxLength - length + 1);
    }
    memcpy(shiftbrite_image, value, length);
    shiftbrite_dot_correct(shiftbrite_x*shiftbrite_y, 65, 50, 50);
    shiftbrite_push_image(shiftbrite_image, shiftbrite_x, shiftbrite_y);
}
