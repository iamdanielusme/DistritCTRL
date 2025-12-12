# Distrit CTRL01 – Controlador MIDI híbrido (Proyecto Electrónica Digital III)

Este repositorio está hecho para llevar el seguimiento del proyecto diseñado para el curso **Electrónica Digital III** de la Universidad de Antioquia.

El proyecto final del curso consiste en desarrollar una aplicación o sistema embebido que utilice el MCU visto en clase e integre **sensores, actuadores, interfaz humano–máquina y módulos de comunicación**. En nuestro caso, el equipo propuso como proyecto **Distrit CTRL01**, un controlador MIDI híbrido orientado a performance musical en vivo.

---

## Descripción general del proyecto

La idea de nuestro proyecto es diseñar y construir un dispositivo híbrido inspirado en la **caja de ritmos Roland TR-808** y en los **controladores modulares de Yaeltex**, implementado con **dos microcontroladores Raspberry Pi Pico** trabajando de manera conjunta (Pico W como *master* y Pico como *slave*).

## Arquitectura general

El sistema se divide en dos placas:

### Master (Pico W)
- 3 **faders analógicos** en ADC internos (GP26, GP27, GP28).
- 2 **sensores ultrasónicos HC-SR04** para controlar parámetros tipo theremin.
- **Anillo de LEDs WS2812** controlado por PIO.
- **Pantalla OLED SH1106** por I2C para mostrar:
  - BPM detectado desde el clock MIDI.
  - Estado de reproducción del secuenciador.
  - Step actual y total de pasos.
- **Módulo de step sequencer** que se sincroniza con el clock MIDI.
- **Interfaz MIDI USB** (TinyUSB) hacia el computador/sintetizador.

### Slave (Pico)
- 4 **botones arcade** (para notas o triggers rítmicos).
- 4 **botones pulsadores** (funciones secundarias / notas adicionales).
- 4 **potenciómetros** leídos a través de un **ADC externo ADS1115** por I2C.
- Enlace **UART** hacia el master con un protocolo simple:
  - Máscara de botones arcade.
  - Máscara de botones normales.
  - Valores analógicos de los 4 potenciómetros.
- LED onboard usado como indicador de actividad y debug.

---

## Software y módulos principales

El proyecto está estructurado en varios módulos en C para mantener el diseño legible y reutilizable:

- `midi_core`: inicializa TinyUSB y se encarga de enviar **Note On/Off** y **Control Change** al host.
- `slave_link`: maneja la comunicación UART con el slave, parseando los frames y exponiendo el estado como una estructura `slave_state_t`.
- `button_driver` (en el slave): lee botones con **antirrebote** y genera máscaras para arcade y normales.
- `pot_driver` (en el slave): configura el **ADS1115** y actualiza periódicamente las lecturas de 4 canales analógicos.
- Lógica en el master para **mapear**:
  - Botones arcade → notas MIDI (ej. C4, C#4, D4, D#4).
  - Botones normales → notas MIDI adicionales.
  - Pots del slave → CC (ej. 20–23).
  - Faders del master → CC (ej. 10–12).
  - Sensores ultrasónicos → CC (ej. 30–31), usando un rango de distancias 10–80 cm.
- `display_oled`: muestra BPM, estado de reproducción y step actual.
- `led_ring`: actualiza el anillo de LEDs con información del step sequencer o estados del controlador.
- `ultra_driver`: mide distancia con los HC-SR04 usando una máquina de estados no bloqueante.

Todo el flujo se realiza de forma **no bloqueante**, coordinando:
- `midi_core_task()` para la pila USB/MIDI.
- `slave_link_task()` para actualizar el estado del slave.
- `ultra_driver_update()` para avanzar las mediciones de los sensores.
- Un timer periódico para refrescar la UI en la OLED.

---

## Uso esperado

1. Conectar el **Pico W (master)** por USB al computador.
2. Seleccionar el dispositivo MIDI generado por la Pico en el DAW (Ableton Live u otro).
3. Mover faders, girar potenciómetros y acercar/alejar la mano a los sensores ultrasónicos para modular parámetros (volumen, filtros, efectos, etc.).
4. Usar los botones arcade y normales para disparar notas, clips o funciones de transporte, dependiendo del mapeo en el DAW.
5. Observar feedback visual en:
   - Pantalla OLED (BPM, step, estado).
   - Anillo de LEDs (pasos del secuenciador / estado del controlador).
   - LED de debug para indicar actividad de botones.

---

## Estado actual del proyecto

- ✅ Comunicación **master–slave** por UART estable.
- ✅ Lectura de **botones** y mapeo a notas MIDI.
- ✅ Lectura de **pots** del slave mediante ADS1115 y envío como CC.
- ✅ Lectura de **3 faders** en el master (ADC interno) y envío como CC.
- ✅ Lectura de **2 sensores ultrasónicos** y mapeo a CC tipo theremin.
- ✅ Integración con **TinyUSB MIDI**, OLED y anillo de LEDs.
- 🔧 Pendiente afinar algunos parámetros de filtrado, rangos y mapeos creativos según el uso musical final.

Este repositorio recoge todo el código fuente, la lógica de comunicación y los módulos de hardware necesarios para que Distrit CTRL01 funcione como un controlador MIDI híbrido, expandible y orientado a performance en vivo.

## Motivación

La motivación para proponer este proyecto surge porque uno de los integrantes del equipo es **productor musical** y ha identificado la ausencia de controladores de este tipo fabricados en Colombia. Medellín, reconocida como una de las ciudades con mayor crecimiento en la escena de la música electrónica en Latinoamérica, es un entorno ideal para explorar este tipo de desarrollos.

Con **Distrit CTRL01** buscamos:

- Unir la formación en **ingeniería electrónica** con la **industria musical local**.
- Diseñar un dispositivo que pueda usarse en **performances en vivo** y procesos creativos reales.
- Demostrar cómo los contenidos de Electrónica Digital III (MCU, comunicaciones, sensores y HMI) pueden materializarse en un **instrumento musical funcional**, diseñado desde la universidad.
