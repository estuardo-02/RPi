"""
Universidad Del Valle de Guatemala 1
Facultad de Ingeniería
Departamento de Ingeniería Electrónica, Mecatrónica y Biomédica
IE3059 – Electrónica Digital 3, Primer Ciclo, 2025

Estuardo Castillo 21559

Programa para 'recepción' de imagenes en un pipe nombrado y transformación sencilla
en este caso se implementa la librería pillow para la transformación a formato de blanco y negro. 

captura_camara.py
"""
import os
import time
import threading
from PIL import Image

#Se instruye la cración del named pipe en la carpeta destinada a archivos temporales, puede
#ser en la misma carpeta si se omite la instrucción. 
PIPE_PATH = "/tmp/image_pipe"
if not os.path.exists(PIPE_PATH):
    print(f"No existe pipe nombrado: {PIPE_PATH}")
    exit(1)

#Empleando una variable global podemos mantener registro de cuantas veces se llama al hilo
function_call_counter = 0
#Método 'atómico' de python para asegurar sincronización en tareas críticas. Cuando un hilo
#intente adquirir un lock que ya esté siendo utilizado por otro hilo, estará bloqueado hasta que 
#se haga un lock.release(), analogo a sem_post() POSIX. 
counter_lock = threading.Lock()  

def transform_image_to_bw(image_path):
    global function_call_counter
    try:
        #Con esta estructura, Python facilita la sincronización, al entrar al bloque indentado
        #autómaticamente comienza la zona crítica, al finalizar se libera
        with counter_lock:  # <- similar a sem_wait()
            function_call_counter += 1
            counter = function_call_counter # <- donde termina indentación se libera recurso 
                                            #(o con runtime error) sem_post()
        print(f"[RECEPTOR] Abriendo archivo: {image_path}")
        image = Image.open(image_path)
        #Reaizar operación de imagen, puede ser alterado según necesidad
        bw_image = image.convert('L')
        #Se exagera tiempo de transformación (en realidad es bastante rápido) para fines
        #demostrativos. 
        time.sleep(5) #remover en producción
        #Guardar imagen, se toma como argumento el contador de llamadas de función en nombre
        transformed_path = image_path.replace(".jpg", f"_bw_{counter}.jpg")
        bw_image.save(transformed_path)

        print(f"[RECEPTOR] Imagen guardada como: {transformed_path}")
    except Exception as e:
        print(f"Error durante tranformación: {e}")
    

def receive_and_transform():
    #threads = []  # descomentar si se emplea bloqueo
    with open(PIPE_PATH, 'r') as pipe:
        while True:
            image_path = pipe.read().strip()  # Leer ruta del archivo que viene de programa productor
            
            if image_path:
                print(f"[RECEPTOR] Imagen recibida: {image_path}")
                threading.Thread(target=transform_image_to_bw, args=(image_path,)).start()

                #Alternativa si se quisiera que la función bloquee:
                """
                transform_image_to_bw(image_path)
                thread = threading.Thread(target=transform_image_to_bw, args=(image_path,))
                threads.append(thread)
                thread.start()

                for t in threads:
                    t.join() 
                """

if __name__ == "__main__":
    print("[RECEPTOR] Inicializando programa receptor de imagenes")
    receive_and_transform()
