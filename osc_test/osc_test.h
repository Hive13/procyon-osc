#ifndef __OSCTEST_H__
#define __OSCTEST_H__

#include "uip_timer.h"
#include "pt.h"

#define OSC_LISTEN_PORT 12000

// This structure is a combination of the fields used in the original DHCP
// structure, and some additional fields for handling OSC data. All the DHCP
// fields were kept the same so that the DHCP code could remain mostly
// unmodified.
struct hybrid_dhcpc_osc_state {
  // DHCP fields
  struct pt pt;
  char state;
  struct uip_udp_conn *conn;
  struct timer timer;
  u16_t ticks;
  const void *mac_addr;
  int mac_len;
  
  u8_t serverid[4];

  u16_t lease_time[2];
  u16_t ipaddr[2];
  u16_t netmask[2];
  u16_t dnsaddr[2];
  u16_t default_router[2];

  // OSC fields
  struct uip_udp_conn * udpConn;
};

// Placeholder struct for TCP application state. 'a' is just to suppress
// compiler warnings about the empty struct.
struct tcp_state {
    int a;
};



#endif /* __OSCTEST_H__ */

