/**
 * Project-specific configuration options for uIP
 */

#ifndef __UIP_CONF_H__
#define __UIP_CONF_H__

#include "inc/lm3s9b90.h"
#include <inttypes.h>

// FIXME: Where is this definition?
#ifndef NULL
#define NULL (void *)0
#endif

// 8-bit datatype
typedef uint8_t u8_t;

// 16-bit datatype
typedef uint16_t u16_t;

// Statistics datatype
typedef unsigned short uip_stats_t;

// Maximum number of TCP connections.
#define UIP_CONF_MAX_CONNECTIONS 40

// Maximum number of listening TCP ports.
#define UIP_CONF_MAX_LISTENPORTS 40

// uIP buffer size.
#define UIP_CONF_BUFFER_SIZE     420

// CPU byte order.
#define UIP_CONF_BYTE_ORDER      LITTLE_ENDIAN

// Logging on or off
#define UIP_CONF_LOGGING         1

// UDP support on or off
#define UIP_CONF_UDP             1

// UDP checksums on or off
#define UIP_CONF_UDP_CHECKSUMS   1

// uIP statistics on or off
#define UIP_CONF_STATISTICS      1

#define UIP_CONF_EXTERNAL_BUFFER 1

#include "osc_test.h"
#include "dhcpc.h"

typedef struct tcp_state uip_tcp_appstate_t;
typedef struct hybrid_dhcpc_osc_state uip_udp_appstate_t;

void udp_appcall();
// This function is defined elsewhere to use UARTprintf so that UIP_LOG can
// generate visible output:
void uip_log(char *m);
void set_led(int status);

#define UIP_APPCALL udp_appcall
#define UIP_UDP_APPCALL udp_appcall

#endif /* __UIP_CONF_H__ */

