/***************************************************************************
*
*   Copyright(c) Jeff V. Merkey 1997-2019.  All rights reserved.
*   Open CWorthy Look Alike Terminal Library.
*
*   IFCON Linux Example Program
*
**************************************************************************/

#define __GNU_SOURCE

#include "cworthy.h"

#define IPV6_ADDR_ANY		0x0000U
#define IPV6_ADDR_UNICAST	0x0001U
#define IPV6_ADDR_MULTICAST	0x0002U
#define IPV6_ADDR_ANYCAST	0x0004U
#define IPV6_ADDR_LOOPBACK	0x0010U
#define IPV6_ADDR_LINKLOCAL	0x0020U
#define IPV6_ADDR_SITELOCAL	0x0040U
#define IPV6_ADDR_COMPATv4	0x0080U
#define IPV6_ADDR_SCOPE_MASK	0x00f0U
#define IPV6_ADDR_MAPPED	0x1000U
#define IPV6_ADDR_RESERVED	0x2000U

#define NUD_INCOMPLETE  0x01
#define NUD_REACHABLE   0x02
#define NUD_STALE       0x04
#define NUD_DELAY       0x08
#define NUD_PROBE       0x10
#define NUD_FAILED      0x20

#define NUD_NOARP       0x40
#define NUD_PERMANENT   0x80
#define NUD_NONE        0x00

#define NTF_PROXY       0x08
#define NTF_ROUTER      0x80
#define NTF_02          0x02
#define NTF_04          0x04

static int INET6_resolve(char *name, struct sockaddr_in6 *sin6)
{
    struct addrinfo req, *ai;
    int s;

    memset(&req, '\0', sizeof req);
    req.ai_family = AF_INET6;
    if ((s = getaddrinfo(name, NULL, &req, &ai))) {
	return -1;
    }
    memcpy(sin6, ai->ai_addr, sizeof(struct sockaddr_in6));
    freeaddrinfo(ai);
    return (0);
}

#ifndef IN6_IS_ADDR_UNSPECIFIED
#define IN6_IS_ADDR_UNSPECIFIED(a) \
        (((__u32 *) (a))[0] == 0 && ((__u32 *) (a))[1] == 0 && \
         ((__u32 *) (a))[2] == 0 && ((__u32 *) (a))[3] == 0)
#endif


static int INET6_rresolve(char *name, struct sockaddr_in6 *sin6, int numeric)
{
    int s;

    /* Grmpf. -FvK */
    if (sin6->sin6_family != AF_INET6) {
	errno = EAFNOSUPPORT;
	return (-1);
    }
    if (numeric & 0x7FFF) {
	inet_ntop(AF_INET6, &sin6->sin6_addr, name, 80);
	return (0);
    }
    if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
        if (numeric & 0x8000)
	    strcpy(name, "default");
	else
	    strcpy(name, "*");
	return (0);
    }

    if ((s = getnameinfo((struct sockaddr *) sin6, sizeof(struct sockaddr_in6),
			 name, 255 /* !! */ , NULL, 0, 0))) {
	return -1;
    }
    return (0);
}


void INET6_reserror(char *text)
{
    herror(text);
}

char *INET6_print(unsigned char *ptr)
{
    static char name[80];

    inet_ntop(AF_INET6, (struct in6_addr *) ptr, name, 80);
    return name;
}


static char *INET6_sprint(struct sockaddr *sap, int numeric)
{
    static char buf[128];

    if (sap->sa_family == 0xFFFF || sap->sa_family == 0)
	return strncpy(buf, ("[NONE SET]"), sizeof(buf));
    if (INET6_rresolve(buf, (struct sockaddr_in6 *) sap, numeric) != 0)
	return strncpy(buf, ("[UNKNOWN]"), sizeof(buf));
    return (buf);
}

static int INET6_getsock(char *bufp, struct sockaddr *sap)
{
    struct sockaddr_in6 *sin6;

    sin6 = (struct sockaddr_in6 *) sap;
    sin6->sin6_family = AF_INET6;
    sin6->sin6_port = 0;

    if (inet_pton(AF_INET6, bufp, sin6->sin6_addr.s6_addr) <= 0)
	return (-1);

    return 16;
}

static int INET6_input(int type, char *bufp, struct sockaddr *sap)
{
    switch (type) {
    case 1:
	return (INET6_getsock(bufp, sap));
    default:
	return (INET6_resolve(bufp, (struct sockaddr_in6 *) sap));
    }
}

#define FLAG_EXT                 3
#define FLAG_NUM_HOST            4
#define FLAG_NUM_PORT            8
#define FLAG_NUM_USER           16
#define FLAG_NUM  (FLAG_NUM_HOST|FLAG_NUM_PORT|FLAG_NUM_USER)
#define FLAG_SYM                32
#define FLAG_CACHE              64
#define FLAG_FIB               128
#define FLAG_VERBOSE           256
#define RTF_UP              0x0001
#define RTF_GATEWAY         0x0002
#define RTF_HOST            0x0004
#define RTF_REINSTATE       0x0008
#define RTF_DYNAMIC         0x0010
#define RTF_MODIFIED        0x0020
#define RTF_MTU             0x0040
#ifndef RTF_MSS
#define RTF_MSS            RTF_MTU
#endif
#define RTF_WINDOW          0x0080
#define RTF_IRTT            0x0100
#define RTF_REJECT          0x0200
#ifndef RTF_DEFAULT
#define RTF_DEFAULT     0x00010000
#endif
#define RTF_ALLONLINK   0x00020000
#ifndef RTF_ADDRCONF
#define RTF_ADDRCONF    0x00040000
#endif
#define RTF_NONEXTHOP   0x00200000
#define RTF_EXPIRES     0x00400000
#define RTF_CACHE       0x01000000
#define RTF_FLOW        0x02000000
#define RTF_POLICY      0x04000000
#define RTF_LOCAL       0x80000000
#define E_NOTFOUND	         8
#define E_SOCK		         7
#define E_LOOKUP	         6
#define E_VERSION	         5
#define E_USAGE		         4
#define E_OPTERR	         3
#define E_INTERN	         2
#define E_NOSUPP	         1

int active = 0;
int netsumactive = 0;
int networkactive = 0;
int menu, mainportal, logportal = -1;
pthread_t pstat;
pthread_t netsumstat;
pthread_t networkstat;
pthread_t plog;

typedef struct _ARPTYPE
{
   int type;
   const char *name;
} ARPTYPE;

ARPTYPE arptype[]={
   0, "NET/ROM Pseudo",
   1, "Ethernet",
   2, "Experimental Ethernet",
   3, "AX.25 Level 2",
   4, "PROnet Token Ring",
   5, "Chaosnet",
   6, "IEEE 802.2 Ethernet/TR/TB",
   7, "ARCnet",
   8, "APPLEtalk",
   15, "Frame Relay DLCI",
   19, "ATM",
   23, "Metricom STRIP",
   24, "IEEE 1394 IPv4 - RFC 2734",
   27, "EUI-64",
   32, "InfiniBand",
   256, "SLIP",
   257, "CSLIP",
   258, "SLIP6",
   259, "CSLIP6",
   260, "Notional KISS",
   264, "ADAPT",
   270, "ROSE",
   271, "CCITT X.25",
   272, "Boards with X.25 in Firmware",
   280, "Controller Area Network",
   512, "PPP",
   513, "Cisco HDLC",
   516, "LAPB",
   517, "Digital's DDCMP Protocol",
   518, "Raw HDLC",
   768, "IPIP Tunnel",
   769, "IP6IP6 Tunnel",
   770, "Frame Relay Access Device",
   771, "SKIP VIF",
   772, "Loopback Device",
   773, "Localtalk Device",
   774, "Fiber Distributed Data Interface",
   775, "AP1000 BIF",
   776, "IPv6-in-IPv4",
   777, "IP over DDP Tunnel",
   778, "GRE over IP",
   779, "PIMSM Interface",
   780, "High Performance Parallel Interface",
   781, "Nexus 64Mbps Ash",
   782, "Acorn Econet",
   783, "Linux-IrDA",
   784, "Point to Point Fibrechannel",
   785, "Fibrechannel Arbitrated Loop",
   786, "Fibrechannel Public Loop",
   787, "Fibrechannel Fabric",
   788, "Fibrechannel Media Type 788",
   789, "Fibrechannel Media Type 789",
   790, "Fibrechannel Media Type 790",
   791, "Fibrechannel Media Type 791",
   792, "Fibrechannel Media Type 792",
   793, "Fibrechannel Media Type 793",
   794, "Fibrechannel Media Type 794",
   795, "Fibrechannel Media Type 795",
   796, "Fibrechannel Media Type 796",
   797, "Fibrechannel Media Type 797",
   798, "Fibrechannel Media Type 798",
   799, "Fibrechannel Media Type 799",
   800, "Magic Type Ident For TR",
   801, "IEEE 802.11",
   802, "IEEE 802.11 + Prism2 Header",
   803, "IEEE 802.11 + Radiotap Header",
   0xFFFF, "Void Type",
   0xFFFE, "Zero Header Device",
};

const char *get_arp_type(int type)
{
   unsigned int i;

   for (i=0; i < (sizeof(arptype) / (sizeof(const char *) + sizeof(int)));
        i++)
   {
      if (arptype[i].type == type)
         return arptype[i].name;
   }
   return "Unknown Device";
}


static int ipx_getaddr(int sock, int ft, struct ifreq *ifr)
{
    ((struct sockaddr_ipx *) &ifr->ifr_addr)->sipx_type = ft;
    return ioctl(sock, SIOCGIFADDR, ifr);
}

#if (IPX_NODE_LEN != 6)
#error "IPX_NODE_LEN != 6"
#endif

static char *IPX_print(unsigned char *ptr)
{
    static char buf[64];
    struct sockaddr_ipx *sipx = (struct sockaddr_ipx *) (ptr - 2);
    int t;

    for (t = IPX_NODE_LEN; t; t--)
	if (sipx->sipx_node[t - 1])
	    break;

    if (t && ntohl(sipx->sipx_network))
	snprintf(buf, sizeof(buf), "%08lX:%02X%02X%02X%02X%02X%02X",
		 (long int) ntohl(sipx->sipx_network),
		 (int) sipx->sipx_node[0], (int) sipx->sipx_node[1],
		 (int) sipx->sipx_node[2], (int) sipx->sipx_node[3],
		 (int) sipx->sipx_node[4], (int) sipx->sipx_node[5]);
    else if (!t && ntohl(sipx->sipx_network))
	snprintf(buf, sizeof(buf), "%08lX",
		 (long int) ntohl(sipx->sipx_network));
    else if (t && !ntohl(sipx->sipx_network))
	snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X",
		 (int) sipx->sipx_node[0], (int) sipx->sipx_node[1],
		 (int) sipx->sipx_node[2], (int) sipx->sipx_node[3],
		 (int) sipx->sipx_node[4], (int) sipx->sipx_node[5]);
    else
	buf[0] = '\0';
    return (buf);
}

static char *IPX_sprint(struct sockaddr *sap, int numeric)
{
    static char buf[64];

    if (sap->sa_family != AF_IPX)
	return strncpy(buf, ("[NONE SET]"), sizeof(buf));
    return (IPX_print((unsigned char *)sap->sa_data));
}

static char *ddp_print(unsigned char *ptr)
{
    static char buf[64];
    struct sockaddr_at *sat = (struct sockaddr_at *) (ptr - 2);
    snprintf(buf, sizeof(buf), "%d/%d", (int) ntohs(sat->sat_addr.s_net),
	     (int) sat->sat_addr.s_node);
    return (buf);
}

static char *ddp_sprint(struct sockaddr *sap, int numeric)
{
    static char buf[64];

    if (sap->sa_family != AF_APPLETALK)
	return strncpy(buf, ("[NONE SET]"), sizeof(buf));
    return (ddp_print((unsigned char *)sap->sa_data));
}

static char *ec_print(unsigned char *ptr)
{
    static char buf[64];
    struct ec_addr *ec = (struct ec_addr *) ptr;
    snprintf(buf, sizeof(buf), "%d.%d", ec->net, ec->station);
    return buf;
}


static char *ec_sprint(struct sockaddr *sap, int numeric)
{
    struct sockaddr_ec *sec = (struct sockaddr_ec *) sap;

    if (sap->sa_family != AF_ECONET)
	return (char *)"[NONE SET]";

    return ec_print((unsigned char *) &sec->addr);
}

unsigned int clip(int c) { return (c & 0xFF); }

ULONG warn_func(NWSCREEN *screen, ULONG index)
{
    ULONG mNum, retCode;

    mask_portal(mainportal);

    mNum = make_menu(screen,
		     " Exit IFCON ",
		     get_screen_lines() - 12,
		     ((get_screen_cols() - 1) / 2) -
                     ((strlen((const char *)"  Exit IFCON  ") + 2) / 2),
		     2,
		     BORDER_DOUBLE,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     0,
		     0,
		     0,
		     TRUE,
		     0);

    add_item_to_menu(mNum, "Yes", 1);
    add_item_to_menu(mNum, "No", 0);

    retCode = activate_menu(mNum);
    if (retCode == (ULONG) -1)
       retCode = 0;

    free_menu(mNum);

    unmask_portal(mainportal);

    return retCode;
}

char *comma_snprintf(char *buffer, int size, const char *format, ...)
{
    unsigned int len, i;
    char buf[1024], *src, *dest;
    size_t vsize = size > (1024 - 1)
	           ? 1024 - 1 : size;
    va_list ap;

    va_start(ap, format);
    len = vsnprintf((char *)buf, vsize, format, ap);
    va_end(ap);

    if (len)
    {
       src = buf + strlen((const char *)buf);
       dest = buffer + vsize;
       *dest = '\0';
       for (i=0; (i < strlen((const char *)buf)) &&
            (dest >= buffer) && (src >= buf); i++)
       {
          if (i && !(i % 3))
             *--dest = ',';
          *--dest = *--src;
       }
       return (char *)dest;
    }
    return (char *)"";
}


struct user_net_device_stats {
    unsigned long long rx_packets;	/* total packets received       */
    unsigned long long tx_packets;	/* total packets transmitted    */
    unsigned long long rx_bytes;	/* total bytes received         */
    unsigned long long tx_bytes;	/* total bytes transmitted      */
    unsigned long rx_errors;	/* bad packets received         */
    unsigned long tx_errors;	/* packet transmit problems     */
    unsigned long rx_dropped;	/* no space in linux buffers    */
    unsigned long tx_dropped;	/* no space available in linux  */
    unsigned long rx_multicast;	/* multicast packets received   */
    unsigned long rx_compressed;
    unsigned long tx_compressed;
    unsigned long collisions;

    /* detailed rx_errors: */
    unsigned long rx_length_errors;
    unsigned long rx_over_errors;	/* receiver ring buff overflow  */
    unsigned long rx_crc_errors;	/* recved pkt with crc error    */
    unsigned long rx_frame_errors;	/* recv'd frame alignment error */
    unsigned long rx_fifo_errors;	/* recv'r fifo overrun          */
    unsigned long rx_missed_errors;	/* receiver missed packet     */
    /* detailed tx_errors */
    unsigned long tx_aborted_errors;
    unsigned long tx_carrier_errors;
    unsigned long tx_fifo_errors;
    unsigned long tx_heartbeat_errors;
    unsigned long tx_window_errors;
};

int pnv = 1;

int get_dev_fields(char *bp, struct user_net_device_stats *stats)
{
    switch (pnv)
   {
    case 3:
	sscanf(bp,
	"%llu %llu %lu %lu %lu %lu %lu %lu %llu %llu %lu %lu %lu %lu %lu %lu",
	       &stats->rx_bytes,
	       &stats->rx_packets,
	       &stats->rx_errors,
	       &stats->rx_dropped,
	       &stats->rx_fifo_errors,
	       &stats->rx_frame_errors,
	       &stats->rx_compressed,
	       &stats->rx_multicast,

	       &stats->tx_bytes,
	       &stats->tx_packets,
	       &stats->tx_errors,
	       &stats->tx_dropped,
	       &stats->tx_fifo_errors,
	       &stats->collisions,
	       &stats->tx_carrier_errors,
	       &stats->tx_compressed);
	break;
    case 2:
	sscanf(bp, "%llu %llu %lu %lu %lu %lu %llu %llu %lu %lu %lu %lu %lu",
	       &stats->rx_bytes,
	       &stats->rx_packets,
	       &stats->rx_errors,
	       &stats->rx_dropped,
	       &stats->rx_fifo_errors,
	       &stats->rx_frame_errors,

	       &stats->tx_bytes,
	       &stats->tx_packets,
	       &stats->tx_errors,
	       &stats->tx_dropped,
	       &stats->tx_fifo_errors,
	       &stats->collisions,
	       &stats->tx_carrier_errors);
	stats->rx_multicast = 0;
	break;
    case 1:
	sscanf(bp, "%llu %lu %lu %lu %lu %llu %lu %lu %lu %lu %lu",
	       &stats->rx_packets,
	       &stats->rx_errors,
	       &stats->rx_dropped,
	       &stats->rx_fifo_errors,
	       &stats->rx_frame_errors,

	       &stats->tx_packets,
	       &stats->tx_errors,
	       &stats->tx_dropped,
	       &stats->tx_fifo_errors,
	       &stats->collisions,
	       &stats->tx_carrier_errors);
	stats->rx_bytes = 0;
	stats->tx_bytes = 0;
	stats->rx_multicast = 0;
	break;
    }
    return 0;
}

int if_getconfig(int skfd, int portal, char *ifname, int *pos, char *bp)
{
	struct ifreq ifr;
	int metric, mtu, flags, family, has_ip, row = pos ? *pos : 0;
	struct sockaddr dstaddr, broadaddr, netmask, ifaddr, hwa;
	unsigned char *hwaddr;
        FILE *f;
        char addr6[40], devname[20];
        struct sockaddr_in6 sap;
        int plen, scope, dad_status, if_idx, fd, stats_valid;
        char addr6p[8][5], *w;
	int has_econet = 0, has_ddp = 0, has_ipx_bb = 0, has_ipx_sn = 0;
	int has_ipx_e3 = 0, has_ipx_e2 = 0;
        struct sockaddr ipxaddr_bb;
        struct sockaddr ipxaddr_sn;
        struct sockaddr ipxaddr_e3;
        struct sockaddr ipxaddr_e2;
        struct sockaddr ddpaddr;
        struct sockaddr ecaddr;
        struct ifmap map;
        unsigned long can_compress = 0, tx_queue_len, keepalive = 0,
		      outfill = 0;
        unsigned long long rx, tx, short_rx, short_tx;
        char rx_increment[5]="b";
        char tx_increment[5]="b";
        char display_buffer[1024];
        char format_buffer[1024];
        char cat_buffer[1024];
        struct user_net_device_stats ifstats;

        memset(&ifstats, 0, sizeof(struct user_net_device_stats));
	get_dev_fields(bp, &ifstats);
        stats_valid = 1;

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFHWADDR, &ifr) >= 0)
        {
	   hwa = ifr.ifr_hwaddr;
	   hwaddr = (unsigned char *)hwa.sa_data;
           family = ifr.ifr_hwaddr.sa_family;
        }
        else
        {
	   memset(&hwa, 0, sizeof(struct sockaddr));
	   hwaddr = (unsigned char *)hwa.sa_data;
           family = 0;
        }

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFFLAGS, &ifr) >= 0)
           flags = ifr.ifr_flags;
        else
           flags = 0;

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFADDR, &ifr) >= 0)
        {
           has_ip = 1;
           ifaddr = ifr.ifr_addr;
        }
        else
        {
	   memset(&ifaddr, 0, sizeof(struct sockaddr));
           has_ip = 0;
        }

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFMETRIC, &ifr) < 0) {
	   metric = 0;
	} else
	   metric = ifr.ifr_metric;

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFMTU, &ifr) < 0)
	   mtu = 0;
	else
	   mtu = ifr.ifr_mtu;

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFDSTADDR, &ifr) < 0) {
	   memset(&dstaddr, 0, sizeof(struct sockaddr));
	} else
	   dstaddr = ifr.ifr_dstaddr;

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFBRDADDR, &ifr) < 0) {
	   memset(&broadaddr, 0, sizeof(struct sockaddr));
	} else
	   broadaddr = ifr.ifr_broadaddr;

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFNETMASK, &ifr) < 0) {
	   memset(&netmask, 0, sizeof(struct sockaddr));
	} else
	   netmask = ifr.ifr_netmask;

        strcpy(ifr.ifr_name, ifname);
        if (ioctl(skfd, SIOCGIFTXQLEN, &ifr) < 0)
	   tx_queue_len = -1;
	else
	   tx_queue_len = ifr.ifr_qlen;

        if (family == ARPHRD_SLIP  || family == ARPHRD_CSLIP  ||
	    family == ARPHRD_SLIP6 || family == ARPHRD_CSLIP6 ||
	    family == ARPHRD_ADAPT)
        {
	    strcpy(ifr.ifr_name, ifname);
	    if (ioctl(skfd, SIOCGOUTFILL, &ifr) < 0)
	       outfill = 0;
	    else
	       outfill = (unsigned long) ifr.ifr_data;

	    strcpy(ifr.ifr_name, ifname);
	    if (ioctl(skfd, SIOCGKEEPALIVE, &ifr) < 0)
	       keepalive = 0;
	    else
	       keepalive = (unsigned long) ifr.ifr_data;
        }

        strcpy(ifr.ifr_name, ifname);
        if (ioctl(skfd, SIOCGIFMAP, &ifr) < 0)
	   memset(&map, 0, sizeof(struct ifmap));
        else
	   memcpy(&map, &ifr.ifr_map, sizeof(struct ifmap));

        display_buffer[0] = '\0';
	snprintf(format_buffer, sizeof(format_buffer), "%s Link encap:%s  ",
                ifname, get_arp_type(family));
        strcat(display_buffer, format_buffer);

        if (hwaddr)
        {
	  snprintf(format_buffer, sizeof(format_buffer),
                  "HWaddr: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x",
	          hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3],
                  hwaddr[4], hwaddr[5]);
           strcat(display_buffer, format_buffer);
        }

	write_portal(portal, (const char *)display_buffer, row++, 2,
                        BRITEWHITE | BGBLUE);



        if (family == ARPHRD_CSLIP || family == ARPHRD_CSLIP6)
	   can_compress = 1;

        fd = socket(AF_APPLETALK, SOCK_DGRAM, 0);
        if (fd >= 0)
        {
	   strcpy(ifr.ifr_name, ifname);
	   if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
           {
	       ddpaddr = ifr.ifr_addr;
	       has_ddp = 1;
	   }
           close(fd);
        }

        fd = socket(AF_IPX, SOCK_DGRAM, 0);
        if (fd >= 0)
        {
	   strcpy(ifr.ifr_name, ifname);
	   if (!ipx_getaddr(fd, IPX_FRAME_ETHERII, &ifr))
           {
	      has_ipx_bb = 1;
	      ipxaddr_bb = ifr.ifr_addr;
	   }
	   strcpy(ifr.ifr_name, ifname);
	   if (!ipx_getaddr(fd, IPX_FRAME_SNAP, &ifr))
           {
	      has_ipx_sn = 1;
	      ipxaddr_sn = ifr.ifr_addr;
	   }
	   strcpy(ifr.ifr_name, ifname);
	   if (!ipx_getaddr(fd, IPX_FRAME_8023, &ifr))
           {
	      has_ipx_e3 = 1;
	      ipxaddr_e3 = ifr.ifr_addr;
	   }
	   strcpy(ifr.ifr_name, ifname);
	   if (!ipx_getaddr(fd, IPX_FRAME_8022, &ifr))
           {
	      has_ipx_e2 = 1;
	      ipxaddr_e2 = ifr.ifr_addr;
	   }
           close(fd);
        }

        fd = socket(AF_ECONET, SOCK_DGRAM, 0);
        if (fd >= 0)
        {
	   strcpy(ifr.ifr_name, ifname);
	   if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
           {
	      ecaddr = ifr.ifr_addr;
	      has_econet = 1;
	   }
           close(fd);
        }

	if (has_ipx_bb)
        {
	   snprintf(display_buffer, sizeof(display_buffer), ("IPX/Ethernet II addr:%s"),
		   IPX_sprint(&ipxaddr_bb, 1));
           write_portal(portal, (const char *)display_buffer, row++, 2,
                        BRITEWHITE | BGBLUE);
        }

	if (has_ipx_sn)
        {
	   snprintf(display_buffer, sizeof(display_buffer), ("IPX/Ethernet SNAP addr:%s"),
		   IPX_sprint(&ipxaddr_sn, 1));
           write_portal(portal, (const char *)display_buffer, row++, 2,
                        BRITEWHITE | BGBLUE);
        }

	if (has_ipx_e2)
        {
	   snprintf(display_buffer, sizeof(display_buffer), ("IPX/Ethernet 802.2 addr:%s"),
		   IPX_sprint(&ipxaddr_e2, 1));
           write_portal(portal, (const char *)display_buffer, row++, 2,
                        BRITEWHITE | BGBLUE);
        }

	if (has_ipx_e3)
        {
	   snprintf(display_buffer, sizeof(display_buffer), ("IPX/Ethernet 802.3 addr:%s"),
		   IPX_sprint(&ipxaddr_e3, 1));
           write_portal(portal, (const char *)display_buffer, row++, 2,
                        BRITEWHITE | BGBLUE);
        }

	if (has_ddp)
        {
	   snprintf(display_buffer, sizeof(display_buffer), ("EtherTalk Phase 2 addr:%s"),
                   ddp_sprint(&ddpaddr, 1));
           write_portal(portal, (const char *)display_buffer, row++, 2,
                        BRITEWHITE | BGBLUE);
        }

	if (has_econet)
        {
	   snprintf(display_buffer, sizeof(display_buffer), ("econet addr:%s"),
                   ec_sprint(&ecaddr, 1));
           write_portal(portal, (const char *)display_buffer, row++, 2,
                        BRITEWHITE | BGBLUE);
        }

        display_buffer[0] = '\0';
        if (flags == 0)
	  strcat(display_buffer, ("[NO FLAGS] "));
        if (flags & IFF_UP)
	  strcat(display_buffer, ("UP "));
        if (flags & IFF_BROADCAST)
	  strcat(display_buffer, ("BROADCAST "));
        if (flags & IFF_DEBUG)
	  strcat(display_buffer, ("DEBUG "));
        if (flags & IFF_LOOPBACK)
	  strcat(display_buffer, ("LOOPBACK "));
        if (flags & IFF_POINTOPOINT)
	  strcat(display_buffer, ("POINTOPOINT "));
        if (flags & IFF_NOTRAILERS)
	  strcat(display_buffer, ("NOTRAILERS "));
        if (flags & IFF_RUNNING)
	  strcat(display_buffer, ("RUNNING "));
        if (flags & IFF_NOARP)
	  strcat(display_buffer, ("NOARP "));
        if (flags & IFF_PROMISC)
	  strcat(display_buffer, ("PROMISC "));
        if (flags & IFF_ALLMULTI)
	  strcat(display_buffer, ("ALLMULTI "));
        if (flags & IFF_SLAVE)
	  strcat(display_buffer, ("SLAVE "));
        if (flags & IFF_MASTER)
	  strcat(display_buffer, ("MASTER "));
        if (flags & IFF_MULTICAST)
	  strcat(display_buffer, ("MULTICAST "));
        if (flags & IFF_DYNAMIC)
	  strcat(display_buffer, ("DYNAMIC "));

        snprintf(format_buffer, sizeof(format_buffer),
		 " MTU:%u  Metric:%u",
		 mtu, metric ? metric : 1);
	strcat(display_buffer, format_buffer);

        if (outfill || keepalive)
        {
           snprintf(format_buffer, sizeof(format_buffer),
		    "  Outfill:%ld  Keepalive:%ld",
		    outfill, keepalive);
	   strcat(display_buffer, format_buffer);
        }
        write_portal(portal, (const char *)display_buffer, row++, 2,
                    BRITEWHITE | BGBLUE);

        if (has_ip)
        {
           display_buffer[0] = '\0';
           snprintf(format_buffer, sizeof(format_buffer),
		    "inet addr:%u.%u.%u.%u  ",
                  clip(ifaddr.sa_data[2]), clip(ifaddr.sa_data[3]),
                  clip(ifaddr.sa_data[4]), clip(ifaddr.sa_data[5]));
	   strcat(display_buffer, format_buffer);

	   if (flags & IFF_POINTOPOINT)
           {
              snprintf(format_buffer, sizeof(format_buffer),
		       "P-t-P:%u.%u.%u.%u  ",
                  clip(dstaddr.sa_data[2]), clip(dstaddr.sa_data[3]),
                  clip(dstaddr.sa_data[4]), clip(dstaddr.sa_data[5]));
	      strcat(display_buffer, format_buffer);
	   }

	   if (flags & IFF_BROADCAST)
           {
              snprintf(format_buffer, sizeof(format_buffer),
		       "Bcast:%u.%u.%u.%u  ",
                  clip(broadaddr.sa_data[2]), clip(broadaddr.sa_data[3]),
                  clip(broadaddr.sa_data[4]), clip(broadaddr.sa_data[5]));
	      strcat(display_buffer, format_buffer);
	   }
           snprintf(format_buffer, sizeof(format_buffer),
		    "Mask:%u.%u.%u.%u",
                  clip(netmask.sa_data[2]), clip(netmask.sa_data[3]),
                  clip(netmask.sa_data[4]), clip(netmask.sa_data[5]));
	   strcat(display_buffer, format_buffer);

           write_portal(portal, (const char *)display_buffer, row++, 2,
                        BRITEWHITE | BGBLUE);
        }

        if ((f = fopen("/proc/net/if_inet6", "r")) != NULL)
        {
	   while (fscanf(f, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %02x"
                             " %02x %02x %20s\n",
		      addr6p[0], addr6p[1], addr6p[2], addr6p[3],
		      addr6p[4], addr6p[5], addr6p[6], addr6p[7],
		  &if_idx, &plen, &scope, &dad_status, devname) != EOF)
           {
	      if (!strcmp(devname, ifname))
              {
                 display_buffer[0] = '\0';
	         snprintf(addr6, sizeof(addr6), "%s:%s:%s:%s:%s:%s:%s:%s",
			 addr6p[0], addr6p[1], addr6p[2], addr6p[3],
			 addr6p[4], addr6p[5], addr6p[6], addr6p[7]);

		 INET6_input(1, addr6, (struct sockaddr *) &sap);
		 snprintf(format_buffer, sizeof(format_buffer),
			  ("inet6 addr: %s/%d"),
		       INET6_sprint((struct sockaddr *) &sap, 1), plen);
	         strcat(display_buffer, format_buffer);

		 snprintf(format_buffer, sizeof(format_buffer), (" Scope:"));
	         strcat(display_buffer, format_buffer);

		 switch (scope)
                 {
		     case 0:
		        strcat(display_buffer, ("Global"));
		        break;

		     case IPV6_ADDR_LINKLOCAL:
		        strcat(display_buffer, ("Link"));
		        break;

		     case IPV6_ADDR_SITELOCAL:
		        strcat(display_buffer, ("Site"));
		        break;

		     case IPV6_ADDR_COMPATv4:
		        strcat(display_buffer, ("Compat"));
		        break;

		     case IPV6_ADDR_LOOPBACK:
		        strcat(display_buffer, ("Host"));
		        break;

		     default:
		        strcat(display_buffer, ("Unknown"));
		  }
                  write_portal(portal, (const char *)display_buffer, row++, 2,
                               BRITEWHITE | BGBLUE);
	      }
	   }
	   fclose(f);
        }

        if (stats_valid)
        {
	   w = comma_snprintf(format_buffer, sizeof(format_buffer), "%llu",
			      ifstats.rx_packets);
           snprintf(display_buffer, sizeof(display_buffer), "RX packets  :%s",
		    w);
           write_portal_cleol(portal, (const char *)display_buffer, row++, 2,
                     BRITEWHITE | BGBLUE);

	   w = comma_snprintf(format_buffer, sizeof(format_buffer), "%lu",
               ifstats.rx_errors);
           snprintf(display_buffer, sizeof(display_buffer), "RX errors   :%s",
		    w);
           write_portal_cleol(portal, (const char *)display_buffer, row++, 2,
                     BRITEWHITE | BGBLUE);

	   w = comma_snprintf(format_buffer, sizeof(format_buffer), "%lu",
               ifstats.rx_dropped);
           snprintf(display_buffer, sizeof(display_buffer), "RX dropped  :%s",
		    w);
           write_portal_cleol(portal, (const char *)display_buffer, row++, 2,
                     BRITEWHITE | BGBLUE);

	   w = comma_snprintf(format_buffer, sizeof(format_buffer), "%lu",
               ifstats.rx_fifo_errors);
           snprintf(display_buffer, sizeof(display_buffer), "RX overruns :%s",
		    w);
           write_portal_cleol(portal, (const char *)display_buffer, row++, 2,
                     BRITEWHITE | BGBLUE);

	   w = comma_snprintf(format_buffer, sizeof(format_buffer), "%lu",
	       ifstats.rx_frame_errors);
           snprintf(display_buffer, sizeof(display_buffer), "RX frame    :%s",
		    w);
           write_portal_cleol(portal, (const char *)display_buffer, row++, 2,
                     BRITEWHITE | BGBLUE);

	   rx = ifstats.rx_bytes;
	   tx = ifstats.tx_bytes;
	   short_rx = rx * 10;
	   short_tx = tx * 10;

	   if (rx > 1000000000000000000)
           {
              short_rx /= 1000000000000000000;
              strcpy(rx_increment, "Eb");
           }
	   else
	   if (rx > 1000000000000000)
           {
              short_rx /= 1000000000000000;
              strcpy(rx_increment, "Pb");
           }
	   else
	   if (rx > 1000000000000)
           {
              short_rx /= 1000000000000;
              strcpy(rx_increment, "Tb");
           }
	   else
	   if (rx > 1000000000)
           {
              short_rx /= 1000000000;
              strcpy(rx_increment, "Gb");
           }
	   else
	   if (rx > 1000000)
           {
              short_rx /= 1000000;
              strcpy(rx_increment, "Mb");
           }
	   else
           if (rx > 1000)
           {
              short_rx /= 1000;
              strcpy(rx_increment, "Kb");
           }

	   if (tx > 1000000000000000000)
           {
              short_tx /= 1000000000000000000;
              strcpy(tx_increment, "Eb");
           }
	   else
	   if (tx > 1000000000000000)
           {
              short_tx /= 1000000000000000;
              strcpy(tx_increment, "Pb");
           }
	   else
	   if (tx > 1000000000000)
           {
              short_tx /= 1000000000000;
              strcpy(tx_increment, "Tb");
           }
	   else
	   if (tx > 1000000000)
           {
              short_tx /= 1000000000;
              strcpy(tx_increment, "Gb");
           }
	   else
	   if (tx > 1000000)
           {
              short_tx /= 1000000;
              strcpy(tx_increment, "Mb");
           }
	   else
           if (tx > 1000)
           {
              short_tx /= 1000;
              strcpy(tx_increment, "Kb");
           }

	   w = comma_snprintf(format_buffer, sizeof(format_buffer), "%llu",
		    ifstats.tx_packets);
           snprintf(display_buffer, sizeof(display_buffer), "TX packets  :%s",
		    w);
           write_portal_cleol(portal, (const char *)display_buffer, row++, 2,
                     BRITEWHITE | BGBLUE);

	   w = comma_snprintf(format_buffer, sizeof(format_buffer), "%lu",
		    ifstats.tx_errors);
           snprintf(display_buffer, sizeof(display_buffer), "TX errors   :%s",
		    w);
           write_portal_cleol(portal, (const char *)display_buffer, row++, 2,
                     BRITEWHITE | BGBLUE);

	   w = comma_snprintf(format_buffer, sizeof(format_buffer), "%lu",
		    ifstats.tx_dropped);
           snprintf(display_buffer, sizeof(display_buffer), "TX dropped  :%s",
		    w);
           write_portal_cleol(portal, (const char *)display_buffer, row++, 2,
                     BRITEWHITE | BGBLUE);

	   w = comma_snprintf(format_buffer, sizeof(format_buffer), "%lu",
		    ifstats.tx_fifo_errors);
           snprintf(display_buffer, sizeof(display_buffer), "TX overruns :%s",
		    w);
           write_portal_cleol(portal, (const char *)display_buffer, row++, 2,
                     BRITEWHITE | BGBLUE);

	   w = comma_snprintf(format_buffer, sizeof(format_buffer), "%lu",
		    ifstats.tx_carrier_errors);
           snprintf(display_buffer, sizeof(display_buffer), "TX carrier  :%s",
		    w);
           write_portal_cleol(portal, (const char *)display_buffer, row++, 2,
                     BRITEWHITE | BGBLUE);

	   w = comma_snprintf(format_buffer, sizeof(format_buffer), "%lu",
		    ifstats.collisions);
           snprintf(display_buffer, sizeof(display_buffer), "collisions  :%s",
		    w);
           write_portal_cleol(portal, (const char *)display_buffer, row++, 2,
                        BRITEWHITE | BGBLUE);

	   if (can_compress)
           {
	      w = comma_snprintf(format_buffer, sizeof(format_buffer), "%lu",
		       ifstats.tx_compressed);
           snprintf(display_buffer, sizeof(display_buffer), "compressed  :%s",
		    w);
           write_portal_cleol(portal, (const char *)display_buffer, row++, 2,
                           BRITEWHITE | BGBLUE);
           }

	   if (tx_queue_len != (unsigned long)-1)
           {
	      w = comma_snprintf(format_buffer, sizeof(format_buffer), "%ld",
		       tx_queue_len);
              snprintf(display_buffer, sizeof(display_buffer),
		       "txqueuelen  :%s", w);
              write_portal_cleol(portal, (const char *)display_buffer, row++, 2,
                           BRITEWHITE | BGBLUE);
           }


	   w = comma_snprintf(format_buffer, sizeof(format_buffer), "%llu", rx);
           snprintf(display_buffer, sizeof(display_buffer),
		    "RX bytes    :%s", w);
	   w = comma_snprintf(format_buffer, sizeof(format_buffer), "%llu",
			      (unsigned long long)(short_rx / 10));
           snprintf(cat_buffer, sizeof(cat_buffer), " (%s.%llu %s) ",
		    w, (unsigned long long)(short_rx % 10), rx_increment);
	   strcat(display_buffer, cat_buffer);

	   write_portal_cleol(portal, (const char *)display_buffer, row++, 2,
                     BRITEWHITE | BGBLUE);

	   w = comma_snprintf(format_buffer, sizeof(format_buffer), "%llu", tx);
           snprintf(display_buffer, sizeof(display_buffer),
		    "TX bytes    :%s", w);
           w = comma_snprintf(format_buffer, sizeof(format_buffer), "%llu",
		    (unsigned long long)(short_tx / 10));
           snprintf(cat_buffer, sizeof(cat_buffer), " (%s.%llu %s) ",
		     w, (unsigned long long)(short_tx % 10), tx_increment);
	   strcat(display_buffer, cat_buffer);

	   write_portal_cleol(portal, (const char *)display_buffer, row++, 2,
                     BRITEWHITE | BGBLUE);
        }

        if ((map.irq || map.mem_start || map.dma || map.base_addr))
        {
           display_buffer[0] = '\0';
	   if (map.irq)
           {
	      snprintf(format_buffer, sizeof(format_buffer),
		       ("Interrupt:%d "), map.irq);
	      strcat(display_buffer, format_buffer);
           }

	   if (map.base_addr >= 0x100)
           {
	      snprintf(format_buffer, sizeof(format_buffer),
		       ("Base address:0x%x "),
		       map.base_addr);
	      strcat(display_buffer, format_buffer);
           }

	   if (map.mem_start)
           {
	      snprintf(format_buffer, sizeof(format_buffer),
		       ("Memory:%lx-%lx "), map.mem_start,
		       map.mem_end);
	      strcat(display_buffer, format_buffer);
           }

	   if (map.dma)
           {
	      snprintf(format_buffer, sizeof(format_buffer),
		       ("DMA chan:%x "), map.dma);
	      strcat(display_buffer, format_buffer);
           }
           write_portal(portal, (const char *)display_buffer, row++, 2,
                        BRITEWHITE | BGBLUE);
        }
        write_portal(portal, (const char *)" ", row++, 2,
                     BRITEWHITE | BGBLUE);
        if (pos)
           *pos = row;

	return 0;
}

char *get_name(char *name, char *p)
{
    while (isspace(*p))
	p++;
    while (*p) {
	if (isspace(*p))
	    break;
	if (*p == ':')
        {
	    // could be an alias
	    char *dot = p, *dotname = name;

	    *name++ = *p++;

	    while (isdigit(*p))
		*name++ = *p++;

	    if (*p != ':')
            {
	        // it wasn't, backup
		p = dot;
		name = dotname;
	    }

	    if (*p == '\0')
		return NULL;

	    p++;
	    break;
	}
	*name++ = *p++;
    }
    *name++ = '\0';
    return p;
}

int display_network_summary(int portal, char *ifname)
{
    FILE *fp;
    char buf[1024];
    int err, pos = 0, skfd;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
       return skfd;

    fp = fopen("/proc/net/dev", "r");
    if (!fp)
    {
       close(skfd);
       return -1;
    }

    fgets(buf, sizeof buf, fp);	 // eat line
    fgets(buf, sizeof buf, fp);

    if (strstr(buf, "compressed"))
       pnv = 3;
    else
    if (strstr(buf, "bytes"))
       pnv = 2;
    else
       pnv = 1;


    err = 0;
    while (fgets(buf, sizeof buf, fp))
    {
	char name[IFNAMSIZ], *s;

	s = get_name(name, buf);
        if (!ifname)
        {
           if_getconfig(skfd, portal, name, &pos, s);
           write_portal_line(portal, pos++, BRITEWHITE | BGBLUE);
        }
        else
        if (!strcasecmp(ifname, name))
        {
           if_getconfig(skfd, portal, name, &pos, s);
           write_portal_line(portal, pos++, BRITEWHITE | BGBLUE);
        }
    }
    fclose(fp);
    close(skfd);
    return err;
}

typedef struct nparam {
   int portal;
   char ifname[256];
} NP;

void *network_routine(void *p)
{
   NP *np = (NP *)p;
   int portal = np ? np->portal : 0;
   char *ifname = np ? np->ifname : NULL;
   int state;

   while (networkactive && p)
   {
       pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state);
       display_network_summary(portal, ifname);
       update_static_portal(portal);
       pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &state);
       sleep(1);
       if (!get_sleep_count(portal))
          clear_portal_focus(portal);
   }
   return NULL;
}

void *network_summary_routine(void *p)
{
   int portal = p ? *(int *)p : 0;
   int state;

   while (netsumactive && p)
   {
       pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state);
       display_network_summary(portal, NULL);
       update_static_portal(portal);
       pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &state);
       sleep(1);
       if (!get_sleep_count(portal))
          clear_portal_focus(portal);
   }
   return NULL;
}

void *pstat_routine(void *p)
{
   int portal = p ? *(int *)p : 0;
   int state;

   while (active && p)
   {
       pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &state);
       display_network_summary(portal, NULL);
       update_static_portal(portal);
       pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &state);
       sleep(1);
       if (!get_sleep_count(portal))
          clear_portal_focus(portal);
   }
   return NULL;
}

ULONG netmenuKeyboardHandler(NWSCREEN *screen, ULONG key, ULONG index)
{
    BYTE display_buffer[1024];

    switch (key)
    {
       case F1:
          snprintf((char *)display_buffer, sizeof(display_buffer),
                  "Go Read the IFCONFIG Manual Page");
          error_portal((const char *)display_buffer,
		      ((get_screen_lines() - 2) / 2));
          break;

       default:
	  break;
    }
    return 0;
}

ULONG netmenuFunction(NWSCREEN *screen, ULONG value, BYTE *option,
                      ULONG menu_index)
{
    int portal = 0;
    char display_buffer[1024];
    NP np;

    switch (value)
    {
       default:
          snprintf(display_buffer, sizeof(display_buffer),
		   "Interface [%s] Statistics", option);
          portal = make_portal(get_console_screen(),
		       (char *)display_buffer,
		       0,
		       3,
		       0,
		       get_screen_lines() - 2,
		       get_screen_cols() - 1,
		       1024,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       NULL,
		       0,
		       NULL,
		       TRUE);
          if (!portal)
             return 0;

          snprintf((char *)display_buffer, sizeof(display_buffer),
                  "  F1-Help  F3-Return to Menu  [terminal:%s]",
                  get_term_name());
          write_screen_comment_line(get_console_screen(),
				    (const char *)display_buffer,
				    BLUE | BGWHITE);

          activate_static_portal(portal);
          update_static_portal(portal);

          networkactive = TRUE;
          np.portal = portal;
          strcpy((char *)np.ifname, (const char *)option);
          pthread_create(&networkstat, NULL, network_routine, &np);

          enable_portal_focus(portal, 5);
          get_portal_resp(portal);

          networkactive = 0;
          pthread_cancel(networkstat);
          pthread_join(networkstat, NULL);

          snprintf((char *)display_buffer, sizeof(display_buffer),
		   "  F1-Help  F3-Exit  TAB-View Stats  "
		   "[terminal:%s]", get_term_name());
          write_screen_comment_line(get_console_screen(),
				    (const char *)display_buffer,
				    BLUE | BGWHITE);
          if (portal)
          {
             deactivate_static_portal(portal);
             free_portal(portal);
          }
          break;
    }
    return 0;
}

int build_network_menu(void)
{
    FILE *fp;
    char buf[1024];
    int err, netmenu, index = 1, skfd;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
       return skfd;

    fp = fopen("/proc/net/dev", "r");
    if (!fp)
    {
       close(skfd);
       return -1;
    }

    netmenu = make_menu(get_console_screen(),
		     "  Network Devices  ",
		     ((get_screen_lines() - 1) / 2),
		     ((get_screen_cols() - 1) / 2) -
                     ((strlen("  Network Devices  ") + 4) / 2),
		     7,
		     BORDER_DOUBLE,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
                     netmenuFunction,
		     NULL,
		     netmenuKeyboardHandler,
		     TRUE,
		     0);

    if (!netmenu)
    {
       fclose(fp);
       close(skfd);
       return -1;
    }

    fgets(buf, sizeof buf, fp);	 // eat line
    fgets(buf, sizeof buf, fp);

    err = 0;
    while (fgets(buf, sizeof buf, fp))
    {
	char name[IFNAMSIZ];
	get_name(name, buf);
        add_item_to_menu(netmenu, name, index++);
    }
    fclose(fp);
    close(skfd);

    mask_portal(mainportal);

    err = activate_menu(netmenu);

    unmask_portal(mainportal);

    if (netmenu)
       free_menu(netmenu);

    return err;
}

#define CONFIG_NAME        "  Network Stats for Linux"
#define COPYRIGHT_NOTICE1  "  Copyright (c) 1997-2019 Leaf Linux. All Rights Reserved."
#define COPYRIGHT_NOTICE2  "  "

ULONG menuFunction(NWSCREEN *screen, ULONG value, BYTE *option,
                   ULONG menu_index)
{
    int portal = 0;
    unsigned char display_buffer[1024];

    switch (value)
    {
       case 1:
          portal = make_portal(get_console_screen(),
		       "Network Summary",
		       0,
		       3,
		       0,
		       get_screen_lines() - 2,
		       get_screen_cols() - 1,
		       1024,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       NULL,
		       0,
		       NULL,
		       TRUE);
          if (!portal)
             return 0;

          mask_portal(mainportal);

          snprintf((char *)display_buffer, sizeof(display_buffer),
                  "  F1-Help  F3-Return to Menu  [terminal:%s]",
                  get_term_name());
          write_screen_comment_line(get_console_screen(),
				    (const char *)display_buffer,
				    BLUE | BGWHITE);

          activate_static_portal(portal);
          update_static_portal(portal);

          netsumactive = TRUE;
          pthread_create(&netsumstat, NULL, network_summary_routine, &portal);

          enable_portal_focus(portal, 5);
          get_portal_resp(portal);

          netsumactive = 0;
          pthread_cancel(netsumstat);
          pthread_join(netsumstat, NULL);

          snprintf((char *)display_buffer, sizeof(display_buffer),
		   "  F1-Help  F3-Exit  TAB-View Stats  "
		   "[terminal:%s]", get_term_name());
          write_screen_comment_line(get_console_screen(),
				    (const char *)display_buffer,
				    BLUE | BGWHITE);
          if (portal)
          {
             deactivate_static_portal(portal);
             free_portal(portal);
          }
          unmask_portal(mainportal);
          break;

       case 2:
          build_network_menu();
          break;

       case 3:
          if (logportal == -1)
          {
             error_portal((const char *)" Message Portal Not Active ",
		         ((get_screen_lines() - 1) / 2));
             break;
          }

          mask_portal(mainportal);

          snprintf((char *)display_buffer, sizeof(display_buffer),
                  "  F1-Help  F3-Return to Menu  [terminal:%s]",
                  get_term_name());
          write_screen_comment_line(get_console_screen(),
				    (const char *)display_buffer,
				    BLUE | BGWHITE);

          activate_static_portal(logportal);
          update_static_portal(logportal);

          enable_portal_focus(logportal, 5);
          get_portal_resp(logportal);

          snprintf((char *)display_buffer, sizeof(display_buffer),
		   "  F1-Help  F3-Exit  TAB-View Stats  "
		   "[terminal:%s]", get_term_name());
          write_screen_comment_line(get_console_screen(),
                                   (const char *)display_buffer,
				   BLUE | BGWHITE);
          if (logportal)
             deactivate_static_portal(logportal);
          unmask_portal(mainportal);
          break;
    }
    return 0;

}

/*
    trap stderr messages and dump them into a portal with a circular buffer
*/

void *plog_routine(void *p)
{
   int stderr_backup, pipefd[2];
   unsigned char display_buffer[4096];
   unsigned char buf[4096];
   unsigned long i = 0;

   if (pipe(pipefd) == -1)
   {
      snprintf((char *)display_buffer, sizeof(display_buffer),
	       "ifcon:  could not open pipe");
      error_portal((const char *)display_buffer,
		     ((get_screen_lines() - 1) / 2));
      return NULL;
   }

   if ((stderr_backup = dup(STDERR_FILENO)) == -1)
   {
      snprintf((char *)display_buffer, sizeof(display_buffer),
	       "ifcon:  could not dup stderr");
      error_portal((const char *)display_buffer,
		     ((get_screen_lines() - 1) / 2));
      return NULL;
   }

   if (dup2(pipefd[1], STDERR_FILENO) == -1)
   {
      snprintf((char *)display_buffer, sizeof(display_buffer),
	       "ifcon:  could not dup2 stderr pipe");
      error_portal((const char *)display_buffer,
		     ((get_screen_lines() - 1) / 2));
      return NULL;
   }

   logportal = make_portal(get_console_screen(),
                       "Message Log",
		       0,
		       3,
		       0,
		       get_screen_lines() - 2,
		       get_screen_cols() - 1,
		       1024,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       NULL,
		       0,
		       NULL,
		       TRUE);
   if (!logportal)
      return NULL;

   while (active)
   {
      if (read(pipefd[0], buf, sizeof(buf)) == -1)
         break;

      buf[sizeof(buf)-1]='\0';
      snprintf((char *)display_buffer, sizeof(display_buffer), "%s", buf);
      write_portal_cleol(logportal, (const char *)display_buffer,
                         (i++ % 1024), 1, BRITEWHITE | BGBLUE);
   }

   if (logportal)
   {
      deactivate_static_portal(logportal);
      free_portal(logportal);
   }

   // close pipe and restore stderr
   close(pipefd[0]);
   close(pipefd[1]);

   if (dup2(stderr_backup, STDERR_FILENO) == -1)
   {
      snprintf((char *)display_buffer, sizeof(display_buffer),
	       "ifcon:  could not restore stderr");
      error_portal((const char *)display_buffer,
		     ((get_screen_lines() - 1) / 2));
      return NULL;
   }
   return NULL;
}

ULONG menuKeyboardHandler(NWSCREEN *screen, ULONG key, ULONG index)
{
    BYTE display_buffer[1024];

    switch (key)
    {
       case F1:
          mask_portal(mainportal);
          snprintf((char *)display_buffer, sizeof(display_buffer),
		   "Help section.  Please refer to the source code for"
		   " more information.");
          error_portal((const char *)display_buffer,
		      ((get_screen_lines() - 2) / 2));
          unmask_portal(mainportal);
          break;

       case TAB:
	  if (mainportal)
	  {
	     snprintf((char *)display_buffer, sizeof(display_buffer),
		      "  F1-Help  F3-Return to Menu  "
		      "[terminal:%s]", get_term_name());
	     write_screen_comment_line(get_console_screen(),
				       (const char *)display_buffer,
				       BLUE | BGWHITE);

             enable_portal_focus(mainportal, -1);
	     get_portal_resp(mainportal);

             snprintf((char *)display_buffer, sizeof(display_buffer),
                      "  F1-Help  F3-Exit  TAB-Switch to Stats "
		      " [terminal:%s]", get_term_name());
             write_screen_comment_line(get_console_screen(),
				       (const char *)display_buffer,
				       BLUE | BGWHITE);
	  }
	  break;

       default:
	  break;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int i;
    ULONG retCode = 0, ssi;
    BYTE display_buffer[1024];
    int plines, mlines, mlen = 0;
    struct utsname utsbuf;

    for (i=0; i < argc; i++)
    {
       if (!strcasecmp(argv[i], "-h"))
       {
          printf("USAGE:  ifcon (text|mono|unicode)\n");
          printf("        text           - disable box line drawing\n");
          printf("        mono           - disable color mode\n");
          printf("        unicode        - enable unicode support\n");
          printf("        ifcon -h       - this help screen\n");
          printf("        ifcon -help    - this help screen\n");
          exit(0);
       }

       if (!strcasecmp(argv[i], "-help"))
       {
          printf("USAGE:  ifcon (text|mono|unicode)\n");
          printf("        text           - disable box line drawing\n");
          printf("        mono           - disable color mode\n");
          printf("        unicode        - enable unicode support\n");
          printf("        ifcon -h       - this help screen\n");
          printf("        ifcon -help    - this help screen\n");
          exit(0);
       }

       if (!strcasecmp(argv[i], "text"))
          set_text_mode(1);

       if (!strcasecmp(argv[i], "mono"))
          set_mono_mode(1);

       if (!strcasecmp(argv[i], "unicode"))
          set_unicode_mode(1);
    }

    if (init_cworthy())
       return 0;

    // set ssi in seconds
    ssi = set_screensaver_interval(3 * 60);

    for (i=0; i < (get_screen_lines() - 1); i++)
    {
       put_char_cleol(get_console_screen(), 176 | A_ALTCHARSET, i,
		      CYAN | BGBLUE);
    }

    unsigned long header_attr = BLUE | BGCYAN;
    if (is_xterm())
       header_attr = BRITEWHITE | BGCYAN;
    if (mono_mode || !has_color)
       header_attr = BLUE | BGWHITE;

    snprintf((char *)display_buffer, sizeof(display_buffer), CONFIG_NAME);
    put_string_cleol(get_console_screen(), (const char *)display_buffer, NULL, 0, header_attr);

    snprintf((char *)display_buffer, sizeof(display_buffer), COPYRIGHT_NOTICE1);
    put_string_cleol(get_console_screen(), (const char *)display_buffer, NULL, 1, header_attr);

    if (!uname(&utsbuf)) {
       snprintf((char *)display_buffer, sizeof(display_buffer),
		"  %s %s %s (%s) [%s]", utsbuf.sysname, utsbuf.release,
		utsbuf.version, utsbuf.machine,
		utsbuf.nodename);
    } else {
       snprintf((char *)display_buffer, sizeof(display_buffer),
		COPYRIGHT_NOTICE2);
    }
    put_string_cleol(get_console_screen(), (const char *)display_buffer, NULL, 2, header_attr);

    snprintf((char *)display_buffer, sizeof(display_buffer),
             "  F1-Help  F3-Exit  TAB-View Stats "
	     " [terminal:%s]", get_term_name());
    write_screen_comment_line(get_console_screen(),
			      (const char *)display_buffer,
			      BLUE | BGWHITE);

   // adjust portal and menu sizes based on screen size
    plines = get_screen_lines() >= 34
	     ? get_screen_lines() - 13
	     : get_screen_lines() - 9;
    mlines = get_screen_lines() >= 34
	     ? get_screen_lines() - 12
	     : get_screen_lines() - 8;
    mlen = 3;

    mainportal = make_portal(get_console_screen(),
		       "Network Summary",
		       0,
		       3,
		       0,
		       plines,
		       get_screen_cols() - 1,
		       1024,
		       BORDER_SINGLE,
		       YELLOW | BGBLUE,
		       YELLOW | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       BRITEWHITE | BGBLUE,
		       NULL,
		       0,
		       NULL,
		       TRUE);
    if (!mainportal)
       goto ErrorExit;

    activate_static_portal(mainportal);
    update_static_portal(mainportal);

    menu = make_menu(get_console_screen(),
		     "  Available Options  ",
		     mlines,
		     ((get_screen_cols() - 1) / 2) -
                     ((strlen("  Available Options  ") + 4) / 2),
		     mlen,
		     BORDER_DOUBLE,
		     YELLOW | BGBLUE,
		     YELLOW | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     BRITEWHITE | BGBLUE,
		     menuFunction,
		     warn_func,
		     menuKeyboardHandler,
		     TRUE,
		     0);

    if (!menu)
	  goto ErrorExit;

    add_item_to_menu(menu, "Network Summary", 1);
    add_item_to_menu(menu, "Adapter Statistics", 2);
    add_item_to_menu(menu, "Message Log", 3);

    active = TRUE;
    pthread_create(&pstat, NULL, pstat_routine, &mainportal);
    pthread_create(&plog, NULL, plog_routine, NULL);

    retCode = activate_menu(menu);

    active = 0;

    pthread_cancel(pstat);
    pthread_cancel(plog);
    pthread_join(pstat, NULL);
    pthread_join(plog, NULL);

ErrorExit:;
    snprintf((char *)display_buffer, sizeof(display_buffer), " Exiting ... ");
    write_screen_comment_line(get_console_screen(),
			      (const char *)display_buffer, BLUE | BGWHITE);


    if (mainportal)
    {
       deactivate_static_portal(mainportal);
       free_portal(mainportal);
    }

    if (menu)
       free_menu(menu);

    set_screensaver_interval(ssi);
    release_cworthy();
    return retCode;
}

