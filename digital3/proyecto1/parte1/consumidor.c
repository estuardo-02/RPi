/*
Universidad Del Valle de Guatemala 1
Facultad de Ingeniería
Departamento de Ingeniería Electrónica, Mecatrónica y Biomédica
IE3059 – Electrónica Digital 3, Primer Ciclo, 2025

Estuardo Castillo 21559

Programa consumidor de proyecto 1. El siguiente programa está encargado de crear pipes para el envío de información según
la información que le envía el programa productor. Por ende, este programa depende de la ejecución del productor para poder operar
correctemente. Para la recepción de mensaje se emplean threads, importante para la compilación incuir la directriz -lpthread
La sincronización en la zona crítica se logra mediante un semaphore binario 
Para fines de demostración de creación de pipes se omite este método para destruirlos, puedeemplearse en línea de comandos 
    -$ rm pipes*
*******Importante********
Para la creación de pipes se emplea el método mkfifo() necesario incluir <sys/types.h> & <sys/stat.h>; Resulta conveniente para 
crear múltiples pipes nombrados cuando se tiene el nombre de la tubería en una cadena, para la creación de la cadena
se emplea snprintf, similar a sprintf (se explicó en archivo productor).
Información adicional: https://manpages.ubuntu.com/manpages/focal/es/man3/mkfifo.3.html

consumidor.c
*/
#include <unistd.h>  
#include <sys/types.h>  
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <pthread.h>  
#include <semaphore.h>  

#define M_MAX 50 // Máximo número de mensajes  
#define STRING_SIZE 50 // Maximo tamaño de mensaje  
#define N_MAX 10  // Maximo número de productores  
#define PIPE_NAME_SIZE 20  

// Variables globales  
sem_t semaphore; // Semáforo binario  
char messages[M_MAX * N_MAX][STRING_SIZE];   //no se emplea alocación dinámica, considerar peor caso aunque desperdicie recursos
int msg_index = 0;  
int n, m; // Número de productores y mensajes  

// Función de los hilos para leer los mensajes de los pipes  
void *read_messages(void *arg) {  
    int pipe_index = *(int *)arg;  
    char pipe_name[PIPE_NAME_SIZE];  
    snprintf(pipe_name, PIPE_NAME_SIZE, "pipe%d", pipe_index + 1);  

    int pipe_fd = open(pipe_name, O_RDONLY);  
    if (pipe_fd < 0) {  
        perror("Error al abrir el pipe para lectura");  
        pthread_exit(NULL);  
    }  

    for (int j = 0; j < m; j++) {  
        char message[STRING_SIZE];  
        read(pipe_fd, message, STRING_SIZE);  

        // Usar semáforo para asegurar que este bloque de código no corrompa cadena
        sem_wait(&semaphore); // Esperar antes de entrar en la sección crítica  
        strcpy(messages[msg_index++], message);  
        sem_post(&semaphore); // Liberar el semáforo después de la sección crítica  
    }  

    close(pipe_fd);  
    pthread_exit(NULL);  
}  

int main() {  
    // Lectura de la información desde el pipe  
    //mkfifo("Info", 0666);  alternativa
    int info_fd = open("Info", O_RDONLY);  
    if (info_fd == -1) {  
        perror("Error al abrir el pipe Info");  
        exit(-1);  
    }  
    if(read(info_fd, &m, sizeof(m)) < 0){
        printf("Info pipe read error\n");
        exit(-1);
    }
    if(read(info_fd, &n, sizeof(n)) < 0){
        printf("Info pipe read error\n");
        exit(-1);
    }    
    close(info_fd);  
    printf("Valores recibidos para M: %d, N: %d \n", m, n);

    //Luego de adquirir la información, ya tengo lo necesario para generar los tubos nombrados
    char pipe_name[PIPE_NAME_SIZE];  
    for (int i = 0; i < n; i++) {  
        snprintf(pipe_name, PIPE_NAME_SIZE, "pipe%d", i + 1);  
        mkfifo(pipe_name, 0666); // Crear el pipe nombrado para cada productor  
    }  
    // Enviar mensaje de sincronización a los productores  
    info_fd = open("Info", O_WRONLY);  
    const char *sync_message = "Pipes creados.";  
    write(info_fd, sync_message, strlen(sync_message) + 1); // +1 para incluir el terminador nulo  
    close(info_fd);  

    // Inicializar el semáforo  
    sem_init(&semaphore, 0, 1); // Semáforo binario inicializado a 1  

    pthread_t threads[N_MAX];  
    int thread_indices[N_MAX];  

    // Crear hilos para leer mensajes  
    for (int i = 0; i < n; i++) {  
        thread_indices[i] = i; // Se necesita guardar el índice para el argumento del hilo  
        pthread_create(&threads[i], NULL, read_messages, (void *)&thread_indices[i]);  
    }  

    // Esperar a que todos los hilos terminen  
    for (int i = 0; i < n; i++) {  
        pthread_join(threads[i], NULL);  
    }  

    // Escribir los mensajes en el archivo  
    FILE *file = fopen("Mensajes_recibidos.txt", "w");  
    for (int i = 0; i < msg_index; i++) {  
        printf("%s\n", messages[i]);  
        fprintf(file, "%s\n", messages[i]);  
    }  
    fclose(file);  

    // Enviar mensaje final al productor  
    info_fd = open("Info", O_WRONLY);  
    char end_message[] = "Productores han terminado";  
    write(info_fd, end_message, sizeof(end_message));  
    close(info_fd);  

    /*
    //**Eliminación de pipes, no implementada para fines demostrativos**

    for (int i = 0; i < n; i++) {  
        char pipe_name[PIPE_NAME_SIZE];  
        snprintf(pipe_name, PIPE_NAME_SIZE, "pipe%d", i + 1);  
        unlink(pipe_name); // Eliminar el pipe nombrado  
    }  
    printf("Pipes eliminados.\n");  
    */ 

    // Destruir el semáforo antes de cerrar el programa  
    sem_destroy(&semaphore);  
    return 0;  
}  