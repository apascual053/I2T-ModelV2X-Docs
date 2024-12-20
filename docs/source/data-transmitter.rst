Entiendo `data_transmitter_main.c`
==================================

En esta página se describe la composición y definición del script ``data_transmitter_main.c``. Este script es utilizado para la transmisión personalizada de un string en una comunicación V2X. Para más información leer el apartado :doc:`basic`.

Este código está escrito es lenguaje ``C``.

Cabecera
--------

En primer lugar se incluyen directivas de pre-procesado para:

- Extensiones especificas de GNU (más allá de POSIX)
- Hilos
- Señales
- Flujos de entrada y salida
- Cadenas de caracteres
- Constantes y funciones de bajo nivel
- Funciones estándar de propósito general
- Manipulación y medición del tiempo
- Funciones específicas para manejo del tiempo

.. code:: c

    #define _GNU_SOURCE
    #include <pthread.h>
    #include <signal.h>
    #include <stdio.h>
    #include <string.h>
    #include <unistd.h>
    #include <stdlib.h>
    #include <time.h>
    #include <sys/time.h>

Después, incluye dos archivos de cabecera personalizados. Estos permite hacer uso de la API del SDK de los módulos Unex utilizados en este proyecto.

.. code:: c

    #include "us_caster_service.h"
    #include "error_code_user.h"

A continuación, encontramos la definición de macros.

.. code:: c
    
    #define MAX_BUFFER_SIZE 1024

El código define varios tipos de datos para mejorar la legibilidad y organización del script. Entre ellos se encuentran:

- Tipos de datos de aplicación: usados para controlar el estado de la aplicación.
- Tipos de datos de hilo: determinan el rol de transmisión o recepción, activando diferentes partes del código según el caso.

.. code:: c

    typedef enum app_state_type {
        APP_STATE_DESTROY = 0,
        APP_STATE_INITIAL = 1,
        APP_STATE_READY = 2,
    } app_state_type_t;

    typedef enum app_thread_type {
        APP_THREAD_TX = 0,
        APP_THREAD_RX = 1
    } app_thread_type_t;


La sección de cabecera concluye con la declaración de varias variables globales:

- Inicialización de un tipo de dato de aplicación
- Un ``caster_handler`` inicializado con un valor no valido.
- Un puntero ``char`` que apunta a ID del caster.

.. code:: c

    static uint8_t app_state = APP_STATE_INITIAL;
    static caster_handler_t caster_handler = INVALID_CASTER_HANDLER;
    static char *caster_rx_id;

Funciones
---------

Leer input del usuario
^^^^^^^^^^^^^^^^^^^^^^

La función ``read_user_input()`` recibe como parámetros:

* Un puntero ``char`` que apunta al comienzo del buffer con el contenido proporcionado.
* Longitud máxima.

El funcionamiento es el siguiente:

* Se le solicita un input de tipo ``string`` y comprueba que no este a ``NULL``.

  * En caso de ``NULL``, genera un error y termina la ejecución del programa.

* Si el ``string`` proporcionado contiene el carácter ``\n`` , se elimina.

  * En consecuencia, se decrementa su longitud ``input_length``.

.. code:: c

    int read_user_input(char *input_buffer, size_t max_length)
    {
        printf("Enter a string: ");
        if (fgets(input_buffer, max_length, stdin) == NULL) {
            fprintf(stderr, "Error reading input.\n");
            return -1;
        }

        // Remove trailing newline if present
        size_t input_length = strlen(input_buffer);
        if (input_length > 0 && input_buffer[input_length - 1] == '\n') {
            input_buffer[input_length - 1] = '\0';
            input_length--;
        }

        return input_length;
    }

Transmisión de la información
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

La función ``transmit_data()`` es la que función que, haciendo uso de la API del SDK de los módulos Unex, envía la información. Concretamente, hace uso de la función ``us_caster_tx()`` para transmitir la información.

Recibe como parámetros un puntero a ``data``.

El funcionamiento es el siguiente:

* Obtiene la longitud de la cadena a transmitir
* Desplaza todos los caracteres una posición a la derecha
* Añade información sobre ID del caster

  * Un byte inicial al principio de ``data``.
  * Un símbolo ``%`` al final de ``data``. Para ello, desplazada una posición a la derecha el símbolo de final de ``string`` (``\0``).

* Hace uso de ``us_caster_tx()``

  * En caso de error, notifica el estado por pantalla y termina la ejecución del programa.
  * En ausencia de error, notifica la transmisión de ``data`` por pantalla.

* Por último, bloquea la ejecución del programa durante 100 ms, para dar tiempo al receptor a procesar la información

Si todo se ha ejecutado correctamente, se vuelve un ``0``.

.. code:: c

    int transmit_data(char *data)
    {
        int len = strlen(data); // Obtener la longitud actual de la cadena
        
        // Desplazar todos los caracteres a la derecha para hacer espacio para el nuevo carácter
        for (int i = len; i >= 0; i--) {
            data[i + 1] = data[i];
        }

        // Añadir el id del caster
        data[0] = caster_rx_id[0];
        data[strlen(data)] = '%';
        data[strlen(data) + 1] = '\0';
        
        int ret = us_caster_tx(caster_handler, NULL, (uint8_t *)data, strlen(data)); //API SDK envio de data
        if (!IS_SUCCESS(ret)) {
            printf("Error transmitting data: %d\n", ret);
            return -1;
        }
        printf("Character transmitted: %s\n", data);

        usleep(100000); // Sleep for 100 ms between transmissions

        return 0;
    }

Función de recepción de señales
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

La función ``app_signal_handler()``  es la función que se encarga de manejar las señales del programa. En concreto, las señales ``SIGINT`` y ``SIGTERM``. La primera de ellas hace referencia a una señal típicamente generada cuando el usuario pulsa ``Ctrl+C``. La segunda, es la señal estándar de solicitud de finalización del programa.

Cuando se recibe alguna de estas señales, el programa notifica que procede a la finalización y cambia el estado a ``APP_STATE_DESTROY``.

.. code:: c

    void app_signal_handler(int sig_num)
    {
        if (sig_num == SIGINT || sig_num == SIGTERM) {
            printf("Received signal %d, shutting down...\n", sig_num);
            app_state = APP_STATE_DESTROY;
        }
    }

Configuración de recepción de señales con pila alternativa
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

La función ``app_setup_signals()`` se utiliza para configurar la recepción de señales mediante una pila alternativa y a través de la función ``app_signal_handler()``.

En primer lugar, se declaran las siguientes variables:

* ``stack_t sigstack``: Una estructura que se usa para el manejo de pilas (espacio de memoria) en las señales. Normalmente, una pila separada evita conflictos con otras pilas del sistema.
* ``struct sigaction sa``: Una estructura que define cómo manejar las señales.
* El valor de retorno en forma de ``int ret``.

El funcionamiento es el siguiente:

* Reserva de espacio para la pila de memoria alternativa (``malloc``):

  * En caso de que no se pueda realizar esta tarea, se finaliza la ejecución del programa.

* Configuración de la pila alternativa:

  * Se indica que la pila tiene tamaño ``SIGSTKSZ``, un valor estándar para el manejo de pilas
    alternativas en el contexto de manejo de señales.
  * Como la pila se usará de manera estándar, se añade ``sigstack.ss_flags = 0``.
  * ``sigaltstack(&sigstack, NULL)`` indica al sistema que use la pila alternativa
    (``sigstack``) que hemos preparado para manejar las señales.

    * Si no se puede configurar, se procede a finalizar la ejecución de la función
      y se devolverá el valor que ``ret`` tenga en ese instante.

* Configuración del manejador de señales, para lo cual, lo primero que se ha hecho es definir ``struct sigaction sa``.
  
  * ``sa.sa_handler`` especifica cuál es la función que se debe ejecutar cuando
    se recibe una señal. En este caso, la función ``app_signal_handler``.
  * ``sa.sa_flags = SA_ONSTACK`` indica que la señal se manejará usando una pila alternativa
    previamente configurada con ``sigaltstack``.
  * ``sigemptyset(&sa.sa_mask)`` configura una máscara de señales, es decir, define qué señales
    deben ser bloqueadas mientras se maneja la señal actual. Aquí, no estamos bloqueando ninguna señal
    (ya que la función ``sigemptyset()`` vacía la máscara de señales), por lo que todas las señales
    pueden ser procesadas.

* Registro de las señales que se deben manejar:

  * Se registra la señal ``SIGINT`` mediante ``sigaction(SIGINT, &sa, NULL)``.
  * Se registra la señal ``SIGTERM`` mediante ``sigaction(SIGTERM, &sa, NULL)``.

.. code:: c

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

Desactivación y limpieza de recursos asociados a la recepción de señales con pila alternativa
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

La función ``app_teardown_signal()``, al contrario que la anterior, se utiliza para desactivar y liberar la pila alternativa configurada para señales. En ella se limpian los recursos usados por ``sigaltstack``.

El funcionamiento es el siguiente:

* Obtención de la pila alternativa actual:

  * ``sigaltstack(NULL, &sigstack)`` obtiene la configuración actual de la pila de señales
    (si está activa o no) y guarda su información en la estructura ``sigstack``.
  * ``sigstack.ss_sp`` apunta a la memoria reservada para la pila alternativa. Esto se guarda en
    ``allocated_stack`` para luego liberarla.

* Desactivación de la pila alternativa:

  * ``sigstack.ss_sp = NULL`` quita el puntero de la pila alternativa, dejando claro que ya no se usará.
  * ``sigstack.ss_flags = SS_DISABLE`` indica al sistema que desactive el uso de la pila alternativa
    para manejar señales.
  * ``sigaltstack(&sigstack, NULL)`` aplica estos cambios, desactivando oficialmente la pila alternativa.

* Liberación de la memoria reservada:

  * Libera la memoria previamente reservada para la pila alternativa con ``malloc()`` en ``app_setup_signals``.

.. code:: c

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

Configuración del nombre y prioridad del hilo
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

La función ``pp_set_thread_name_and_priority()`` establece el nombre y la prioridad de un hilo según su tipo (TX o RX).

* Verifica si el nombre del hilo (``p_name``) es ``NULL``. Si es así, retorna ``-1``.

* Obtiene los límites de prioridad para los hilos de transmisión (TX) y recepción (RX).

* Valida que la prioridad esté dentro de los límites permitidos según el tipo de hilo:

  * Para hilos de transmisión (``APP_THREAD_TX``), se verifica que la prioridad esté en el rango permitido para TX.
  * Para hilos de recepción (``APP_THREAD_RX``), se verifica que la prioridad esté en el rango permitido para RX.

* Si el tipo de hilo no es reconocido, retorna ``-1``.

* Establece el nombre del hilo usando `pthread_setname_np()`. Si falla, retorna ``-1``.

* Si se permite la configuración de prioridades (``__SET_PRIORITY__``), se establece la prioridad del hilo usando ``pthread_setschedparam()``. Si falla, retorna ``-1``.

* Retorna ``0`` en caso de éxito.

.. code:: c

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


Función principal
-----------------

En primer lugar, después de haber declarado alguna variables generales, se desactiva el buffer de ``stdout``  de tal forma que se presenta todo en pantalla sin esperar a completar o rellenar dicho buffer.

.. code::c

    setbuf(stdout, NULL);xd

Invocar el programa
^^^^^^^^^^^^^^^^^^^

En segundo lugar, se verifica la correcta llamada al programa. Esta precisa de tres argumento: el nombre del programa, la dirección IP a la que se enviará la información, y el ID receptor del caster).

En caso de que esta llamada no se haga correctamente, se procede a la finalización del programa.

En el caso de haberse realizado correctamente la llamada, se asigna a ``caster_rx_id`` el tercer parámetro (de índice ``2`` ) pasado por consola.

.. code:: c

    if (argc != 3) {
        printf("Usage:\n");
        printf("  ./data_transmitter [IP] [caster_rx_id] \n");
        printf("  For example,\n");
        printf("    ./data_transmitter 192.168.1.3 1");
        return -1;
    }
    caster_rx_id = argv[2];

Configuración de las señales
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

En tercer lugar, se configura el sistema para hacer uso de la pila alternativa durante el manejo de señales a través de la función ``app_setup_signals()``.

En caso de no haberse podido realizar correctamente, se procede a presentar el fallo por pantalla y a terminar la ejecución del problema.

.. code:: c

    ret = app_setup_signals();
    if (ret != 0) {
        printf("Failed to set up signals\n");
        return -1;
    }

Configuración e inicialización del caster
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``struct caster_conm_config_t`` es una estructura que define la configuración de la comunicación. Consta de los siguientes miembros públicos:

* ``const char *ip`` la dirección del modulo (el caster se comunica con el módulo).
* ``uint16_t caster_id`` un valor que identifica inequivocamente al caster.
* ``caster_comm_mode_e caster_comm_mode`` un valor de la enumeración que caracteriza el modo de comunicación. Este puede tomar los valores:

  * ``CASTER_MODE_TX``
  * ``CASTER_MODE_TX``
  * ``CASTER_MODE_SPLMNT``
  * ``CASTER_MODE_POTI``
  * ``CASTER_MODE_MAX``

Por otra parte, la función ``us_caster_init()`` inicia el contexto del caster.

Tanto la estructura como esta última función están definidas en ``us_caster_service.h``.

.. code:: c

    caster_comm_config_t config = {.ip = argv[1], .caster_id = 0, .caster_comm_mode = CASTER_MODE_TX};

    us_caster_init();

Conexión con el servicio V2Xcast
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Otro de los servicios proporcionado por ``us_caster_service.h`` es la función ``us_caster_create()``. Esta función crea una instancia del caster y levanta los recursos necesarios para su funcionamiento.

En caso de devolver un código de error, se indicaria que no se es capaz de crear la conexión con el módulo (recordar que el caster se encarga de la comunicación pc-módulo) y termina con la ejecución del programa.

.. code:: c

    ret = us_caster_create(&caster_handler, &config);
    if (!IS_SUCCESS(ret)) {
        printf("Cannot link to V2Xcast Service, V2Xcast Service create ret: [%d]!\n", ret);
        printf("Please confirm network connection by pinging the Unex device then upload a V2Xcast config to create a V2Xcast Service.\n");
        return -1;
    }
    printf("-----------------------\n");

Lectura de datos del usuario
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Seguidamente, se procede a solicitar el usuario la información (``string``) a transmitir, y se hace todo el procesado. Esto se hace mediante la función ``read_user_input()``. En caso de recibir una respuesta negativa, se termina la ejecución del programa.

.. code:: c

    input_length = read_user_input(input_string, sizeof(input_string));
    if (input_length < 0) {
        return -1;
    }

Transmisión de los datos
^^^^^^^^^^^^^^^^^^^^^^^^

Una vez se tiene la información a transmitir, se realiza la propia transmisión mediante la función ``transmit_data()`` previamente introducida. Si se produce algún error en la transmisión (``ret !=0``), no se termina la ejecución del programa pero se notifica del error por pantalla.

.. code:: c

    ret = transmit_data(input_string);
    if (ret != 0) {
        printf("Error transmitting data.\n");
    }

Configuración de los hilos de tranmisión
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A continuación, a través previamente explicada ``app_set_thread_name_and_priority()``, se configuran los hilos de la forma deseada. La función ``us_caster_thread_info_get()`` obtiene la información necesaria del caster (es una función de la API) y deposita esa información en la variable que se le pasa como parámetro y que está definida al principio de ``main``: ``caster_thread_info``.

.. code:: c

    us_caster_thread_info_get(&caster_thread_info);
    ret = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_TX, caster_thread_info.tx_thread_name, caster_thread_info.tx_thread_priority_low);

Liberación de recursos
^^^^^^^^^^^^^^^^^^^^^^

Se libera la instancia del caster que habia creado así como las recursos utilizados por dicha instancia. Para ello, se hace uso de la función ``us_caster_realease()`` de la API del SDK.

.. code:: c

    us_caster_release(caster_handler);

Limpieza de señales
^^^^^^^^^^^^^^^^^^^

Se desinicializa el contexto iniciado para la creación del caster mediante la función ``us_caster_deinit()`` de la API del SDK.

.. code:: c

    us_caster_deinit();

Finalización de señales
^^^^^^^^^^^^^^^^^^^^^^^

Por último, se desactiva y limpian los recursos usando por la pila alternativa para el manejo de señales. En el caso de que todo se haya ejecutado correctamente, se proporcional la salida correspondiente ``EXIT_SUCESS``.

.. code:: c

        app_teardown_signal();
        return EXIT_SUCCESS;

A continuación, se muestra la función ``main`` completa.

.. code:: c

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

        /* thread of data transmitter */
        char input_string[MAX_BUFFER_SIZE]; // Buffer to hold user input
        size_t input_length;
        
        input_length = read_user_input(input_string, sizeof(input_string));
        if (input_length < 0) {
            return -1;
        }

        ret = transmit_data(input_string);
        if (ret != 0) {
            printf("Error transmitting data.\n");
        }

        /* If the example is run in Unex device, please use the below functions to set tx and rx message threads name and priority */
        /* If the example is run on other platforms, it is optional to set tx and rx message threads name and priority */
        us_caster_thread_info_get(&caster_thread_info);
        ret = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_TX, caster_thread_info.tx_thread_name, caster_thread_info.tx_thread_priority_low);

        us_caster_release(caster_handler);

        us_caster_deinit();

        app_teardown_signal();
        return EXIT_SUCCESS;
    }