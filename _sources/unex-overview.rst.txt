Descripción general
===================

El **Unex SOM-352** es un módulo de comunicación V2X que integra el chipset **Craton2**. Este módulo está diseñado para facilitar la implementación de funciones V2X en vehículos y aplicaciones de transporte inteligente. Al incorporar el Craton2, el SOM-352 ofrece compatibilidad con las tecnologías **DSRC** y **C-V2X**, permitiendo la comunicación directa entre vehículos y entre vehículos e infraestructuras.

El punto fuerte de estos módulos Unex es la incorporación del kit **V2Xcast SDK**. V2Xcast SDK permite interactuar con el módulo Unex V2X sin necesidad de desarrollar directamente sobre él. En lugar de desarrollar programas que implementen las pilas de protocolos y estándares V2X desde cero, permite utilizar una API (**V2Xcast Services**) que abstrae estos detalles y facilita la integración en sus aplicaciones. Esto permite trabajar a un nivel más alto, sin necesidad de gestionar directamente las capas inferiores de comunicación.

Mediante **V2Xcast** se pueden configurar las pilas de protocolos a utilizar, invocar la API correspondiente para crear/liberar *Casters* y utilizar estos *Casters* para enviar/recibir mensajes de forma sencilla.

.. admonition:: Dos formas de usar los módules Unex 352

  Se pueden desarrollar programas orientados directamente a trabajar con el chipset **Craton2**, para lo cual habrá que compilar los programas con un compilador específico. O, tambień, se pueden desarrollar programas para otras arquitecturas más frecuentes como ``x86-64`` (Ubuntu, por ejemplo) o ``armv7a`` (Raspberry Pi OS, por ejemplo). Estas últimas deberán interacturar con el módulo a través del servicio V2Xcast.

El funcionamiento de **V2Xcast Service** se puede ver en la siguiente figura.

.. figure:: images/system_architecture_external_use.svg
   :alt: Descripción de la imagen
   :width: 50%
   :align: center

   Arquitectura del sistema desde el punto de vista externo

De niveles superiores a inferiores, las aplicaciones que requiren comunicaciones V2X pueden hacer uso del kit de desarrollo **V2Xcast SDK**. Dicho kit realiza llamadas a los **Servicios V2Xcast**. Como existen diferentes normativas en función de la región en la que se desea desplegar el proyecto, estas llamadas harán uso bien de los servicios US (United States) o bien de los servicios EU (European Union).

.. note::
	
	En este proyecto, como se quieren explorar los estándares IEEE, se hará uso de los *US Services*. Más adelante se muestran las diferencias en los estándares implementados.

En cualquier caso, estos servicios integran funcionalidades tanto de manejo de mensajes, como de seguridad, codificación, red, transporte, etc. Los servicios se apoyan en el PAL (*Platform Abstraction Layer*) que se encarga de proporcionar una **interfaz uniforme** entre los **servicios** y el de **hardware** subyacentes. En este caso, PAL gestiona la interacción entre los servicios de V2Xcast y componentes del sistema como: V2X, C-V2X, Criptografía (Seguridad), HSM (Hardware Security Module, para almacenamiento y procesamiento de claves seguras) o GNSS (para información de posicionamiento).

El paquete de US trae consigo un **conjunto de ficheros**, entre los que se encuentra el SDK y la documentación oficial. 

.. code-block:: text

    v200/v2x-files/
    ├── BSP
    ├── DOC
    ├── LTS
    └── SDK
    4 directories, 0 files

.. note::

  En la carpeta de documentación (DOC) podemos encontrar la documentación de instalación en formato PDF y la documentación oficial en formato ``html``.

En concreto, el SDK trae:

1. ``${PRODUCT_NAME}-remote_upgrade-${SDK_VERSION}.sh``
	- Un **script** para **actualizar** la pila de protocolos US de forma remota.
2. ``us_protocol_${dsrc/cv2x}-craton2-${SDK_VERSION}.tgz``
	- **US SDK** para la plataforma **Craton2**.
3. ``us_v2xcast_sdk-armv7a_32_linux-${SDK_VERSION}.tgz``
	- **US V2Xcast** para las plataformas **ARMv7a**.
4. ``us_v2xcast_sdk-armv8a_64_linux-${SDK_VERSION}.tgz``
	- **US V2xcast** para las plataformas **ARMv8a**.
5. ``us_v2xcast_sdk-x86_64_linux-${SDK_VERSION}.tgz``
	- **US V2Xcast** para las plataformas **X86-64**.
6. ``poky-${TOOLCHAIN_TYPE}-toolchain-${TOOLCHAIN_VERSION}.sh``
	- Un **script** para **instalar y configurar** el *toolchain* de desarrollo necesario para compilar aplicaciones en plataformas específicas. El *toolchain* es un conjunto de herramientas que incluyen compiladores cruzados y otros recursos.

.. note::

	Los tipos ``armv7a``, ``armv8a`` y ``x86-64`` hacen referencia los tres tipo de arquitecturas más comunes en equipos de usuario (PCs, por ejemplo). En equipos GNU/Linux, ejecutando el comando ``uname -u`` podemos saber cual es la arquitectura del equipo. A lo largo del proyecto es importante saber donde se compilan y ejecutan los distintos programas y asegurarse de que estan orientados al tipo de arquitectura deseado.

.. caution::

	En lo que respecta a la instalación física de los módulos, existe una guía de instalación dentro del paquete. A modo de resumen, los modulos se montan sobre una adaptador a interfaz USB-A que se conecta directamente a cualquier equipo. Los módulos poseen tres puntos para la **colocación de antenas**, dos elevados superiormente para poner antenas de **recepción y transmisión de mensajes** generales y otra para el **servicio GNSS de posicionamiento**. Esta última debe colocarse fuera de cualquier **recinto cerrado** (fuera de cualquier edificio).

Por último, es importante mencionar que Unex implementa funciones V2X basadas en la capa de protocolo internacionales y versiones de estándares relacionados. En el caso de los servicios US, implementa los siguientes protocolos y estándares.

.. list-table:: 
   :header-rows: 1
   :widths: 25 50 25

   * - Protocol Layer
     - Standard name
     - Version
   * - Message Layer
     - SAE J2735 V2X Communications Message Set Dictionary
     - 202007
   * - Network & Transport Layer
     - IEEE Std 1609.3
     - 2020 (partial)
   * - Access Layer
     - IEEE Std 1609.4
     - 2016
   * - Security Layer
     - IEEE Std 1609.2
     - 2016
   * - 
     - IEEE Std 1609.2a
     - 2017
   * - 
     - IEEE Std 1609.2b
     - 2019
   * - 
     - IEEE Std 1609.2.1
     - 2022



Arquitectura
------------

En la siguiente figura se pueden observar las partes de las que se compone la solución software de los módulos Unex. Dicha arquitectura se compone de: **V2X facility modules**, **V2X network and transport modules**, **V2X security modules**, **V2Xcast service modules** y **otros módulos**.

.. figure:: images/system_architecture.svg
   :alt: Descripción de la imagen
   :width: 70%
   :align: center

   Arquitectura de la solución software de los módulos Unex

Conexión con el módulo
----------------------

Configuración de la antena GNSS
-------------------------------

Capturador de paquetes
----------------------

Controlador de los recursos radio
---------------------------------