# Proyecto Electrónica Digital 3
# Sistema SCADA 
Gabriel Carrera 21219
Estuardo Castillo 21559
## 1. Compilación de los programas

Este proyecto consta de cuatro programas distintos. A continuación se describe cómo compilar cada uno de ellos.

### 1.1. Historiador

Compila el historiador con:

```bash
gcc historiador.c -o historiador -lpthread
```

> **Nota:** Es importante incluir la librería de pthread ya que se utilizan hilos en el programa.

---

### 1.2. RTU1

RTU1 utiliza la librería de servo (instalada automáticamente con wiringPi). Al compilar, debes llamar a la ruta donde se encuentra dicha librería:

```bash
gcc rtu1.c -o rtu1 /ruta/a/softServo.c -lpthread -lwiringPi
```

**Ejemplo:**

```bash
gcc rtu1.c -o rtu1 /home/usuario/WiringPi/wiringPi/softServo.c -lpthread -lwiringPi
```

> Este programa también utiliza la librería de pthread y la de wiringPi para manejar los GPIOs.

---

### 1.3. RTU2

RTU2 es similar a RTU1, pero no utiliza el servo, así que no es necesaria la librería softServo:

```bash
gcc rtu2.c -o rtu2 -lpthread -lwiringPi
```

---

### 1.4. ESP32 Webserver

Este programa se utiliza para la función IoT y se compila desde el **Arduino IDE**.  
Es necesario tener instalado el paquete de placa del ESP32.

1. En **Preferencias** del Arduino IDE, agrega la siguiente URL en "Additional boards manager URLs":

    ```
    https://espressif.github.io/arduino-esp32/package_esp32_index.json
    ```

2. En **Board Manager**, busca `esp32` e instala el paquete **esp32 by Espressif**.

3. Cambia la SSID, contraseña, puerto local y las IP de ambas RTUs en el código fuente antes de compilar.

    ```
    const char* ssid = "NETGEAR_iemtbmUVG"; //nombre de la red
    const char* password = "iemtbmUVG"; //Contraseña
    
    const char * udpAddress = "10.0.0.11"; //IP address del RTU1
    const char * udpAddress2 = "10.0.0.15"; //IP address del RTU2
    
    const int udpPort = 2006; //Puerto local
    ```
---

## 2. Conexiones

Las conexiones se realizan con cada una de las Raspberry Pi.  
En ambos códigos de las RTU hay definiciones en la parte superior para ver en qué pin se encuentra cada cosa.  
**Importante:** Los pines definidos utilizan el pinout de wiringPi.

El ESP32 únicamente necesita estar encendido, ya que se comunica mediante UDP con las RTUs y no requiere conexiones externas.

---

## 3. Ejecución de los programas

> **IMPORTANTE:**  
> Para correr el sistema, primero ejecuta las dos RTUs y después el historiador.  
> El ESP32 puede funcionar independientemente del momento en que se ejecute.  
> Todos los dispositivos deben estar en la misma red para que el sistema funcione correctamente.

### 3.1. RTU1 y RTU2

Las RTUs se ejecutan de la siguiente manera (el puerto debe ser 2000 o superior):

```bash
./rtu1 <numero-de-puerto>
# Ejemplo:
./rtu1 2000
```

### 3.2. Historiador

El historiador se ejecuta cuando ambos RTUs ya están corriendo.  
Requiere 3 argumentos: IP del RTU1, IP del RTU2 y el puerto local.

```bash
./historiador <IP-RTU1> <IP-RTU2> <numero-de-puerto>
# Ejemplo:
./historiador 10.0.0.6 10.0.0.20 2000
```

### 3.3. ESP32 Webserver

Este programa se puede ejecutar en cualquier momento haciendo click en **Upload** en el Arduino IDE.  
Debe estar conectado a la computadora y en la misma red que los demás dispositivos.

---

## 4. Acceso al Webserver

Para utilizar las aplicaciones IoT, necesitas la IP del webserver.  
Esta IP la imprime el ESP32 después de ejecutar el programa en la terminal del Arduino IDE.

Accede desde un navegador en un dispositivo que también esté en la misma red.
La impresión en consola deberá verse así:
```bash
WiFi connected! IP address: <IP en red>
```
---

Con esto ya tienes todo lo necesario para utilizar el sistema y que funcione correctamente.
