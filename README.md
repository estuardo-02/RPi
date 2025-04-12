<h1 align="center">Repositorio Proyectos Raspberry Pi</h1>

<h3 align="center"> Estuardo Castillo | cas21559@uvg.edu.gt</h3>


## Resumen

El siguiente repositorio apunta a recolectar variedad de proyectos para la Raspberry Pi. En gran parte se tratan de pruebas de concepto empleando librerías básicas
en C y python para demostrar conceptos en RTOS

# Proyecto 1

Clonar repositorio. 
Nota: esto clona el repositorio a la ubicación dónde se esté trabajando. Considerar copiar en alguna ruta conocida. 
## Linux
```bash
    #Considerar ubicación de archivo. Ejemplo: directorio para proyectos 
    #~ $ mkdir proyectos
    #~ $ cd proyectos
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
### parte 2 uso
Asumiendo que esté en carpeta de parte1. Considerar usar dos terminales para poder interrumpir programas con Ctrl+C o empujar a background
```bash
    #~ $ ../RPi/digital3/proyecto1/parte1/ cd ..
    cd parte2/
    python3 captura_camara.py &
    #En otra terminal, misma ubicación:
    python3 procesador_imagen.py
```
