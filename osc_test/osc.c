#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "uip/uip.h"
#include <string.h>
#include <stdlib.h>
#include "osc.h"

// Endianness conversion macro
#ifndef htonl
    #define htonl(a)                    \
        ((((a) >> 24) & 0x000000ff) |   \
         (((a) >>  8) & 0x0000ff00) |   \
         (((a) <<  8) & 0x00ff0000) |   \
         (((a) << 24) & 0xff000000))
#endif

#ifndef ntohl
    #define ntohl(a)    htonl((a))
#endif

//
// Try to parse an OSC packet.
//
void osc_parse(char * data, int dataLength) {
    int offset = 0;

    // Field names for logging purposes. End with a null.
    char * fields[] = { "Address", "Typetag", 0 };

    char * addr = NULL;
    char * typetag = NULL;
    // Pointers in which to put data from the respective field in 'fields'.
    char ** destinations[] = { &addr, &typetag, NULL };
    
    UARTprintf("Processing OSC...\n");

    // Get the address pattern and typetag from the packet.
    int field = 0;
    for(field = 0; fields[field] && offset < dataLength; ++field) {
        // Set the destination pointers.
        if (destinations[field]) {
            *(destinations[field]) = data + offset;
        }

        // Set 'offset' _after_ the null at the end of this string.
        while (offset < dataLength && data[offset++]);

        // Pad to be a multiple of 4 bytes (as OSC strings must be)
        offset = (offset + 3) & ~0x03;
    }

    UARTprintf("%s: %s\n", fields[0], addr);
    UARTprintf("%s: %s\n", fields[1], typetag);

    g_osc_state.addressPattern = addr;

    // (typetag - data) is an adjustment for how much our offsets change when
    // we're referring to the typetag pointer rather than 'data'.
    osc_dispatch_callbacks(typetag, offset - (typetag - data), dataLength - (typetag - data));
}

//
// Process the arguments given a pointer to a typetag field (including comma).
// Specify in 'argOffset' where the arguments start.
//
void osc_dispatch_callbacks(char * typetag, int argOffset, int maxLength) {
    int * intVal = NULL;

    int argNum = 1;

    UARTprintf("Packet:\n");
    printHexDump(typetag, 32);
    UARTprintf("osc_dispatch_callbacks(\"%s\", %d, %d)\n", typetag, argOffset, maxLength);

    if (typetag[0] != ',') {
        UARTprintf("[WARNING] This lacks a comma and is thus not a typetag?\n");
    }
    while (typetag[argNum] && argNum < maxLength) {
        switch(typetag[argNum]) {
        case 'i': // 32-bit integer
            intVal = (int *) (typetag + argOffset);
            g_osc_state.intCallback(argNum, ntohl(*intVal));
            argOffset += 4;
            break;
        case 'f': // 32-bit float
            g_osc_state.floatCallback(argNum, bigEndianFloat(typetag + argOffset));
            argOffset += 4;
            break;
        case 'S': // Symbol
        case 's': // String
            g_osc_state.stringCallback(argNum, typetag + argOffset, maxLength - argOffset);
            // For some reason, strnlen is implicitly declared here though
            // I've #included string.h. The code still links and runs though.
            argOffset += strnlen(typetag + argOffset, maxLength - argOffset);
            // Pad to be a multiple of 4 bytes (as OSC strings must be)
            argOffset = (argOffset + 3) & ~0x03;
            break;
        case 'b': // Binary blob (32-bit size comes first)
            // This value gives the length of the blob in bytes.
            intVal = (int *) (typetag + argOffset);
            argOffset += 4;
            g_osc_state.blobCallback(argNum, typetag + argOffset, ntohl(*intVal));
            argOffset += *intVal;
            // Pad to be a multiple of 4 bytes (as blob data must also be)
            argOffset = (argOffset + 3) & ~0x03;
            break;
        case 'h':
            // Untested (oscP5 won't send longs?)
            {
                // long val = bigEndianLong(typetag + argOffset);
                // Can't print longs this way:
                // UARTprintf("Arg %d: Long %ld\n", argNum, val);
            }
            argOffset += 8;
            break;
        case 't':
            UARTprintf("[ERROR] Arg %d is a timetag; not implemented!\n", argNum);
            break;
        case 'd':
            // Untested
            {
                //double val = bigEndianDouble(typetag + argOffset);
                UARTprintf("Arg %d: Double (not printing value)\n", argNum);
            }
            argOffset += 8;
            break;
        case 'c':
            // Character is ASCII, but represented in 32 bits
            {
                int * tmp = (int *) (typetag + argOffset);
                UARTprintf("Arg %d: ASCII character %d\n", argNum, ntohl(*tmp));
            }
            argOffset += 4;
            break;
        case 'r':
            // Untested
            UARTprintf("Arg %d: RGBA color ", argNum);
            printHexDump(typetag + argOffset, 4);
            argOffset += 4;
            // {
            //    intVal = (int *) (typetag + argOffset);
            //    ntohl(*intVal) ...
            // }
            break;
        case 'm':
            // Untested
            UARTprintf("Arg %d: 4 bytes of MIDI data, ", argNum);
            printHexDump(typetag + argOffset, 4);
            argOffset += 4;
            break;
        // For args T, F, N, I, no bytes are allocated.
        case 'T':
            UARTprintf("Arg %d: Boolean true\n", argNum);
            break;
        case 'F':
            UARTprintf("Arg %d: Boolean false\n", argNum);
            break;
        case 'N':
            // Untested
            UARTprintf("Arg %d: Nil\n", argNum);
            break;
        case 'I':
            // Untested
            UARTprintf("Arg %d: Infinitum\n", argNum);
            break;
        // Arrays (not handled):
        case '[':
        case ']':
            UARTprintf("Arrays are not supported! \n", argNum);
            break;
        default:
            {
                char tmp[2];
                tmp[1] = 0;
                tmp[0] = typetag[argNum];
                UARTprintf("Arg %d has unknown typetag %s\n", argNum, tmp);
            }
            break;
        }
        ++argNum;
    }
}

void osc_appcall(char * data, int length) {
    UARTprintf("Calling OSC routines\n");
    osc_parse(uip_appdata, uip_datalen());
}

//
// Example callbacks
//
void intEcho(int argNum, int value) {
    UARTprintf("Arg %d: int %d\n", argNum, value);
}

void floatEcho(int argNum, float value) {
    // Full printf/sprintf is not available on this embedded environment, so
    // %f is not available and we have to rig up our own:
    float mantissa = value - (int)value;
    int d1 = mantissa * 10;
    mantissa = 10 * mantissa - d1;
    int d2 = mantissa * 10;
    mantissa = 10 * mantissa - d2;
    int d3 = mantissa * 10;
    UARTprintf("Arg %d: float %d.%d%d%d\n", argNum, (int)value, d1,d2,d3);
}

void stringEcho(int argNum, char * value, int length) {
    UARTprintf("Arg %d: string %s\n", argNum, value);
}

void blobEcho(int argNum, char * value, int length) {
    UARTprintf("Arg %d: blob with %d bytes\n", argNum, length);
}


//
// Endianness conversion
//

// Return the big-endian 32-bit float at the 4 bytes starting at 'ptr'
float bigEndianFloat(char * ptr) {
    float retVal;
    reverseBuffer(ptr, (char *) &retVal, 4);
    return retVal;
}

// Return the big-endian 64-bit integer at the 8 bytes starting at 'ptr'
long bigEndianLong(char * ptr) {
    long retVal;
    reverseBuffer(ptr, (char *) &retVal, 8);
    return retVal;
}

// Return the big-endian 64-bit integer at the 8 bytes starting at 'ptr'
double bigEndianDouble(char * ptr) {
    double retVal;
    reverseBuffer(ptr, (char *) &retVal, 8);
    return retVal;
}

void reverseBuffer(char * src, char * dest, int size) {
    int i;
    for(i = 0; i < size; ++i) {
        dest[i] = src[size - i - 1];
    }
}

// Print a hex dump of 'bytes' bytes going into the buffer 't'. Note that this
// has no protection whatsoever if that buffer is shorter than the size given!
// It will print 16 bytes per line.
void printHexDump(char * t, int bytes)
{
    int i;
    for(i = 0; i < bytes; ++i) {
        UARTprintf("%02x ", t[i]);
        // 1 space, 2 spaces, & newline every 4, 8, & 16 bytes.
        if (!((i + 1) % 4))  UARTprintf(" ");
        if (!((i + 1) % 8))  UARTprintf("  ");
        if (!((i + 1) % 16)) UARTprintf("\n");
    }
    if (bytes % 16) UARTprintf("\n");
}
