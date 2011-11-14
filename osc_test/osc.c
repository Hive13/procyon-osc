#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "uip/uip.h"
#include "string.h"
#include "osc.h"

// OSC 

void osc_parse(char * data, int length) {
    char addr[200];
    int offset = 0;
    char * fields[] = { "Address", "Pattern", 0 };
    int field = 0;

    for(field = 0; fields[field] && offset < length; ++field) {
        int i;
        UARTprintf("%s: %s\n", fields[field], data + offset);
        // Set 'offset' _after_ the null at the end of this string.
        while (offset < length && data[offset++]);
        // Pad to be a multiple of 4 bytes.
        offset = (offset + 3) & ~0x03;
    }
/*
    UARTprintf("Entered osc_parse\n");
    // (1) Get address pattern.
    for(i = 0; (i + offset) < length && (data + offset)[i]; ++i) {
        addr[i] = data[i];
    }
    if (i < length) {
        addr[i] = 0;
    }
    UARTprintf("OSC address: %s\n", addr);

    // (2) Get typetag.
    char typetag[200];
    for(i = 0; (i + offset) < length && (data + offset)[i]; ++i) {
        typetag[i] = data[i];
    }
    if (i < length) {
        typetag[i] = 0;
    }
    UARTprintf("OSC typetag: %s\n", typetag);
    // Pad to multiple of 4
    offset = (i + 3) & ~0x03;
*/
}

void osc_appcall(char * data, int length) {
    UARTprintf("Calling OSC routines\n");
    osc_parse(uip_appdata, uip_datalen());
}

