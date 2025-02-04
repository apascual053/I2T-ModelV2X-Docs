B.A.T.M.A.N.
============

¿Qué es B.A.T.M.A.N.?
----------------------

B.A.T.M.A.N. (*Better Approach To Mobile Ad-hoc Networking*) es un protocolo de enrutamiento diseñado para redes ad hoc móviles de múltiples saltos. Fue desarrollado por la comunidad *Freifunk* con el objetivo de mejorar el rendimiento en redes de malla a gran escala, en comparación con protocolos como OLSR (*Optimized Link State Routing Protocol*).

La principal característica de B.A.T.M.A.N. es su enfoque descentralizado para el enrutamiento: en lugar de que un nodo tenga una visión global de la red, cada nodo solo conoce la dirección desde la que recibe información y la reenvía en consecuencia. Esto permite un encaminamiento dinámico y adaptable, especialmente en entornos de red cambiantes.

El protocolo tiene dos implementaciones principales:

- **B.A.T.M.A.N. clásico**: Funciona en la capa 3 (red) y manipula las tablas de enrutamiento del sistema.
- **B.A.T.M.A.N. avanzado (batman-adv)**: Opera en la capa 2 (enlace de datos), permitiendo que la red funcione como un solo dominio de difusión (*broadcast*).

.. note::

   En nuestro caso, para desplegar la red V2X, vamos a hacer uso de *batman-adv*.