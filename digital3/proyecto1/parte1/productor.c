/*
Universidad Del Valle de Guatemala 1
Facultad de Ingeniería
Departamento de Ingeniería Electrónica, Mecatrónica y Biomédica
IE3059 – Electrónica Digital 3, Primer Ciclo, 2025

Estuardo Castillo 21559

Programa productor de proyecto 1. El siguiente programa está encargado de crear un pipe 'Info' para
interactar con programa consumidor. se crean subprocesos con fork() para encargarse del envío de mensajes
Uso del archivo: ./exec [mensajes] [productores]

productor.c
*/

#include <unistd.h>  
#include <sys/types.h>  
#include <fcntl.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <time.h>  
#include <ctype.h>  

//valores constantes se definen en preprocesador. 
#define M 10           // Valor por defecto para M  
#define N 2            // Valor por defecto para N  
#define M_MAX 50       // Valor máximo para M  
#define N_MAX 10       // Valor máximo para N  
#define PIPE_NAME_SIZE 20  
#define MESSAGE_SIZE 50  
//se definen los valores para timing de mensajes constantes
#define SLEEP_MIN 20000 // en ms
#define SLEEP_MAX 100000 

int main(int argc, char *argv[]) {  
    int m = M;  // Mensajes por defecto  
    int n = N;  // Productores por defecto  

    // Procesar argumentos de entrada  
    if(argc == 1){
        //mensaje a usuario, no interrumpe flujo funciona como advertencia. 
        fprintf(stderr,"Uso correcto: %s [mensajes] [productores]\n", argv[0]);
    }
    if (argc > 1) {  
        m = atoi(argv[1]);  
        if (m > M_MAX) m = M_MAX;  
    }  
    if (argc > 2) {  
        n = atoi(argv[2]);  
        if (n > N_MAX) n = N_MAX;  
    }  

    // Crear un pipe nombrado para la información  
    // Crear pipe nombrado Info
    int dummy;
    int info_fd; //file descriptor
    dummy = system("mkfifo Info");

    // Se abre el pipe nombrado para escribir información relevante 
    if((info_fd = open("Info", O_WRONLY)) < 0)
    {
        printf("pipe Info error\n");
        exit(-1);
    }
    //send message
    if (write(info_fd, &m, sizeof(m)) <  0 ){
        printf("pipe Info error\n");
        close(info_fd);
        exit(-1);
    }
    if (write(info_fd, &n, sizeof(n)) <  0 ){
        printf("pipe Info error\n");
        close(info_fd);
        exit(-1);
    }
    // Esperar un segundo  
    //sleep(1);  
    //mejor: esperar confirmación 'ack' de consumidor cuando estén listos los named pipes
    info_fd = open("Info", O_RDONLY);  
    char sync_message[30];  
    read(info_fd, sync_message, sizeof(sync_message));  
    printf("Sincronización recibida: %s\n", sync_message);  
    close(info_fd);  

    // Crear los procesos hijos productores  
    for (int i = 0; i < n; i++) {  
        pid_t pid = fork();  
        
        if (pid < 0) {  
            perror("Error al crear proceso hijo");  
            exit(1);  
        }  
        if (pid == 0) { // Proceso hijo  
            char pipe_name[PIPE_NAME_SIZE];  
            //La función snprintf() es idéntica a la función sprintf() con la adición del argumento n ,
            //que indica el número máximo de caracteres (incluido el carácter nulo final) que se deben 
            //escribir en el almacenamiento intermedio, permite cadenas más elaboradas. 
            snprintf(pipe_name, PIPE_NAME_SIZE, "pipe%d", i + 1);  
            //sleep(1);
            int pipe_fd = open(pipe_name, O_WRONLY);  
            if (pipe_fd < 0) {  
                perror("Error al abrir el pipe para escritura");  
                exit(1);  
            }  

            for (int j = 0; j < m; j++) {  
                // Generar mensaje  
                char message[MESSAGE_SIZE];  
                snprintf(message, MESSAGE_SIZE, "Msj. %d, Prod. %d", j + 1, i + 1);  
                write(pipe_fd, message, strlen(message) + 1);  
                printf("Prod. %d envió: %s\n", i + 1, message);  

                // Dormir entre 20 y 100 ms  
                usleep(SLEEP_MIN + rand() % (SLEEP_MAX - SLEEP_MIN));  
            }  

            close(pipe_fd);  
            exit(0); // Terminar el proceso hijo  
        }  
    }  
    /*
    *Notar que hay un par de formas de aguardar sincronización de que los mensajes terminen
    se puede usar un mensaje a través de pipe info, por ejemplo. Sin embargo aquí se demuestra otra
    forma efectiva con la instrucción wait(NULL); que aguarda a que un proceso hijo finalice 
    
    for (int i = 0; i < n; i++) {  //cuenta termina cuando todos los hijos terminen
        wait(NULL);  //hasta que un hijo termine se sube la cuenta
    }  
    */
    // Esperar a recibir confirmación del consumidor  
    info_fd = open("Info", O_RDONLY);  
    char end_message[30];  
    read(info_fd, end_message, sizeof(end_message));  
    printf("%s\n", end_message);  

    close(info_fd);  

    return 0;  
}  
