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
#include <arpa/inet.h>


#include "us_caster_service.h"
#include "error_code_user.h"

/*
********************************************************************************
Structures definition
********************************************************************************
*/

// Estructura para el encabezado Ethernet (14 bytes)
struct ethernet_header {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t eth_type;
};

// Estructura para el encabezado IP (mínimo 20 bytes)
struct ip_header {
    uint8_t version_ihl;      // Versión y longitud de encabezado
    uint8_t tos;              // Tipo de servicio
    uint16_t total_length;    
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
};

// Estructura para el encabezado ICMP (8 bytes)
struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
};

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

/*
********************************************************************************
Auxiliary functions
********************************************************************************
*/

// Función auxiliar para imprimir una MAC
static void print_mac(const uint8_t *mac) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Función auxiliar para imprimir una IP
static void print_ip(uint32_t ip) {
    struct in_addr addr;
    addr.s_addr = ip;
    printf("%s\n", inet_ntoa(addr));
}

// Función auxiliar para convertir un dígito hexadecimal a entero
static int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1; // Error: carácter inválido
}

/*
 *******************************************************************************
 * System interaction function
 *******************************************************************************
 */

void app_signal_handler(int sig_num)
{
    if (sig_num == SIGINT) {
        printf("SIGINT signal!\n");
    }
    if (sig_num == SIGTERM) {
        printf("SIGTERM signal!\n");
    }
    app_state = APP_STATE_DESTROY;
}

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

static void app_teardown_signal(void)
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

    return;
}

static int32_t app_set_thread_name_and_priority(pthread_t thread, app_thread_type_t type, char *p_name, int32_t priority)
{
    int32_t result = 0;
    caster_thread_info_t limited_thread_config = {0};

    if (p_name == NULL) {
        return -1;
    }

    /* Check thread priority is in the limited range */
    us_caster_thread_info_get(&limited_thread_config);

    if (APP_THREAD_RX == type) {
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

    return 0;
}

/*
*******************************************************************************
* Data manupilating function
*******************************************************************************
*/

static void handle_received_data(uint8_t *rx_buf, size_t len) {
    // Verificar que tengamos al menos el encabezado Ethernet (14 bytes)
    if (len < 14) {
        printf("Datos insuficientes para encabezado Ethernet\n");
        return;
    }
    
    // Interpretar el encabezado Ethernet
    struct ethernet_header *eth = (struct ethernet_header *) rx_buf;
    printf("== Encabezado Ethernet ==\n");
    printf("MAC Destino: ");
    print_mac(eth->dest_mac);
    printf("MAC Origen: ");
    print_mac(eth->src_mac);
    
    uint16_t eth_type = ntohs(eth->eth_type);
    printf("Tipo de protocolo: 0x%04x\n", eth_type);
    
    // Solo procesamos IPv4 (0x0800)
    if (eth_type == 0x0800) {
        // Verificar tamaño mínimo para el encabezado IP
        if (len < 14 + sizeof(struct ip_header)) {
            printf("Datos insuficientes para encabezado IP\n");
            return;
        }
        struct ip_header *ip = (struct ip_header *)(rx_buf + 14);
        
        // La longitud del encabezado IP está en los 4 bits bajos
        int ip_header_length = (ip->version_ihl & 0x0F) * 4;
        if (len < 14 + ip_header_length) {
            printf("Datos insuficientes para la longitud indicada del encabezado IP\n");
            return;
        }
        
        printf("== Encabezado IP ==\n");
        printf("Versión: %d\n", ip->version_ihl >> 4);
        printf("Longitud de encabezado: %d bytes\n", ip_header_length);
        printf("Protocolo: %d\n", ip->protocol);
        printf("IP Origen: ");
        print_ip(ip->src_ip);
        printf("IP Destino: ");
        print_ip(ip->dest_ip);
        
        // Si el protocolo es ICMP (valor 1), interpretar el encabezado ICMP
        if (ip->protocol == 1) {
            if (len < 14 + ip_header_length + sizeof(struct icmp_header)) {
                printf("Datos insuficientes para encabezado ICMP\n");
                return;
            }
            struct icmp_header *icmp = (struct icmp_header *)(rx_buf + 14 + ip_header_length);
            printf("== Encabezado ICMP ==\n");
            printf("Tipo: %d\n", icmp->type);
            printf("Código: %d\n", icmp->code);
            printf("Checksum: 0x%04x\n", ntohs(icmp->checksum));
        }
    } else {
        printf("Tipo de protocolo no soportado (solo IPv4 se procesa en este ejemplo)\n");
    }
}

// Función de preprocesado de datos recibidos
static void preprocess_received_data(uint8_t *rx_buf, size_t len) {
    // Verificar que haya al menos un caracter de ID, dos hex y el '%' final
    if (len < 4) {
        printf("Datos recibidos demasiado cortos.\n");
        return;
    }

    // Verificar que el último carácter sea '%'
    if (rx_buf[len - 1] != '%') {
        printf("Formato de datos incorrecto: falta el delimitador '%%'.\n");
        return;
    }

    // La trama hexadecimal está entre el primer carácter (ID) y el '%' final.
    // Es decir, desde rx_buf[1] hasta rx_buf[len-2]
    size_t hex_len = len - 2;

    // Cada byte se representa con dos caracteres hexadecimales,
    // por lo que la longitud debe ser par.
    if (hex_len % 2 != 0) {
        printf("Formato hexadecimal incorrecto: longitud impar.\n");
        return;
    }

    size_t binary_len = hex_len / 2;
    uint8_t *binary_data = malloc(binary_len);
    if (binary_data == NULL) {
        printf("Error al asignar memoria.\n");
        return;
    }

    // Convertir la cadena hexadecimal en datos binarios.
    for (size_t i = 0; i < binary_len; i++) {
        int high = hex_char_to_int(rx_buf[1 + 2 * i]);
        int low  = hex_char_to_int(rx_buf[1 + 2 * i + 1]);
        if (high < 0 || low < 0) {
            printf("Carácter hexadecimal inválido.\n");
            free(binary_data);
            return;
        }
        binary_data[i] = (high << 4) | low;
    }

    // Llamar a handle_received_data() con el buffer binario
    handle_received_data(binary_data, binary_len);

    free(binary_data);
}

/*
*******************************************************************************
MAIN function
*******************************************************************************
*/

int main(int argc, char **argv)
{
    caster_handler_t caster_handler = INVALID_CASTER_HANDLER;
    us_caster_rx_info_t rx_info;
    uint8_t rx_buf[US_CASTER_PKT_SIZE_MAX];
    size_t len;
    int ret;

    setbuf(stdout, NULL);
    if (argc != 2) {
        printf("Usage:\n");
        printf("  ./data_receiver [IP]\n");
        printf("  For example,\n");
        printf("    ./data_receiver 192.168.1.3\n");
        return -1;
    }

    ret = app_setup_signals();
    if (ret != 0) {
        printf("Failed to set up signals\n");
        return -1;
    }

    us_caster_init();

    caster_comm_config_t config = {.ip = argv[1], .caster_id = 0, .caster_comm_mode = CASTER_MODE_RX};

    ret = us_caster_create(&caster_handler, &config);
    if (!IS_SUCCESS(ret)) {
        printf("Cannot link to V2Xcast Service, V2Xcast Service create ret: [%d] %s!\n", ret, ERROR_MSG(ret));
        printf("Please confirm network connection by ping the Unex device then upload a V2Xcast config to create a V2Xcast Service.\n");
        return -1;
    }

    caster_thread_info_t thread_info;
    us_caster_thread_info_get(&thread_info);
    ret = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_RX, thread_info.rx_thread_name, thread_info.rx_thread_priority_low);
    if (ret != 0) {
        printf("Failed to set thread name and priority\n");
        return -1;
    }

    printf("Waiting for data...\n");

    while (app_state != APP_STATE_DESTROY) {
        ret = us_caster_rx(caster_handler, &rx_info, rx_buf, sizeof(rx_buf), &len);
        if (IS_SUCCESS(ret)) {
            preprocess_received_data(rx_buf, len);
        } else {
            printf("Error receiving data: %d\n", ret);
        }
    }

    us_caster_release(caster_handler);
    us_caster_deinit();

    app_teardown_signal();

    return EXIT_SUCCESS;
}

