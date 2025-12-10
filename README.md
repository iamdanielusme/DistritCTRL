 # DistritCTRL
Este repositorio esta hecho para llevar seguimiento al proyecto diseñado para el curso Electrónica Digital III

El proyecto final del curso de Electrónica Digital III consiste en desarrollar una aplicación o sistema embebido que utilice el MCU del curso y que involucre el uso de sensores, actuadores, Interfaz Humano Máquina, módulos de comunicación, etc. Los estudiantes deberán conformar grupos de trabajo para la realización del proyecto. Si bien no se establece un máximo de estudiantes, la complejidad del proyecto y el número de integrantes del grupo debe ser coherente. Cada grupo definirá los requisitos y características de la aplicación o sistema que diseñará. Este cuestionario recoge información básica necesaria para comprender la propuesta inicial de trabajo que cada grupo quiere realizar.

1.	Por favor ingrese un nombre para su proyecto. El nombre debe ser un nombre corto como el que tendría un producto comercial. No utilice nombres largos que parezcan el título de una tesis. 
Distrit CTRL01

2.	Describa de la forma detallada la idea inicial de proyecto final de su equipo de trabajo.
La idea de nuestro proyecto es diseñar y construir un dispositivo híbrido inspirado en la caja de ritmos Roland TR-808 y en los controladores modulares de Yaeltex, implementado con dos microcontroladores Raspberry Pi Pico trabajando de manera conjunta.
El sistema busca integrar las siguientes funcionalidades:
•	Secuenciador rítmico: similar al de la TR-808, permitiendo programar patrones de percusión en pasos.
•	Interfaz de control físico: con faders dedicados para cada canal, además de botones para control de patrones y activación de instrumentos.
•	Sensores interactivos: se incorporará un sensor tipo theremin (por ultrasonido o capacitivo) para controlar parámetros expresivos en tiempo real.
•	Módulo de efectos: implementación de efectos como delay y reverb, además de otros botones para control adicional (mute, solo, etc.).
•	Compatibilidad con MIDI: el sistema enviará y recibirá datos MIDI (USB), permitiendo sincronización y control en software como Ableton Live.
Para gestionar esta complejidad, se emplearán dos Raspberry Pi Pico, distribuyendo las funciones de la siguiente forma:
•	Pico #1 (Master): encargado del motor principal del secuenciador, la generación de triggers, la comunicación MIDI y la salida de datos hacia Ableton.
•	Pico #2 (Slave): dedicado a la lectura de controles físicos (faders, potenciómetros, botones) y sensores (theremin), enviando esta información en tiempo real al Pico W principal.
La comunicación entre ambas se podrá realizar mediante protocolos como UART o I2C, garantizando una transferencia eficiente de datos.
De esta manera, el sistema final se concibe como un instrumento modular, expandible y adaptable, combinando elementos clásicos de la síntesis rítmica con nuevas formas de interacción musical.

3.	Describa brevemente cual es la motivación que los llevo a proponer la tematica de su proyecto de aula.
La motivación para proponer este proyecto surge porque uno de los integrantes del equipo es productor musical y ha identificado la ausencia de sistemas de este tipo fabricados en Colombia. Medellín, reconocida como una de las ciudades con mayor crecimiento en la escena de la música electrónica en Latinoamérica, representa un entorno ideal para este desarrollo. Por eso, consideramos importante que un proyecto así se realice en la Universidad de Antioquia, uniendo la formación en ingeniería con la fuerza creativa de la industria musical local.
