#define _GNU_SOURCE
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "us_caster_service.h"
#include "error_code_user.h"

/*
 *******************************************************************************
 * Macros
 *******************************************************************************
 */

#define MAX_BUFFER_SIZE 1024

/*
 *******************************************************************************
 * Structure definition
 *******************************************************************************
 */

/*
 *******************************************************************************
 * Data type definition
 *******************************************************************************
 */
/* Application status type */
typedef enum app_state_type {
    APP_STATE_DESTROY = 0,
    APP_STATE_INITIAL = 1,
    APP_STATE_READY = 2,
} app_state_type_t;

typedef enum app_thread_type {
    APP_THREAD_TX = 0,
    APP_THREAD_RX = 1
} app_thread_type_t;

/*
 *******************************************************************************
 * Global variables
 *******************************************************************************
 */
static uint8_t app_state = APP_STATE_INITIAL;
static caster_handler_t caster_handler = INVALID_CASTER_HANDLER;
static char *caster_rx_id;

/*
 *******************************************************************************
 * Data manipulating unctions
 *******************************************************************************
 */

/*
This function was a test for first tests
int generate_ethernet_frame(char *input_buffer, size_t max_length) {
    // Definimos una trama de ejemplo (los mismos bytes de tu ejemplo anterior)
    uint8_t frame_bytes[] = {
        0x94, 0x83, 0xc4, 0x22, 0x3e, 0xd4, 0x08, 0x00,
        0x27, 0xf0, 0xbb, 0x09, 0x08, 0x00, 0x45, 0x00,
        0x00, 0x54, 0x0f, 0x9d, 0x40, 0x00, 0x40, 0x01,
        0xf9, 0x47, 0xc0, 0xa8, 0x58, 0x72, 0xc0, 0xa8,
        0x58, 0x01, 0x08, 0x00, 0xa7, 0xcd, 0x00, 0x03,
        0x00, 0x01, 0x07, 0xf4, 0xb6, 0x67, 0x00, 0x00,
        0x00, 0x00, 0xc9, 0xff, 0x09, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
        0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
        0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
        0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d,
        0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37
    };

    size_t frame_size = sizeof(frame_bytes);
    // Cada byte se convertirá en dos caracteres hexadecimales, más el terminador nulo
    if (max_length < frame_size * 2 + 1) {
        printf("Buffer demasiado pequeño para la trama Ethernet.\n");
        return -1;
    }

    // Convertir cada byte en su representación hexadecimal
    for (size_t i = 0; i < frame_size; i++) {
        sprintf(input_buffer + (i * 2), "%02x", frame_bytes[i]);
    }
    input_buffer[frame_size * 2] = '\0';

    return frame_size * 2;  // Retornamos la longitud de la cadena generada
}*/

int transmit_data(char *data)
{
    int len = strlen(data);

    // Shift data to make room for caster_rx_id at the start
    for (int i = len; i >= 0; i--) {
        data[i + 1] = data[i];
    }
    data[0] = caster_rx_id[0];
    data[strlen(data)] = '%';
    data[strlen(data) + 1] = '\0';

    int ret = us_caster_tx(caster_handler, NULL, (uint8_t *)data, strlen(data));
    if (!IS_SUCCESS(ret)) {
        printf("Error transmitting data: %d\n", ret);
        return -1;
    }
    printf("Character transmitted: %s\n", data);

    usleep(100000); // Sleep for 100 ms

    return 0;
}

void app_signal_handler(int sig_num)
{
    if (sig_num == SIGINT || sig_num == SIGTERM) {
        printf("Received signal %d, shutting down...\n", sig_num);
        app_state = APP_STATE_DESTROY;
    }
}

int capture_and_transmit_icmp()
{
    int sock;
    struct sockaddr_ll addr;
    uint8_t buffer[MAX_BUFFER_SIZE];

    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    // Bind socket to a specific interface (e.g., enp0s3)
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "enp0s3", IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) == -1) {
        perror("ioctl");
        close(sock);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sock);
        return -1;
    }

    printf("Capturing ICMP packets on enp0s3...\n");

    while (app_state != APP_STATE_DESTROY) {
        ssize_t len = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
        if (len < 0) {
            perror("recvfrom");
            break;
        }

        if (len < sizeof(struct ethhdr)) {
            continue;
        }

        struct ethhdr *eth = (struct ethhdr *)buffer;
        if (ntohs(eth->h_proto) != ETH_P_IP) {
            continue; // Not IPv4
        }

        struct iphdr *ip = (struct iphdr *)(buffer + sizeof(struct ethhdr));
        if (ip->protocol != IPPROTO_ICMP) {
            continue; // Not ICMP
        }

        // Convert Ethernet frame to hex string for transmission
        char hex_buffer[MAX_BUFFER_SIZE * 2 + 1];
        for (size_t i = 0; i < len; i++) {
            sprintf(hex_buffer + (i * 2), "%02x", buffer[i]);
        }
        hex_buffer[len * 2] = '\0';

        // Transmit the hex string
        transmit_data(hex_buffer);
    }

    close(sock);
    return 0;
}

/*
 *******************************************************************************
 * System interaction function
 *******************************************************************************
 */

int app_setup_signals(void)
{
    stack_t sigstack;
    struct sigaction sa;
    int ret = -1;

    sigstack.ss_sp = malloc(SIGSTKSZ);
    if (sigstack.ss_sp == NULL) {
        return -1;
    }
    sigstack.ss_size = SIGSTKSZ;
    sigstack.ss_flags = 0;
    if (sigaltstack(&sigstack, NULL) == -1) {
        perror("sigaltstack()");
        goto END;
    }

    sa.sa_handler = app_signal_handler;
    sa.sa_flags = SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) != 0) {
        perror("sigaction()");
        goto END;
    }
    if (sigaction(SIGTERM, &sa, NULL) != 0) {
        perror("sigaction()");
        goto END;
    }

    ret = 0;
END:
    return ret;
}

void app_teardown_signal(void)
{
    stack_t sigstack;
    uint8_t *allocated_stack = NULL;

    /* Get the signal stack pointer so we can free the memory */
    sigaltstack(NULL, &sigstack);
    allocated_stack = (uint8_t *)sigstack.ss_sp;

    /* Tell the kernel to stop using it */
    sigstack.ss_sp = NULL;
    sigstack.ss_flags = SS_DISABLE;
    sigstack.ss_size = SIGSTKSZ;
    sigaltstack(&sigstack, NULL);

    /* Free old stack */
    free(allocated_stack);
}

static int32_t app_set_thread_name_and_priority(pthread_t thread, app_thread_type_t type, char *p_name, int32_t priority)
{
    int32_t result = 0;
    caster_thread_info_t limited_thread_config = {0};

#ifdef __SET_PRIORITY__
    int policy = SCHED_FIFO;
    struct sched_param param;
#endif  // __SET_PRIORITY__
    if (p_name == NULL) {
        return -1;
    }

    /* Check thread priority is in the limited range */
    us_caster_thread_info_get(&limited_thread_config);

    if (APP_THREAD_TX == type) {
        /* Check the limited range for tx thread priority */
        if ((priority < limited_thread_config.tx_thread_priority_low) || (priority > limited_thread_config.tx_thread_priority_high)) {
            /* Thread priority is out of range */
            printf("The tx thread priority is out of range (%d-%d): %d \n", limited_thread_config.tx_thread_priority_low, limited_thread_config.tx_thread_priority_high, priority);
            return -1;
        }
#ifdef __SET_PRIORITY__
        policy = SCHED_RR;
#endif  // __SET_PRIORITY__
    }
    else if (APP_THREAD_RX == type) {
        /* Check the limited range for rx thread priority */
        if ((priority < limited_thread_config.rx_thread_priority_low) || (priority > limited_thread_config.rx_thread_priority_high)) {
            /* Thread priority is out of range */
            printf("The rx thread priority is out of range (%d-%d): %d \n", limited_thread_config.rx_thread_priority_low, limited_thread_config.rx_thread_priority_high, priority);
            return -1;
        }
    }
    else {
        /* Target thread type is unknown */
        printf("The thread type is unknown: %d \n", type);
        return -1;
    }

    result = pthread_setname_np(thread, p_name);
    if (result != 0) {
        printf("Can't set thread name: %d (%s)\n", result, strerror(result));
        return -1;
    }

#ifdef __SET_PRIORITY__
    param.sched_priority = priority;
    result = pthread_setschedparam(thread, policy, &param);
    if (result != 0) {
        printf("Can't set thread priority: %d (%s)\n", result, strerror(result));
        return -1;
    }
#endif  // __SET_PRIORITY__
    return 0;
}

/*
 *******************************************************************************
 * MAIN function
 *******************************************************************************
 */

int main(int argc, char **argv)
{
    caster_thread_info_t caster_thread_info;
    int ret;

    setbuf(stdout, NULL);
    if (argc != 3) {
        printf("Usage:\n");
        printf("  ./data_transmitter [IP] [caster_rx_id] \n");
        printf("  For example,\n");
        printf("    ./data_transmitter 192.168.1.3 1");
        return -1;
    }
    caster_rx_id = argv[2];
    
    ret = app_setup_signals();
    if (ret != 0) {
        printf("Failed to set up signals\n");
        return -1;
    }

    caster_comm_config_t config = {.ip = argv[1], .caster_id = 0, .caster_comm_mode = CASTER_MODE_TX};

    us_caster_init();

    /* init v2xcast */
    ret = us_caster_create(&caster_handler, &config);
    if (!IS_SUCCESS(ret)) {
        printf("Cannot link to V2Xcast Service, V2Xcast Service create ret: [%d]!\n", ret);
        printf("Please confirm network connection by pinging the Unex device then upload a V2Xcast config to create a V2Xcast Service.\n");
        return -1;
    }
    printf("-----------------------\n");

    ret = capture_and_transmit_icmp();

    /* If the example is run in Unex device, please use the below functions to set tx and rx message threads name and priority */
    /* If the example is run on other platforms, it is optional to set tx and rx message threads name and priority */
    us_caster_thread_info_get(&caster_thread_info);
    ret = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_TX, caster_thread_info.tx_thread_name, caster_thread_info.tx_thread_priority_low);

    us_caster_release(caster_handler);

    us_caster_deinit();

    app_teardown_signal();
    return EXIT_SUCCESS;
}

