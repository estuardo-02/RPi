/*
-------------------------------------------------------------
Gabriel Carrera 21216 y Estuardo Castillo 21559
Electrónica Digital 3
Proyecto # 2 - Historiador
07/05/2025
-------------------------------------------------------------

Este programa implementa un "Historiador" para registrar y gestionar eventos recibidos de dos RTUs (Remote Terminal Units) a través de UDP. 
Permite enviar comandos a las RTUs, visualizar eventos en tiempo real, consultar el historial y guardar eventos en un archivo de texto.

**Uso de buffer circular:**  
El programa utiliza un arreglo bidimensional `reportArray[2048][1024]` como buffer circular para almacenar hasta 2048 eventos recibidos. 
Cuando se supera este límite, los eventos más antiguos se sobrescriben, asegurando siempre tener los 2048 eventos más recientes. 
El índice de escritura y lectura se gestiona usando la variable `events` y operaciones módulo (`% 2048`).

**Recomendaciones:**
- Validar siempre los límites de los arreglos para evitar desbordamientos.
- Proteger el acceso concurrente al buffer circular usando semáforos, como se hace en este programa, para evitar condiciones de carrera entre el hilo principal y el hilo de recepción.
- Considerar el manejo de errores al abrir archivos y al recibir datos.
- Si se requiere almacenar más eventos, aumentar el tamaño del buffer circular, pero considerar el uso de memoria.
- Para ambientes de producción, implementar logs rotativos o almacenamiento persistente adicional.

Se emplean funciones de goto, convenientes para el menu
*/

//*************LIBRERIAS*************//
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sched.h>
#include <string.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

#define LOGS "logs.txt"

//*************VARIABLES GLOBALES*************//
FILE *log;
struct sockaddr_in server1, server2, from;
struct hostent *hp, *hp2;
int socketfd;
int i;
int estado = 0;
unsigned long events;
unsigned int length;
char buffer[1024];
char msg[1024];
char reportArray[2048][1024];
char input[3];
sem_t semaphore1;

//*************DECLARACIÓN DE FUNCIONES*************//
void receive_utrs(void *ptr); //Hilo para recibir

//*************HILO PRINCIPAL*************//
int main(int argc, char *argv[])
{
    int portno; //Variable para asignar número de puerto
    pthread_t receive; //Variable tipo pthread

    if (argc != 4) //Si no hay cuatro argumentos, no correr el programa
    {
        printf("This program must be executed like this:\n%s IP_RTU1 IP_RTU2 Port_No.\n", argv[0]); //Escribir
        fflush(stdout); //Escribir
        exit(0); //Salir del programa
    }

    else //Si si chequear puerto...
    {
        if(atoi(argv[3]) < 2000) //Si el puerto es inválido o menor a 2000
        {
            printf("Numero de puerto inválido o menor a 2000\n"); //Escribir
            fflush(stdout); //Escribir
            exit(0); //Salir del programa
        }

        else //Si es válido...
        {
            portno = atoi(argv[3]); //Asignar a la variable del puerto
            printf("Número de puerto %d\n", portno); //Mostrar puerto
            fflush(stdout); //Escribir
        }
    }

    sem_init(&semaphore1, 0, 1); //Iniciar semaphore

    socketfd = socket(AF_INET, SOCK_DGRAM, 0); //Crear socket para UDP
    if(socketfd == -1) //Si es -1...
    {
        printf("Error al crear socket\n"); //Escribir
        fflush(stdout); //Escribir
        exit(-1); //Salir del programa
    }

    hp = gethostbyname(argv[1]); //Chequear si la IP si se encuentra
    if(hp == NULL) //Si no se encuentra...
    {
        printf("Error al obtener información del RTU1\n"); //Escribir
        fflush(stdout); //Escribir
        exit(-1); //Salir del programa
    }

    hp2 = gethostbyname(argv[2]); //Chequear si la IP si se encuentra 
    if(hp2 == NULL) //Si no se encuentra
    {
        printf("Error al obtener información del RTU2\n"); //Error
        fflush(stdout); //Escribir 
        exit(-1); //Salir del programa
    }

    memset(&server1, 0, sizeof(server1)); //Limpiar estructura para la RTU1
    server1.sin_family = AF_INET; //Definir
    server1.sin_port = htons(portno); //Asignar puerto local a trabajar
    inet_aton(argv[1], &server1.sin_addr); //Asignar la IP

    memset(&server2, 0, sizeof(server2)); //Limpiar estructura para la RTU2
    server2.sin_family = AF_INET;
    server2.sin_port = htons(portno); //Asignar puerto local a trabajar
    inet_aton(argv[2], &server2.sin_addr); //Asignar la IP

	length = sizeof(struct sockaddr_in); //Definir la variable para obtener largo de estructura

    memset(msg, 0, sizeof(msg)); //Limpiar mensaje

    sleep(1); //Delay 

    pthread_create(&receive, NULL, (void*)&receive_utrs, NULL); //Iniciar hilo para la recepción de datos

    strcpy(msg, "reconnect"); //Copiar a string
    if(sendto(socketfd, msg, sizeof(msg), 0, (struct sockaddr *)&server1, length) == -1) //Enviar mensaje para que RTU2 tomen información del cliente
    {
        printf("Error sending message\n"); //Escribir
        fflush(stdout); //Escribir
        exit(-1); //Salir del programam
    }

    memset(msg, 0, sizeof(msg)); //Limpiar mensaje

    strcpy(msg, "reconnect"); //Copiar a string
    if(sendto(socketfd, msg, sizeof(msg), 0, (struct sockaddr *)&server2, length) == -1) //Enviar mensaje para que RTU2 tomen información del cliente
    {
        printf("Error sending message\n"); //Escribir
        fflush(stdout); //Escribir
        exit(-1); //Salir del programam
    }

    //*************CICLO*************//
    while(1)
    {
        opciones:
        memset(input, 0, sizeof(input));
        printf("Press 'C' to send command\nPress 'L' to see live events\nPress 'R' to see all previous events\nPress S to save all registered events\nPress 'X' any moment to quit\n"); //Escribir
        fflush(stdout); //Escribir
        fflush(stdin); //Limpiar entrada
        fgets(input, sizeof(input), stdin); //Tomar input
        if (input[0] == 'C' || input[0] == 'c') //Si el input es c
        {
            estado = 0; //Estado en 0
            do 
            {
                memset(msg, 0, sizeof(msg)); //Limpiar buffer del mensaje
                printf("Press Q to quit\n"); //Escribir
                fflush(stdout); //Escribir
                printf("Command must be: RTU#: led#_on or RTU#: led#_off\n"); //Estrucutra de comando
                fflush(stdout); //Escribir
                printf("Command to be sent: "); //Escribir
                fflush(stdout); //Escribir
                fflush(stdin); //Limpiar entrada
                fgets(msg, sizeof(msg), stdin); //Tomar del teclado

                if(msg[3] == '1') //Si el mensaje tiene en la posición 4 un 1 
                {
                    if(sendto(socketfd, msg, sizeof(msg), 0, (struct sockaddr *)&server1, length) == -1) //Enviar mensaje al server 1
                    {
                        printf("Error sending message\n"); //Error
                        fflush(stdout); //Escribir
                        exit(-1); //Salir del programa
                    }
                }

                else if(msg[3] == '2') //Si el mensaje tiene en la posición 4 un 2
                {
                    if(sendto(socketfd, msg, sizeof(msg), 0, (struct sockaddr *)&server2, length) == -1)
                    {
                        printf("Error sending message\n"); //Error al enviar mensaje
                        fflush(stdout); //Escribir
                        exit(-1); //Escribir
                    }
                }

                else if(msg[0] == 'q' || msg[0] == 'Q') //Si se presiona q
                {
                    goto opciones; //Salir al menu
                }

            } while((msg[0] != 'Q') || (msg[0] != 'q'));
        }

        else if(input[0] == 'S' || input[0] == 's') //Si se presiona s
        {
            int num_to_save = 0;
            char temp[16];
            printf("¿Cuántos eventos desea guardar? (máximo 2048): ");
            fflush(stdout);
            fflush(stdin);
            fgets(temp, sizeof(temp), stdin);
            num_to_save = atoi(temp);

            sem_wait(&semaphore1); // Tomar el semáforo

            // Ajustar el número de eventos a guardar al máximo posible
            int total_events = (events < 2048) ? events : 2048;
            if(num_to_save > total_events) num_to_save = total_events;
            if(num_to_save <= 0) num_to_save = total_events;

            log = fopen(LOGS, "w"); // Abrir archivo para guardar eventos

            /* 
            Lógica del buffer circular para guardar los eventos más recientes primero:
            - El evento más reciente está en reportArray[(events - 1) % 2048]
            - El siguiente más reciente en reportArray[(events - 2) % 2048], etc.
            - Guardamos hasta num_to_save eventos en orden descendente.
            */
            for(i = 0; i < num_to_save; i++)
            {
                int idx = (int)((events - 1 - i + 2048) % 2048); // Sumar 2048 para evitar negativos
                fputs(reportArray[idx], log);
                fputs("\n", log);
            }

            sem_post(&semaphore1); // Liberar semáforo
            fclose(log); // Cerrar archivo
            printf("Se guardaron %d eventos en %s\n", num_to_save, LOGS);
            fflush(stdout);
        }

        else if(input[0] == 'R' || input[0] == 'r') //Si se presiona r...
        {
            estado = 0; //Estado en 0 para no visualizar eventos
            sem_wait(&semaphore1); //Se toma el semaphore

            /*
            Lógica del buffer circular para mostrar eventos:
            - Si events < 2048: los eventos están en reportArray[0] a reportArray[events-1]
            - Si events >= 2048: el evento más antiguo está en reportArray[events % 2048], el más reciente en reportArray[(events-1) % 2048]
            - Para mostrar en orden cronológico, recorremos desde el más antiguo usando el índice circular.
            */
            int total_events = (events < 2048) ? events : 2048;
            int start = (events < 2048) ? 0 : (events % 2048);
            for(i = 0; i < total_events; i++)
            {
                int idx = (start + i) % 2048;
                printf("%s\n", reportArray[idx]); //Mostrar en la terminal
                fflush(stdout); //Escribir
            }

            sem_post(&semaphore1); //Devolver el semaphore
        }
        
        else if(input[0] == 'L' || input[0] == 'l') //Si el input es l...
        {
            estado = 1; //Se cambia el estado para mostrar eventos en la terminal
            regresar: //Label
            memset(input, 0, sizeof(input)); //Limpiar mensaje
            printf("Press Q to quit\n"); //Mostrar en la terminal
            fflush(stdout); //Escribir
            fflush(stdin); //Limpiar entrada
            fgets(input, sizeof(input), stdin); //Tomar del teclado
            if(input[0] == 'Q' || input[0] == 'q') //Si se presiona q...
            {
                estado = 0; //Salir y cambiar a que no se muestren eventos
            }

            else
            {
                goto regresar; //Si no se ingresa lo solicitado
            }

        }

        else if(input[0] == 'x' || input[0] == 'X') //Si se presiona X, se sale del programa
        {
            close(socketfd); //Cerrar socket
            exit(0); //Salir del programa
        }

        else
        {
            printf("Unkown command try again\n"); //Ninguno de los anteriores
            fflush(stdout); //No hacer nada
        }
    }

    return 0;
}

//*************HILO PARA RECIBIR DEL RTUs*************//
void receive_utrs(void *ptr)
{
    while(1)
    {
        memset(buffer, 0, sizeof(buffer)); //Limpiar buffer para recibir mensaje
        if(recvfrom(socketfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&from, &length) == -1) //Si se recibió algo y chequear error
        {
            printf("Error al recibir información\n"); //Error
            fflush(stdout); //Escribir
            exit(-1); //Salir del programa
        }
        sem_wait(&semaphore1);
        strcpy(reportArray[events % 2048], buffer);
        events++;
        sem_post(&semaphore1);
        if(estado == 1)
        {
            printf("%s\n", buffer); //Mostrar en terminal
            fflush(stdout); //Escrbir
        }
    }

    pthread_exit(0); //Salir del hilo
}
