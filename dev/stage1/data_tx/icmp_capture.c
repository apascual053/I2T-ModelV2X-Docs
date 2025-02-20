#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>

#define BUFFER_SIZE 2048

void print_hex(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

int main() {
    int sock;
    struct sockaddr_ll addr;
    uint8_t buffer[BUFFER_SIZE];

    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind socket to a specific interface (e.g., enp0s3)
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "enp0s3", IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) == -1) {
        perror("ioctl");
        close(sock);
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Listening for outgoing ICMP packets on enp0s3...\n");

    while (1) {
        ssize_t len = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
        if (len < 0) {
            perror("recvfrom");
            break;
        }

        if (len < sizeof(struct ethhdr)) {
            continue; // Ignore small packets
        }

        struct ethhdr *eth = (struct ethhdr *)buffer;

        if (ntohs(eth->h_proto) != ETH_P_IP) {
            continue; // Not an IPv4 packet
        }

        struct iphdr *ip = (struct iphdr *)(buffer + sizeof(struct ethhdr));
        if (ip->protocol != IPPROTO_ICMP) {
            continue; // Not an ICMP packet
        }

        printf("Captured outgoing ICMP packet (Ethernet frame):\n");
        print_hex(buffer, len);
    }

    close(sock);
    return 0;
}
