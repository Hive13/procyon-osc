//*****************************************************************************
// osc_test: OSC listener for Procyon board. This currently gets an address
// via DHCP as well.
// This is based on the enet_uip example from the Procyon board, which is
// itself based on some code from the uIP source and StellarisWare from TI.
//*****************************************************************************

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
#include "uip/uip.h"
#include "uip/uip_arp.h"
//#include "httpd/httpd.h"
#include "dhcpc/dhcpc.h"

#include "osc_shiftbrite_receiver.h"
#include "shiftbrite.h"
#include "osc.h"

// If HEARTBEAT is true, then the system will echo dots in the main event
// loop to signify that it has not crashed.
#define HEARTBEAT 0

extern struct hybrid_dhcpc_osc_state s;
void set_up_spi(int clock_speed);

int delay_usec = -1;

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Ethernet with uIP (enet_uip)</h1>
//!
//! This example application demonstrates the operation of the Stellaris
//! Ethernet controller using the uIP TCP/IP Stack.  DHCP is used to obtain
//! an Ethernet address.  A basic web site is served over the Ethernet port.
//! The web site displays a few lines of text, and a counter that increments
//! each time the page is sent.
//!
//! UART0, connected to the FTDI virtual COM port and running at 115,200,
//! 8-N-1, is used to display messages from this application.
//!
//! For additional details on uIP, refer to the uIP web page at:
//! http://www.sics.se/~adam/uip/
//
//*****************************************************************************

//*****************************************************************************
//
// Defines for setting up the system clock.
//
//*****************************************************************************
#define SYSTICKHZ               CLOCK_CONF_SECOND
#define SYSTICKMS               (1000 / SYSTICKHZ)
#define SYSTICKUS               (1000000 / SYSTICKHZ)
#define SYSTICKNS               (1000000000 / SYSTICKHZ)

//*****************************************************************************
//
// Macro for accessing the Ethernet header information in the buffer.
//
//*****************************************************************************
u8_t ucUIPBuffer[UIP_BUFSIZE + 2];
u8_t *uip_buf;

#define BUF                     ((struct uip_eth_hdr *)uip_buf)

//*****************************************************************************
//
// A set of flags.  The flag bits are defined as follows:
//
//     0 -> An indicator that a SysTick interrupt has occurred.
//     1 -> An RX Packet has been received.
//     2 -> A TX packet DMA transfer is pending.
//     3 -> A RX packet DMA transfer is pending.
//
//*****************************************************************************
#define FLAG_SYSTICK            0
#define FLAG_RXPKT              1
#define FLAG_TXPKT              2
#define FLAG_RXPKTPEND          3
static volatile unsigned long g_ulFlags;

//*****************************************************************************
//
// A system tick counter, incremented every SYSTICKMS.
//
//*****************************************************************************
volatile unsigned long g_ulTickCounter = 0;

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.  In this application uDMA is only used for USB,
// so only the first 8 channels are needed.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
tDMAControlTable g_sDMAControlTable[8];
#elif defined(ccs)
#pragma DATA_ALIGN(g_sDMAControlTable, 1024)
tDMAControlTable g_sDMAControlTable[8];
#else
tDMAControlTable g_sDMAControlTable[8] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// Default TCP/IP Settings for this application.
//
// Default to Link Local address ... (169.254.1.0 to 169.254.254.255).  Note:
// This application does not implement the Zeroconf protocol.  No ARP query is
// issued to determine if this static IP address is already in use.
//
// TODO:  Uncomment the following #define statement to enable STATIC IP
// instead of DHCP.
//
//*****************************************************************************
//#define USE_STATIC_IP

#ifndef DEFAULT_IPADDR0
#define DEFAULT_IPADDR0         169
#endif

#ifndef DEFAULT_IPADDR1
#define DEFAULT_IPADDR1         254
#endif

#ifndef DEFAULT_IPADDR2
#define DEFAULT_IPADDR2         19
#endif

#ifndef DEFAULT_IPADDR3
#define DEFAULT_IPADDR3         63
#endif

#ifndef DEFAULT_NETMASK0
#define DEFAULT_NETMASK0        255
#endif

#ifndef DEFAULT_NETMASK1
#define DEFAULT_NETMASK1        255
#endif

#ifndef DEFAULT_NETMASK2
#define DEFAULT_NETMASK2        0
#endif

#ifndef DEFAULT_NETMASK3
#define DEFAULT_NETMASK3        0
#endif

//*****************************************************************************
//
// UIP Timers (in MS)
//
//*****************************************************************************
#define UIP_PERIODIC_TIMER_MS   500
#define UIP_ARP_TIMER_MS        10000

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// The interrupt handler for the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Increment the system tick count.
    //
    g_ulTickCounter++;

    //
    // Indicate that a SysTick interrupt has occurred.
    //
    HWREGBITW(&g_ulFlags, FLAG_SYSTICK) = 1;
}

//*****************************************************************************
//
//! When using the timer module in UIP, this function is required to return
//! the number of ticks.  Note that the file "clock-arch.h" must be provided
//! by the application, and define CLOCK_CONF_SECONDS as the number of ticks
//! per second, and must also define the typedef "clock_time_t".
//
//*****************************************************************************
clock_time_t
clock_time(void)
{
    return((clock_time_t)g_ulTickCounter);
}

//*****************************************************************************
//
// The interrupt handler for the Ethernet interrupt.
//
//*****************************************************************************
void
EthernetIntHandler(void)
{
    unsigned long ulTemp;

    //
    // Read and Clear the interrupt.
    //
    ulTemp = ROM_EthernetIntStatus(ETH_BASE, false);
    ROM_EthernetIntClear(ETH_BASE, ulTemp);

    //
    // Check to see if an RX Interrupt has occurred.
    //
    if(ulTemp & ETH_INT_RX)
    {
        //
        // Indicate that a packet has been received.
        //
        HWREGBITW(&g_ulFlags, FLAG_RXPKT) = 1;

        //
        // Disable Ethernet RX Interrupt.
        //
        ROM_EthernetIntDisable(ETH_BASE, ETH_INT_RX);
    }

    //
    // Check to see if waiting on a DMA to complete.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_RXPKTPEND) == 1)
    {
        //
        // Verify the channel transfer is done
        //
        if(uDMAChannelModeGet(UDMA_CHANNEL_ETH0RX) == UDMA_MODE_STOP)
        {
            //
            // Indicate that a data has been read in.
            //
            HWREGBITW(&g_ulFlags, FLAG_RXPKTPEND) = 0;
        }
    }

    //
    // Check to see if the Ethernet TX uDMA channel was pending.
    //
    if(HWREGBITW(&g_ulFlags, FLAG_TXPKT) == 1)
    {
        //
        // Verify the channel transfer is done
        //
        if(uDMAChannelModeGet(UDMA_CHANNEL_ETH0TX) == UDMA_MODE_STOP)
        {
            //
            // Trigger the transmission of the data.
            //
            HWREG(ETH_BASE + MAC_O_TR) = MAC_TR_NEWTX;

            //
            // Indicate that a packet has been sent.
            //
            HWREGBITW(&g_ulFlags, FLAG_TXPKT) = 0;
        }
    }
}

//*****************************************************************************
//
// Callback for when DHCP client has been configured.
//
//*****************************************************************************
void
dhcpc_configured(const struct hybrid_dhcpc_osc_state *s)
{
    uip_sethostaddr(&s->ipaddr);
    uip_setnetmask(&s->netmask);
    uip_setdraddr(&s->default_router);
    UARTprintf("IP: %d.%d.%d.%d\n", s->ipaddr[0] & 0xff, s->ipaddr[0] >> 8,
               s->ipaddr[1] & 0xff, s->ipaddr[1] >> 8);
}

//*****************************************************************************
//
// Read a packet using DMA instead of directly reading the FIFO if the
// alignment will allow it.
//
//*****************************************************************************
long
EthernetPacketGetDMA(unsigned long ulBase, unsigned char *pucBuf, long lBufLen)
{
    unsigned long ulTemp;
    unsigned char pucData[4];
    unsigned char *pucBuffer;
    long lTempLen, lFrameLen;
    long lRemainder;
    int iIdx;

    //
    // Check the arguments.
    //
    ASSERT(ulBase == ETH_BASE);
    ASSERT(pucBuf != 0);
    ASSERT(lBufLen > 0);

    //
    // If the buffer is not aligned on an odd half-word then it cannot use DMA.
    // This is because the two packet length bytes are written in front of the
    // packet, and the packet data must have two bytes that can be pulled off
    // to become a word and leave the remainder of the buffer word aligned.
    //
    if(((unsigned long)pucBuf & 3) != 2)
    {
        //
        // If there is not proper alignment the packet must be sent without
        // using DMA.
        //
        return(ROM_EthernetPacketGetNonBlocking(ulBase, pucBuf, lBufLen));
    }

    //
    // Read WORD 0 from the FIFO, set the receive Frame Length and store the
    // first two bytes of the destination address in the receive buffer.
    //
    ulTemp = HWREG(ulBase + MAC_O_DATA);
    lFrameLen = (long)(ulTemp & 0xffff);
    pucBuf[0] = (unsigned char)((ulTemp >> 16) & 0xff);
    pucBuf[1] = (unsigned char)((ulTemp >> 24) & 0xff);

    //
    // The maximum DMA size is the frame size - the two bytes already read and
    // truncated to the nearest word size.
    //
    lTempLen = (lFrameLen - 2) & 0xfffffffc;
    lRemainder = (lFrameLen - 2) & 3;

    //
    // Don't allow writing beyond the end of the buffer.
    //
    if(lBufLen < lTempLen)
    {
        lRemainder = lTempLen - lBufLen;
        lTempLen =  lBufLen;
    }
    else if(lBufLen >= (lFrameLen - 2 + 3))
    {
        //
        // If there is room, just DMA the last word as well so that the
        // special copy after DMA is not required.
        //
        lRemainder = 0;
        lTempLen = lFrameLen - 2 + 3;
    }

    //
    // Mark the receive as pending.
    //
    HWREGBITW(&g_ulFlags, FLAG_RXPKTPEND) = 1;

    //
    // Set up the DMA to transfer the Ethernet header when a
    // packet is received
    //
    uDMAChannelTransferSet(UDMA_CHANNEL_ETH0RX, UDMA_MODE_AUTO,
                           (void *)(ETH_BASE + MAC_O_DATA),
                           &pucBuf[2], lTempLen>>2);
    uDMAChannelEnable(UDMA_CHANNEL_ETH0RX);

    //
    // Issue a software request to start the channel running.
    //
    uDMAChannelRequest(UDMA_CHANNEL_ETH0RX);

    //
    // Wait for the previous transmission to be complete.
    //
    while(HWREGBITW(&g_ulFlags, FLAG_RXPKTPEND) == 1)
    {
    }

    //
    // See if there are extra bytes to read into the buffer.
    //
    if(lRemainder)
    {
        //
        // If the remainder is more than 3 bytes then the buffer was never big
        // enough and data must be tossed.
        //
        if(lRemainder > 3)
        {
            //
            // Read any remaining WORDS (that did not fit into the buffer).
            //
            while(lRemainder > 0)
            {
                ulTemp = HWREG(ulBase + MAC_O_DATA);
                lRemainder -= 4;
            }
        }

        //
        // Read the last word from the FIFO.
        //
        *((unsigned long *)&pucData[0]) = HWREG(ulBase + MAC_O_DATA);

        //
        // The current buffer position is lTempLen plus the two bytes read
        // from the first word.
        //
        pucBuffer = &pucBuf[lTempLen + 2];

        //
        // Read off each individual byte and save it.
        //
        for(iIdx = 0; iIdx < lRemainder; iIdx++)
        {
            pucBuffer[iIdx] = pucData[iIdx];
        }
    }

    //
    // If frame was larger than the buffer, return the "negative" frame length.
    //
    lFrameLen -= 6;
    if(lFrameLen > lBufLen)
    {
        return(-lFrameLen);
    }

    //
    // Return the Frame Length
    //
    return(lFrameLen);
}

//*****************************************************************************
//
// Transmit a packet using DMA instead of directly writing the FIFO if the
// alignment will allow it.
//
//*****************************************************************************
static long
EthernetPacketPutDMA(unsigned long ulBase, unsigned char *pucBuf,
                     long lBufLen)
{
    unsigned long ulTemp;

    //
    // If the buffer is not aligned on an odd half-word then it cannot use DMA.
    // This is because the two packet length bytes are written in front of the
    // packet, and the packet data must have two bytes that can be pulled off
    // to become a word and leave the remainder of the buffer word aligned.
    //
    if(((unsigned long)pucBuf & 3) != 2)
    {
        //
        // If there is not proper aligment the packet must be sent without
        // using DMA.
        //
        return(ROM_EthernetPacketPut(ulBase, pucBuf, lBufLen));
    }

    //
    // Indicate that a packet is being sent.
    //
    HWREGBITW(&g_ulFlags, FLAG_TXPKT) = 1;

    //
    // Build and write WORD 0 (see format above) to the transmit FIFO.
    //
    ulTemp = (unsigned long)(lBufLen - 14);
    ulTemp |= (*pucBuf++) << 16;
    ulTemp |= (*pucBuf++) << 24;
    HWREG(ulBase + MAC_O_DATA) = ulTemp;

    //
    // Force an extra word to be transferred if the end of the buffer is not
    // aligned on a word boundary.  The math is actually lBufLen - 2 + 3 to
    // insure that the proper number of bytes are written.
    //
    lBufLen += 1;

    //
    // Configure the TX DMA channel to transfer the packet buffer.
    //
    uDMAChannelTransferSet(UDMA_CHANNEL_ETH0TX, UDMA_MODE_AUTO,
                           pucBuf, (void *)(ETH_BASE + MAC_O_DATA),
                           lBufLen>>2);

    //
    // Enable the Ethernet Transmit DMA channel.
    //
    uDMAChannelEnable(UDMA_CHANNEL_ETH0TX);

    //
    // Issue a software request to start the channel running.
    //
    uDMAChannelRequest(UDMA_CHANNEL_ETH0TX);

    //
    // Wait for the previous transmission to be complete.
    //
    while((HWREGBITW(&g_ulFlags, FLAG_TXPKT) == 1) &&
          EthernetSpaceAvail(ETH_BASE))
    {
    }

    //
    // Take back off the byte that we addeded above.
    //
    return(lBufLen - 1);
}

//*****************************************************************************
//
// This example demonstrates the use of the Ethernet Controller with the uIP
// TCP/IP stack.
//
//*****************************************************************************
int
main(void)
{
    uip_ipaddr_t ipaddr;
    static struct uip_eth_addr sTempAddr;
    long lPeriodicTimer, lARPTimer;
    unsigned long ulUser0, ulUser1;
    unsigned long ulTemp;
    int loopCount = 0;

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Adjust the pointer to be aligned on an odd half word address so that
    // DMA can be used.
    //
    uip_buf = (u8_t *)(((unsigned long)ucUIPBuffer + 3) & 0xfffffffe);

    //
    // Initialize the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInit(0);
    UARTprintf("\033[2JStarting osc_test...\n");

    // Compute a delay time for SysCtlClockGet():
    // SysCtlDelay = 3 cycles per iteration
    // t us * (1 s / 1e6 us) * (f cycles / s) * (1 iter / 3 cycles)
    //  = t * f / (3 * 1e6) iterations
    delay_usec = SysCtlClockGet() / (3 * 1000000);
    UARTprintf("One microsecond = %d cycles?\n", delay_usec);

    //
    // Enable the uDMA controller and set up the control table base.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    ROM_uDMAEnable();
    ROM_uDMAControlBaseSet(g_sDMAControlTable);

    //
    // Configure the DMA TX channel
    //
    uDMAChannelAttributeDisable(UDMA_CHANNEL_ETH0TX, UDMA_ATTR_ALL);
    uDMAChannelControlSet(UDMA_CHANNEL_ETH0TX,
                              UDMA_SIZE_32 | UDMA_SRC_INC_32 |
                              UDMA_DST_INC_NONE | UDMA_ARB_8);

    // Set up our SPI output, 200 kHz
    set_up_spi(200000);
   
    if (0) 
    {
        int px = 0, py = 0;
        unsigned char * img = shiftbrite_get_image(&px, &py);
        int lights = px * py;

        {
            int i = 0;
            int j = 0;

            // All but the lowest 8 bits of the number passed in are ignored
            while (1) {
                if (i % 10000 == 0) {
                    UARTprintf("Still alive...\n");
                }

                // Set the dot correction registers.
                // (This should not be a prerequisite to every single color
                // command. However, the ShiftBrite appears to have some
                // flakiness - particularly to minor electrical disturbances
                // such as touching a scope probe to a pin - which this
                // partially remedies, as it seems that the corruption is
                // identical to those cases where the color correction
                // registers have never been sent in the first place.)
                shiftbrite_dot_correct(lights);
                /*for(j = 0; j < lights; ++j) {
                    shiftbrite_command(1, 65, 50, 50);
                    //shiftbrite_command(1, 10, 10, 10);
                }
                shiftbrite_delay_latch(lights);*/

                // Make a simple test pattern:
                for(j = 0; j < lights; ++j) {
                    img[3*j + 0] = ((i >> 2) + 255*j/lights) % 255;
                    img[3*j + 1] = (255 * j) / (lights - 1);
                    img[3*j + 2] = 255 - img[3*j + 1];
                }
                shiftbrite_push_image(img, px, py);
                
                ++i;
            }
        }
    }


    //
    // Read the MAC address from the user registers.
    //
/*
    FlashUserGet(&ulUser0, &ulUser1);
    if((ulUser0 == 0xffffffff) || (ulUser1 == 0xffffffff))
    {
        //
        // We should never get here.  This is an error if the MAC address has
        // not been programmed into the device.  Exit the program.
        //
        UARTprintf("MAC Address Not Programmed!\n");
        while(1)
        {
        }
    }
*/

    // Hard-code MAC address:
    ulUser0 = 11934208;
    ulUser1 = 10503937;
    //
    // Enable and Reset the Ethernet Controller.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);

    //
    // Enable Port F for Ethernet LEDs.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinConfigure(GPIO_PF2_LED1);
    GPIOPinConfigure(GPIO_PF3_LED0);
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //
    // Configure SysTick for a periodic interrupt.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / SYSTICKHZ);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Configure the DMA channel for Ethernet receive.
    //
    uDMAChannelAttributeDisable(UDMA_CHANNEL_ETH0RX, UDMA_ATTR_ALL);
    uDMAChannelControlSet(UDMA_CHANNEL_ETH0RX,
                          UDMA_SIZE_32 | UDMA_SRC_INC_NONE |
                          UDMA_DST_INC_32 | UDMA_ARB_8);

    //
    // Initialize the Ethernet Controller and disable all Ethernet Controller
    // interrupt sources.
    //
    ROM_EthernetIntDisable(ETH_BASE, (ETH_INT_PHY | ETH_INT_MDIO |
                                      ETH_INT_RXER | ETH_INT_RXOF |
                                      ETH_INT_TX | ETH_INT_TXER | ETH_INT_RX));
    ulTemp = ROM_EthernetIntStatus(ETH_BASE, false);
    ROM_EthernetIntClear(ETH_BASE, ulTemp);

    //
    // Initialize the Ethernet Controller for operation.
    //
    ROM_EthernetInitExpClk(ETH_BASE, ROM_SysCtlClockGet());

    //
    // Configure the Ethernet Controller for normal operation.
    // - Full Duplex
    // - TX CRC Auto Generation
    // - TX Padding Enabled
    //
    ROM_EthernetConfigSet(ETH_BASE, (ETH_CFG_TX_DPLXEN | ETH_CFG_TX_CRCEN |
                                     ETH_CFG_TX_PADEN));
    //
    // Wait for the link to become active.
    //
    UARTprintf("Waiting for Link\n");
    while((ROM_EthernetPHYRead(ETH_BASE, PHY_MR1) & 0x0004) == 0)
    {
    }
    UARTprintf("Link Established\n");

    //
    // Enable the Ethernet Controller.
    //
    ROM_EthernetEnable(ETH_BASE);

    //
    // Enable the Ethernet interrupt.
    //
    ROM_IntEnable(INT_ETH);

    //
    // Enable the Ethernet RX Packet interrupt source.
    //
    ROM_EthernetIntEnable(ETH_BASE, ETH_INT_RX);

    //
    // Enable all processor interrupts.
    //
    ROM_IntMasterEnable();

    //
    // Initialize the uIP TCP/IP stack.
    //
    uip_init();
#ifdef USE_STATIC_IP
    uip_ipaddr(ipaddr, DEFAULT_IPADDR0, DEFAULT_IPADDR1, DEFAULT_IPADDR2,
               DEFAULT_IPADDR3);
    uip_sethostaddr(ipaddr);
    UARTprintf("IP: %d.%d.%d.%d\n", DEFAULT_IPADDR0, DEFAULT_IPADDR1,
               DEFAULT_IPADDR2, DEFAULT_IPADDR3);
    uip_ipaddr(ipaddr, DEFAULT_NETMASK0, DEFAULT_NETMASK1, DEFAULT_NETMASK2,
               DEFAULT_NETMASK3);
    uip_setnetmask(ipaddr);
#else
    uip_ipaddr(ipaddr, 0, 0, 0, 0);
    uip_sethostaddr(ipaddr);
    UARTprintf("Waiting for IP address...\n");
    uip_ipaddr(ipaddr, 0, 0, 0, 0);
    uip_setnetmask(ipaddr);
#endif

    //
    // Convert the 24/24 split MAC address from NV ram into a 32/16 split MAC
    // address needed to program the hardware registers, then program the MAC
    // address into the Ethernet Controller registers.
    //
    sTempAddr.addr[0] = ((ulUser0 >>  0) & 0xff);
    sTempAddr.addr[1] = ((ulUser0 >>  8) & 0xff);
    sTempAddr.addr[2] = ((ulUser0 >> 16) & 0xff);
    sTempAddr.addr[3] = ((ulUser1 >>  0) & 0xff);
    sTempAddr.addr[4] = ((ulUser1 >>  8) & 0xff);
    sTempAddr.addr[5] = ((ulUser1 >> 16) & 0xff);

	UARTprintf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
		sTempAddr.addr[0], sTempAddr.addr[1], sTempAddr.addr[2],
		sTempAddr.addr[3], sTempAddr.addr[4], sTempAddr.addr[5]); 

    //
    // Program the hardware with it's MAC address (for filtering).
    //
    ROM_EthernetMACAddrSet(ETH_BASE, (unsigned char *)&sTempAddr);
    uip_setethaddr(sTempAddr);

    //
    // Initialize the TCP/IP Application (e.g. web server).
    //
	/*UARTprintf("Calling httpd_init()...");
    httpd_init();
	UARTprintf("done!\n");*/

#ifndef USE_STATIC_IP
    //
    // Initialize the DHCP Client Application.
    //
    UARTprintf("Making DHCP request...\n");

    dhcpc_init(&sTempAddr.addr[0], 6);
    dhcpc_request();
#endif

    // Set up ShiftBrite OSC routines
    {
        shiftbrite_osc_init();
    }

    // Set up OSC callbacks
    {
        g_osc_state.intCallback = &intEcho;
        //g_osc_state.intCallback = &shiftbrite_osc_int_callback;
        g_osc_state.floatCallback = &floatEcho;
        //g_osc_state.stringCallback = &stringEcho;
        //g_osc_state.stringCallback = &shiftbrite_osc_blob_callback;
        //g_osc_state.blobCallback = &blobEcho;
        g_osc_state.blobCallback = &shiftbrite_osc_blob_callback;
    }

    // Set up OSC listener (hopefully)
    {
        uip_ipaddr_t addr;
        uip_ipaddr(addr, 255, 255, 255, 255);
        s.udpConn = uip_udp_new(&addr, 0);
        if(s.udpConn != NULL) {
            uip_udp_bind(s.udpConn, HTONS(OSC_LISTEN_PORT));
            UARTprintf("Listening for OSC, port %d\n", OSC_LISTEN_PORT);
        } else {
            UARTprintf("OSC failed! (port %d)!\n", OSC_LISTEN_PORT);
        }
    }

    //
    // Main Application Loop.
    //
    lPeriodicTimer = 0;
    lARPTimer = 0;
    loopCount = 0;
    while(true)
    {
        //
        // Wait for an event to occur.  This can be either a System Tick event,
        // or an RX Packet event.
        //
        while(!g_ulFlags)
        {
        }

        //
        // If SysTick, Clear the SysTick interrupt flag and increment the
        // timers.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_SYSTICK) == 1)
        {
            HWREGBITW(&g_ulFlags, FLAG_SYSTICK) = 0;
            lPeriodicTimer += SYSTICKMS;
            lARPTimer += SYSTICKMS;
        }

        //
        // Check for an RX Packet and read it.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_RXPKT))
        {
            //
            // Get the packet and set uip_len for uIP stack usage.
            //
            uip_len = (unsigned short)EthernetPacketGetDMA(ETH_BASE, uip_buf,
                                                           sizeof(ucUIPBuffer));
            //
            // Clear the RX Packet event and re-enable RX Packet interrupts.
            //
            if(HWREGBITW(&g_ulFlags, FLAG_RXPKT) == 1)
            {
                HWREGBITW(&g_ulFlags, FLAG_RXPKT) = 0;
                ROM_EthernetIntEnable(ETH_BASE, ETH_INT_RX);
            }

            //
            // Process incoming IP packets here.
            //
            if(BUF->type == htons(UIP_ETHTYPE_IP))
            {
                uip_arp_ipin();
                uip_input();

                //
                // If the above function invocation resulted in data that
                // should be sent out on the network, the global variable
                // uip_len is set to a value > 0.
                //
                if(uip_len > 0)
                {
                    uip_arp_out();
                    EthernetPacketPutDMA(ETH_BASE, uip_buf, uip_len);
                    uip_len = 0;
                }
            }

            //
            // Process incoming ARP packets here.
            //
            else if(BUF->type == htons(UIP_ETHTYPE_ARP))
            {
                uip_arp_arpin();

                //
                // If the above function invocation resulted in data that
                // should be sent out on the network, the global variable
                // uip_len is set to a value > 0.
                //
                if(uip_len > 0)
                {
                    EthernetPacketPutDMA(ETH_BASE, uip_buf, uip_len);
                    uip_len = 0;
                }
            }
        }

        //
        // Process TCP/IP Periodic Timer here.
        //
        if(lPeriodicTimer > UIP_PERIODIC_TIMER_MS)
        {
            lPeriodicTimer = 0;
            for(ulTemp = 0; ulTemp < UIP_CONNS; ulTemp++)
            {
                uip_periodic(ulTemp);

                //
                // If the above function invocation resulted in data that
                // should be sent out on the network, the global variable
                // uip_len is set to a value > 0.
                //
                if(uip_len > 0)
                {
                    uip_arp_out();
                    EthernetPacketPutDMA(ETH_BASE, uip_buf, uip_len);
                    uip_len = 0;
                }
            }

#if UIP_UDP
            for(ulTemp = 0; ulTemp < UIP_UDP_CONNS; ulTemp++)
            {
                uip_udp_periodic(ulTemp);

                //
                // If the above function invocation resulted in data that
                // should be sent out on the network, the global variable
                // uip_len is set to a value > 0.
                //
                if(uip_len > 0)
                {
                    uip_arp_out();
                    EthernetPacketPutDMA(ETH_BASE, uip_buf, uip_len);
                    uip_len = 0;
                }
            }
#endif

#if 0
            {
                int x,y;
                unsigned char * img = shiftbrite_get_image(&x, &y);
                int pos = loopCount % (x*y*3);
                int i = 0;
                for(i = 0; i < x*y*3; ++i) {
                    img[i] = 0;
                }
                img[pos] = (unsigned char) 255;
                shiftbrite_refresh();
            }
#endif
            // If not doing anything else, refresh the display.
#if 1
            shiftbrite_refresh();
            UARTprintf("Refresh (t=%d)\n", loopCount);
#endif

            loopCount++;
        }

        //
        // Process ARP Timer here.
        //
        if(lARPTimer > UIP_ARP_TIMER_MS)
        {
            lARPTimer = 0;
            uip_arp_timer();
        }
    }
}

void udp_appcall()
{
    #if HEARTBEAT
    UARTprintf(".");
    #endif

    // Do some OSC-specific processing
    if (uip_newdata()) {
		UARTprintf("%d bytes in\n", uip_datalen());
        UARTprintf("lport %d, rport %d\n", ntohs(uip_udp_conn->lport), ntohs(uip_udp_conn->rport));
        switch(ntohs(uip_udp_conn->lport)) {
        case OSC_LISTEN_PORT:
            osc_appcall(uip_appdata, uip_datalen());
            break;
        default:
            break;
        }
    }

    // Pass onto the DHCP code, which needs to run whether or not any new data
    // has arrived (presumably, it will handle this by itself)
    dhcpc_appcall();
}

void uip_log(char *m) {
    UARTprintf(m);
}

//Waste cycle delay function
void myDelay(unsigned long delay)
{ 
	while(delay)
	{ 
		delay--;
		__asm__ __volatile__("mov r0,r0");
	}
}

// This functionality should be moved elsewhere!
void set_up_spi(int clock_speed) {
    //
    // Set up SSI0 for SPI.
    //
    UARTprintf("Attempting to enable SPI with %d Hz clock...\n", clock_speed);
    // Set up PA2 (Clk), PA4 (Rx), PA5 (Tx) to use SSI0 peripheral.
    // (Leave out PA5 (Fss) because we want to toggle it manually)
    ROM_GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_5);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
    // Set clock to 8 bits & the given rate in Hz
    ROM_SSIConfigSetExpClk(SSI0_BASE, ROM_SysCtlClockGet(),
        SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, clock_speed, 8);
    // Set this delay carefully when more LEDs are in the chain

    ROM_SSIEnable(SSI0_BASE);
    // Set PA3 to GPIO as we must manually set this latch
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_3);
    UARTprintf("Enabled SPI!\n");
}


