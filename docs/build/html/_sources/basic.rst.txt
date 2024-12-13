Comunicación básica y personalizada
===================================

Este capítulo proporciona las instrucciones necesarias para iniciar y configurar la maqueta V2X más básicas utilizando el SDK proporcionado por el proveedor de los módulos V2X comprados por el grupo I2T, `Unex <https://unex.com.tw/en/product/>`_.

En primer lugar, lugar se va a poner en marcha la comunicación entre dos módulos V2X que intercambiarán mensajes BSM (*Basis Safety Message*). En segundo lugar, haremos uso de V2X para intercambiar mensajes personalizados, fueras de los estándares de seguridad vehicular.

Para trabajar con los módulos Unex, en primera instancia, haremos uso del **Entorno de desarrollo y soporte** proporcionado junto al *hardware* de los propios módulos V2X. En este entorno se encuentra el siguiente material.

.. code-block:: text

    v200/v2x-files/
    ├── BSP
    ├── DOC
    ├── LTS
    └── SDK
    4 directories, 0 files

1. **Board Support Package (BSP).** Donde se encuentran configuraciones, *drivers* y herramientas necesarias para interactuar con *hardware* específico.
2. **Documentation (DOC).** Donde se encuentran manuales y documentos básicos sobre los módulos utilizados y otro equipamiento Unex de la familia.
3. **Long Term Support (LTS).** Donde se encuentran archivos relacionados con una versión de soporte a largo plazo, como actualizaciones, parches o configuraciones para garantizar la estabilidad.
4. **Software Development Kit (SDK).** Incluye herramientas para trabajar con los módulos con bibliotecas, ejemplos de código y otros recursos para el desarrollo de *software* relacionado con el sistema. En este caso, la versión es la ``v.200``.

Comunicacion básica
-------------------

.. admonition:: **Objetivo**

   Poner en marcha una primera comunicación entre dos módulos V2X, creando una pequeña red celular C-V2X de tipo V2V. Para ello, intercambiar mensajes BSM.

Para llevar a cabo este objetivo se hara uso del SDK, que proporciona formatos de mensaje previamente configurados y ejemplos. Esto permite abstraerse en este ejercicio del contenido de la informacíón intercambiada.

.. note::

	La documentación general viene en formato ``html`` y se puede encontrar en ``v200/v2x-files/DOC/index.html``.

	.. code-block:: console

		$ find v200/v2x-files/ -name "index.html"
		./DOC/index.html

	En el apartado *4. Application Note* del capítulo *V2Xcast*, se describe qué tipos de mensajes se estandarizan tanto en EU (Unión Europea) como en US (United States). No obstante, no es de demasiado interés ya que el objetivo de este este proyecto se basa en una comunicación personalizada.

	Existe cierta correspondencia entre esta descripción y el apartado *1. Quick Start Guide* de la sección *V2Xcast* de la documentación oficial de Unex.


Para el **modulo transmisor**, haremos uso del siguiente material

1. Un modulo Unex C-V2X/802.11p con su respectivo adaptador USB.
2. Una VM (máquina virtual) con Ubuntu 22.04.3 LTS (codename: *Jammy*) y arquitectura CPU **x86_64** gestionada por VirtualBox.

.. warning::

	Para que la VM pueda reconocer el módulo, es necesario agregar un filtro de dispositivos USB en la configuración.

.. admonition:: ¿Qué tipo de arquitectura tiene mi equipo?

	En Linux, para saber que tipo de arquitectura se tiene tan sólo se debe ejecutar ``uname -m`` sobre una terminal. Las respuestas típicas son: ``x86_64`` (x86 de 64 bits), ``armv7_hf`` (ARM de 32 bits) y ``armv8_64`` (ARM de 64 bits).

Pasos a seguir en el módulo transmisor
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Dentro de la VM de Ubuntu, colocamos los ficheros del entorno de desarrollo, ``BSM``, ``DOC``, ``LTS`` y ``SDK`` en un directorio deseado.

.. _descomprimir_sdk:

Descomprimir el SDK
"""""""""""""""""""

Dentro de la carpeta ``SDK``, encontramos varios fichero ``.tar``. Cada uno de ellos corresponde con un tipo de arquitectura de CPU. Para ello, se deberá extraer el fichero correspondiente a cada caso. Al descomprimir, tendremos las herramientas proporcionadas por el SDK disponibles.

.. code-block:: console

	$ cd SDK/
	$ tar zxvf us_v2xcast_sdk-x86_64_linux-v2.0.1.tgz

Dentro de este directorio encontramos los siguiente:

.. code-block:: text

	us_v2xcast_sdk
	├── bin
	├── demo_certificates
	├── example
	├── inc
	├── lib
	├── sh
	├── tool
	└── VERSION.txt

	7 directories, 1 file

.. _compilar_ejemplos:

Compilar los ficheros de ejemplo
""""""""""""""""""""""""""""""""

Para este ejercicio vamos a hacer uso de los ejemplos de la carpeta ``example``, donde se alojan varios directorio que incluyen ficheros ``.c`` asociados a las interacciones de diferentes tipos de mensajes estándarizados.

.. code-block:: text

	example
	├── libj2735msg
	├── Makefile
	├── v2xcast_bsm
	├── v2xcast_event_detector_us
	├── v2xcast_map
	├── v2xcast_multiple_subscrible_us
	├── v2xcast_probe_data_collection
	├── v2xcast_rtcm
	├── v2xcast_rtk_application_us
	├── v2xcast_spat
	├── v2xcast_static_memory_usage_us
	├── v2xcast_tim
	├── v2xcast_traffic_signal_detector_us
	├── v2xcast_tsp
	└── v2xcast_txrx_us

	14 directories, 1 file

Al ejecutar ``Makefile`` estaremos compilando todos estos ficheros en la VM.

.. code-block:: console

	$ cd us_v2xcast_sdk/example
	$ make 
	Users shall select the platform at the first execution
	1. x86_64, 2. armv7_hf, 3. armv8_64, 4. vtx351, 5. vtx352 (type range: 1 ~ 5): 1

.. note::

	Por defecto, ``make`` guardara este valor en memoria para no tener que volver a ser introducido. En caso de querer recompilar estos fichero para otras arquitecturas se puede volver al estado inicial mediante ``$ make reset``.

Preparar el servicio V2Xcast en el modulo
"""""""""""""""""""""""""""""""""""""""""

Una vez tenemos los ficheros preparados, debemos configurar el modulo para la transmisión.

El intercambio de información entre el host (VM) y el módulo se lleva a cabo mediante paquetes ``IP`` sobre ``USB`` haciendo uso la tecnologia ``RNDIS/NCM``. Esta tecnologia es transparente para la gestión del módulo. No obstante, es útil entender porque las comunicaciones host-módulo se ejecutan mediante protocolos y herramientas de red.

.. admonition:: ¿Que es la tencología RNDIS/NCM?

	Por un lado, RNDIS (*Remote Network Drive Interface Specification*) permite que un dispositivo actue como una interfaz de red virtual a través de USB, creando una conexión similar a Ethernet. Por otro lado, NCM (*Network Control Model*) es un estándar más eficiente desarrollado por ``USB-IF`` para transferencias de datos en redes a través de USB, diseñado para maximizar la velocidad.

.. warning::

	Antes de configurar directamente el servicio V2Xcast, debemos comprobar que existe conectividad con el módulo. Para ello, simplemente verificamos que se puede llevar a cabo un ``ping``. El módulo, por defecto, el módulo se asigna a sí mismo la dirección ``192.168.1.3``.

Para crear el servicio V2Xcast, debemos subir un fichero ``.json`` con las características de la comunicación deseada. En el caso de este primer ejemplo, el fichero está ya preparado y se encuentra en ``us_v2xcast_sdk/bin/bsm_adhoc.json``. Por tanto, sólo debemos subirlo al módulo. Esto se puede realizar a través del programa ``v2xcast_manager`` proporcionado en el SDK. Este programa incorpora herramientas de red y facilita la gestión del módulo.

.. code-block:: console

	$ cd us_v2xcast_sdk/bin
	$ ./ v2xcast_manager -m post -f ./cv2x/bsm_adhoc.json 192.168.1.3/cfg/v2xcast
	Uploading US V2Xcast config successful

Transmición periódica
"""""""""""""""""""""

Para comenzar a realizar una transmisión periódica de mensaje BSM, tan sólo debemos ejecutar los correspondientes fichero ``.c`` que previamente hemos compilado. A estos, le debemos pasar como parámetros la dirección de comunicación del módulo (``192.168.1.3``), el rol que toma el módulo (``0`` para recibir, ``1`` para enviar) y un ``0``.

.. code-block:: console

	$ cd us_v2xcast_sdk/example/v2xcast_bsm
	$ ./v2xcast_bsm 192.168.1.3 1 0

Una vez el módulo está transmitiendo el resultado esperado es el siguiente:

.. code-block:: text

	connect to server 192.168.1.3:30001 with identity v2xcast_bsm@12-23 06:24:49
	-----------------------
	Index 0, token {
	      "id": "abcd",
	      "tractionControlStatus": 3,
	      "antiLockBrakeStatus": 3,
	      "stabilityControlStatus": 3,
	      "vehicleSafetyExtensions": {
	        "option": 1,
	        "rightTurnSignalOn": 1,
	        "leftTurnSignalOn": 0
	      },
	      "specialVehicleExtensions": {
	        "option": 1,
	        "lightBarInUse": 1,
	        "sirenInUse": 2
	      }
	    }
	BSM id: abcd, tractionControlStatus: 3, antiLockBrakeStatus: 3, stabilityControlStatus: 3, vehicleSafetyExtensions_option: 1, rightTurnSignalOn: 1, leftTurnSignalOn: 0, specialVehicleExtensions_option: 1,lightBarInUse: 1, sirenInUse: 2
	message count 1

	coreData.secMark: 50700
	coreData.long: 121.035425
	coreData.lat: 24.808811
	12-23 06:24:50.779  6254  6257 D J2735 UPER_ENCODE

	encode successfully
	BSM encoded data:

	00 14 33 40 58 58 98 D9 31 83 22 37 37 97 59 B7
	2E 26 08 00 00 00 00 00 00 00 00 00 7E 7D 07 D0
	7F 7F FF 17 E0 00 00 01 00 1D 64 00 20 F9 D9 02
	00 10 34 20 88 20

	Transmitted 54 bytes!
	-----------------------

.. warning::
	
	Los módulos deben tener conectadas tres antenas para que se de lugar a la comunicación. En especial, la antena que recoge información de la posición (GNSS), debe tener visión directa con los satelites del sistema. Por tanto, es fundamental que los dispositivos se encuentre en espacios abierto (fuera de la ventana de un edificio, en la calle, etc.).

.. note::
	
	Destacar que en esta transmisión las llamadas a la API del SDK que realizan los ficheros ``.c`` se ejecutan directamente sobre la VM de Linux y **NO** sobre el módulo.

Pasos a seguir en el módulo receptor
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Para el **modulo receptor**, haremos uso del siguiente material.

1. Un módulo Unec C-V2X/802.11p con su respectivo adaptador USB.
2. Una Raspberry Pi con Rapsbian y arquitectura CPU **armv7_hf**.

En la Raspberry Pi, colocamos los ficheros del entorno de desarrollo, ``BSM``, ``DOC``, ``LTS`` y ``SDK`` en un directorio deseado.

Los pasos a seguir son similares al caso del transmisor. En primer lugar debemos :ref:`descomprimir_sdk`. En segundo lugar, :ref:`compilar_ejemplos`. En este caso, estaremos compilando los fichero en la Rapsberry Pi en lugar de la VM. Además, en lugar de seleccionar la opción ``x86_64``, debemos selecionar ``3`` ya que hacemos uso de la arquitectura ``armv7_hf``.

Sin embargo, la forma en la que preparamos el servicio V2Xcast en el módulo es diferente (por ser un sistema con arquitectura *armv7_hf*).

Preparar el servicio V2Xcast en el módulo
"""""""""""""""""""""""""""""""""""""""""

Para subir el fichero ``bsm_adhoc.json`` y configurar el servicio (las características de la comunicación V2X) haremos uso de SCP (*Secure Copy Protocol*). Dado que el módulo tiene asignada la dirección ``192.168.1.3``, podemos copiar de forma recursviva toda el contenido bajo ``us_v2xcast_sdk`` en el módulo.

.. code-block:: console
	
	$ cd us_v2xcast_sdk/
	$ scp -r us_v2xcast_sdk root@192.168.1.3:/home/unex

Posteriormente, nos conectaremos al módulo a través de SSH.

.. code-block:: console

	$ ssh root@192.168.1.3
	$ cd /home/unex/us_v2xcast_sdk

Y ahora, desde el propio modulo, subiremos el fichero ``bsm_adhoc.json`` a través del propio gestor del SDK.

.. code-block:: console

	$ ./v2xcast_manager -m post -f ./cv2x/bsd_adhoc.sjon
	Uploading US V2Xcast config successful

Recepción
"""""""""

Continuando el la consola del módulo mediante SSH, para preparar el módulo para la transmisión, tan sólo debemos ejecutar los ficheros ``.c`` compilados previamente para ello.

.. code-block::console

	cd /home/unex/us_v2xcast_sdk/example/v2xcast_bsm
	./v2xcast_bsm 127.0.0.1 0 0

El resultado, al recebir algun paquete, se espera que sea el siguiente:

.. code-block:: text

	connect to server 127.0.0.1:30002
	-----------------------
	Received 54 bytes!
	BSM decoding data:

	00 14 33 40 18 58 98 D9 09 DC E2 37 51 D8 D9 B7
	36 FE 08 00 00 00 00 00 00 00 00 00 7E 7D 07 D0
	7F 7F FF 17 E0 00 00 01 00 1D 64 00 20 F9 D9 02
	00 10 34 20 88 20

	Decoded BSM
	  coreData.msgCnt: 1
	  coreData.id: 61626364
	  secMark: 10099
	  transmission: 0
	  partII[0]:
	    partII_Id: 0
	     case VehicleSafetyExt: 3
	  partII[1]:
	    partII_Id: 1
	-----------------------

.. warning::
	
	Los módulos deben tener conectadas tres antenas para que se de lugar a la comunicación. En especial, la antena que recoge información de la posición (GNSS), debe tener visión directa con los satelites del sistema. Por tanto, es fundamental que los dispositivos se encuentre en espacios abierto (fuera de la ventana de un edificio, en la calle, etc.).

.. note::
	
	Destacar que en esta recepción las llamadas a la API del SDK que realizan los ficheros ``.c`` se ejecutan sobre el módulo y **NO** sobre la Raspeberry Pi. Esto puede llegar a suponer un obstáculo ya que las capacidad del módulo es limidada (se trata de un sistema embebido).

Comunicación personalizada
--------------------------