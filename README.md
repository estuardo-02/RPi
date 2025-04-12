<h1 align="center">Repositorio Proyectos Raspberry Pi</h1>

<h3 align="center"> Estuardo Castillo | cas21559@uvg.edu.gt</h3>


## Resumen

El siguiente repositorio apunta a recolectar variedad de proyectos para la Raspberry Pi. En gran parte se tratan de pruebas de concepto empleando librerías básicas
en C y python para demostrar conceptos en RTOS

# Proyecto 1

Clonar repositorio y colocarse en ubicación de archivos
## Linux
```bash
    git clone https://github.com/estuardo-02/RPi.git
```
### parte 1 compilar archivos
```bash
    cd RPi/digital3/proyecto1/parte1/
    gcc productor.c -o productor
    gcc consumidor.c -o consumidor -lpthread
```
### parte 1 uso
Emplear dos terminales. Ejecutar ./productor primero
```bash
    ./productor [#mensajes] [#productores]
    ./consumidor
```
