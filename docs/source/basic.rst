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

.. admonition:: **Objetivos**

   Poner en marcha una primera comunicación entre dos módulos V2X, creando una pequeña red celular C-V2X de tipo V2V. Para ello, intercambiar mensajes BSM.

Para llevar a cabo este objetivo se hara uso del SDK, que proporciona formatos de mensaje previamente configurados y ejemplos. Esto permite abstraerse en este ejercicio del contenido de la informacíón intercambiada.

.. note::

	En la documentación de Unex, en el apartado *4. Application Note* del capítulo *V2Xcast*, se describe qué tipos de mensajes se estandarizan tanto en EU (Unión Europea) como en US (United States). No obstante, no es de demasiado interés ya que el objetivo de este este proyecto se basa en una comunicación personalizada.

.. tip::

	La documentación general viene en formato ``html`` y se puede encontrar en ``v200/v2x-files/DOC/index.html``.

	.. code-block:: bash

		$ find v200/v2x-files/ -name "index.html"
		./DOC/index.html

A continuación, se redactan las instrucciones de la configuración tanto del módulo transmisor como del módulo receptor.

.. note::

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

Dentro de la VM de Ubuntu, colocamos los ficheros del entorno de desarrollo, ```BSM``, ```DOC``, ``LTS`` y ```SDK`` en un directorio deseado.

Dentro de la carpeta ``SDK``, encontramos varios fichero ``.tar``. Cada uno de ellos corresponde con un tipo de arquitectura de CPU. Para ello, se deberá extraer el fichero correspondiente a cada caso. Al descomprimir, tendremos las herramientas proporcionadas por el SDK disponibles.

.. code-block:: bash

	$ cd SDK/
	$ tar zxvf us_v2xcast_sdk-x86_64_linux-v2.0.1.tgz