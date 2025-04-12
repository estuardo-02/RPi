"""
Universidad Del Valle de Guatemala 1
Facultad de Ingeniería
Departamento de Ingeniería Electrónica, Mecatrónica y Biomédica
IE3059 – Electrónica Digital 3, Primer Ciclo, 2025

Estuardo Castillo 21559

Programa para 'captura' de imagenes via Cam Module 3 conectado a RPi
el siguiente programa sirve como una prueba de concepto para captura y envío de imágenes, así
como la interacción del programa con periféricos de RPi 4. El programa interactúa directamente
con programa procesador_imagen.py

Hardware necesario: LED, resistencia de ~330 ohms, push-button
captura_camara.py
"""
import threading
import os
import time
import RPi.GPIO as GPIO

# GPIO setup
BUTTON_GPIO = 11
LED_GPIO = 13  # Define el pin GPIO para el LED

# Configuración de GPIO
GPIO.setmode(GPIO.BOARD)
GPIO.setup(BUTTON_GPIO, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.setup(LED_GPIO, GPIO.OUT)  # Configurar LED como salida

# El programa productor es responsable de crear el pipe.
PIPE_PATH = "/tmp/image_pipe"
if not os.path.exists(PIPE_PATH):
    os.mkfifo(PIPE_PATH)

# Se simula la imagen que capturaría la RPi
#SIMULATED_IMAGE_PATH = "/home/pi/Documents/tato/proyecto1/test_image.jpg"
image_file_name = "test_image.jpg"  
current_directory = os.path.dirname(os.path.abspath(__file__))  
# Arreglado para ggenerar path relativo a donde se descargue archivos
SIMULATED_IMAGE_PATH = os.path.join(current_directory, image_file_name)

# Función para simular la captura de imágenes 
def simulate_capture_image():
    print("[EMISOR] Iniciando captura")
    #Se emplea un arreglo para mantener control de los subprocesos, dado que es posible
    #que se creen múltiples procesos al mismo tiempo
    threads = [] 
    while True:
        if GPIO.input(BUTTON_GPIO) == GPIO.HIGH:  # Button pressed
            time.sleep(0.2)  # Debounce
            # Use a simulated image instead of capturing
            image_path = SIMULATED_IMAGE_PATH
            thread = threading.Thread(target=send_image, args=(image_path,))
            threads.append(thread)
            thread.start()
            #Se emplea un mecanismo para el hilo principal espere hasta que termine hilo
            #mediante join(). Esto se implementa porque al entrar diversos hilos pueden
            #corromper la escritura de la ruta (que se envía por pipe)
            #Puede emplearse algún otro mecanismo de sync para recursos compartidos como 
            #semaphores, mutexes (explorado en el otro archivo)
            for t in threads:
                t.join() 

# Envio
def send_image(image_path):
    with open(PIPE_PATH, 'w') as pipe:
        pipe.write(image_path)
        print(f"[EMISOR] Imagen enviada: {image_path}")
        # Encender el LED después de enviar la imagen
        GPIO.output(LED_GPIO, GPIO.HIGH)  # Prender LED
        time.sleep(0.1)  # Mantener el LED encendido por 1 segundo
        GPIO.output(LED_GPIO, GPIO.LOW)  # Apagar LED

if __name__ == "__main__":
    print("[EMISOR] Iniciando programa capturador de imágenes")
    try:
        simulate_capture_image()
    except KeyboardInterrupt:
        print("\n[EMISOR] Finalizando programa...")
    finally:
        GPIO.cleanup()  # Limpiar configuración GPIO al salir
