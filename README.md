# Distrit CTRL01 – Controlador MIDI híbrido (Proyecto Electrónica Digital III)

Este repositorio está hecho para llevar el seguimiento del proyecto diseñado para el curso **Electrónica Digital III** de la Universidad de Antioquia.

El proyecto final del curso consiste en desarrollar una aplicación o sistema embebido que utilice el MCU visto en clase e integre **sensores, actuadores, interfaz humano–máquina y módulos de comunicación**. En nuestro caso, el equipo propuso como proyecto **Distrit CTRL01**, un controlador MIDI híbrido orientado a performance musical en vivo.

---

## Descripción general del proyecto

La idea de nuestro proyecto es diseñar y construir un dispositivo híbrido inspirado en la **caja de ritmos Roland TR-808** y en los **controladores modulares de Yaeltex**, implementado con **dos microcontroladores Raspberry Pi Pico** trabajando de manera conjunta (Pico W como *master* y Pico como *slave*).

El sistema busca integrar las siguientes funcionalidades:

- **Secuenciador rítmico**: similar al de la TR-808, permitiendo programar patrones de percusión por pasos.
- **Interfaz de control físico**: faders dedicados para cada canal, potenciómetros, y botones para control de patrones y activación de instrumentos.
- **Sensores interactivos**: uso de sensores tipo *theremin* (ultrasonido) para controlar parámetros expresivos en tiempo real.
- **Módulo de control de efectos**: pensado para manejar parámetros como delay, reverb u otros efectos desde el hardware.
- **Compatibilidad MIDI (USB)**: el sistema envía datos MIDI hacia software como Ableton Live, permitiendo sincronización, control de instrumentos virtuales y automatización de parámetros.

Para gestionar la complejidad del sistema se utilizan dos MCUs con funciones distribuidas:

- **Pico W (Master)**  
  - Motor principal del secuenciador.  
  - Generación de triggers.  
  - Comunicación MIDI USB con el computador/DAW.  
  - Manejo de la interfaz visual (anillo de LEDs y pantalla OLED).  

- **Pico (Slave)**  
  - Lectura de controles físicos: faders, potenciómetros y botones (arcade y normales).  
  - Lectura de sensores (ultrasonido tipo theremin).  
  - Envío de toda esta información al master en tiempo real mediante un enlace serie (UART).

De esta manera, **Distrit CTRL01** se concibe como un instrumento **modular, expandible y adaptable**, que combina elementos clásicos de la síntesis rítmica con nuevas formas de interacción musical basadas en sensores.

---

## Motivación

La motivación para proponer este proyecto surge porque uno de los integrantes del equipo es **productor musical** y ha identificado la ausencia de controladores de este tipo fabricados en Colombia. Medellín, reconocida como una de las ciudades con mayor crecimiento en la escena de la música electrónica en Latinoamérica, es un entorno ideal para explorar este tipo de desarrollos.

Con **Distrit CTRL01** buscamos:

- Unir la formación en **ingeniería electrónica** con la **industria musical local**.
- Diseñar un dispositivo que pueda usarse en **performances en vivo** y procesos creativos reales.
- Demostrar cómo los contenidos de Electrónica Digital III (MCU, comunicaciones, sensores y HMI) pueden materializarse en un **instrumento musical funcional**, diseñado desde la universidad.
