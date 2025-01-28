Descripción general
===================

Una de los puntos fuertes de estos módulos Unex es la incorporación del kit **V2Xcast SDK**. Mientras que la mecánica habitual con este tipo de implementaciones es que los desarrolladores estudien los estándares y pilas de protocolos relacionados (como las normas IEEE 1609.3 y 1609.4), así como las restricciones necesarias para los niveles inferiores, y, a partir de esa información, integren la API en sus proyectos de forma que el sistema funcione correctamente, V2Xcast ofrece una alternativa de trabajo.

Mediante **V2Xcast** se puede configuar las pilas de protocolos a utilizar, invocar la API para crear/liberar *Casters* y utilizar estos *Casters* para enviar/recibir mensajes. El funcionamiento se puede ver en la siguiente figura.

FALTA LA FIGURA.

De niveles superiores a inferiores, los aplicaciones que requiren comunicaciones V2X pueden hacer uso del kit de desarrollo **V2Xcast SDK**. Dicho kit realiza llamadas a los **Servicios V2Xcast**. Como existen diferentes normativas en función de la región en la que se desea desplegar el proyecto, estas llamadas harán uso bien de los servicios US (United States) o bien de los servicios EU (European Union).

.. note::
	
	En este proyecto, como se quieren explorar los estándares IEEE, se hará uso de los *US Services*.

En cualquier caso, estos servicios integran funcionalidades tanto de manejo de mensajes, como de seguridad, codificación, red, transporte, etc. Los servicios se apoyan en el PAL (*Platform Abstraction Layer*) que se encarga de proporcionar una **interfaz uniforme** entre los **servicios** y el de **hardware** subyacentes. En este caso, PAL gestiona la interacción entre los servicios de V2Xcast y componentes del sistema como: V2X, C-V2X, Criptografía (Seguridad), HSM (Hardware Security Module, para almacenamiento y procesamiento de claves seguras) o GNSS (para información de posicionamiento).

Arquitectura
------------

Conexión con el módulo
----------------------

Configuración de la antena GNSS
-------------------------------

Capturador de paquetes
----------------------

Controlador de los recursos radio
---------------------------------