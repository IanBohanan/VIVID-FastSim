#include <pcap/pcap.h>
#include <stdio.h>
#include <iostream>
#include <sys/mman.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h> 
#include <stdlib.h>
#include <cstdlib>
#include <list>

// session handle
pcap_t *handle;
pcap_dumper_t *pcap_dumper;

/* ethernet headers are always exactly 14 bytes [1] */
#define SIZE_ETHERNET 14
/* Ethernet addresses are 6 bytes */
#define ETHER_ADDR_LEN	6
struct ethernet {
    u_char  ether_dhost[ETHER_ADDR_LEN];    /* destination host address */
    u_char  ether_shost[ETHER_ADDR_LEN];    /* source host address */
    u_short ether_type;                     /* IP? ARP? RARP? etc */
};

/* IP header */
struct ip {
    u_char  ip_vhl;                 /* version << 4 | header length >> 2 */
    u_char  ip_tos;                 /* type of service */
    u_short ip_len;                 /* total length */
    u_short ip_id;                  /* identification */
    u_short ip_off;                 /* fragment offset field */
    #define IP_RF 0x8000            /* reserved fragment flag */
    #define IP_DF 0x4000            /* don't fragment flag */
    #define IP_MF 0x2000            /* more fragments flag */
    #define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
    u_char  ip_ttl;                 /* time to live */
    u_char  ip_p;                   /* protocol */
    u_short ip_sum;                 /* checksum */
    struct  in_addr ip_src,ip_dst;  /* source and dest address */
};
#define IP_HL(ip)               (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)                (((ip)->ip_vhl) >> 4)

/* TCP header */
typedef u_int tcp_seq;
struct tcp {
    u_short th_sport;               /* source port */
    u_short th_dport;               /* destination port */
    tcp_seq th_seq;                 /* sequence number */
    tcp_seq th_ack;                 /* acknowledgement number */
    u_char  th_offx2;               /* data offset, rsvd */
};
#define TH_OFF(th)      (((th)->th_offx2 & 0xf0) >> 4)

void process_packet(u_char *  , const struct pcap_pkthdr *header, const u_char *packet)
{
    pcap_dump((u_char*)pcap_dumper, header, packet);
    pcap_dump_flush(pcap_dumper);
}

int main(int argc, char *argv[])
{
	const char *device = argv[1];	
    char error_buffer[PCAP_ERRBUF_SIZE];

    // Get IP and netmask
    bpf_u_int32 netmask;
    bpf_u_int32 ip;
    if (pcap_lookupnet(device, &ip, &netmask, error_buffer) == -1) {
        std::cout << (stderr, "Couldn't get netmask for device %s: %s\n", device, error_buffer) << std::endl;
        ip = 0;
        netmask = 0;
    }

    // Open session in promiscuous mode
    handle = pcap_open_live(device, BUFSIZ, 1, 100, error_buffer);
    pcap_set_immediate_mode(handle, 1);
    if (handle == NULL) {
        std::cout << (stderr, "Couldn't open device %s: %s\n", device, error_buffer) << std::endl;
        return(2);
    }

    // Setup writing packets to pcap file
    pcap_dumper = pcap_dump_open(handle, "output.pcap");
    
    // Loop over packets
    std::cout << "Starting sniffer loop" << std::endl;
    pcap_loop(handle, -1, process_packet, NULL);

    pcap_close(handle);
    pcap_dump_close(pcap_dumper);

    return(0);
}
