/*
-------------------------------------------------------------
Gabriel Carrera 21216 y Estuardo Castillo 21559
Electrónica Digital 3
Proyecto # 2 - RTU 1
06/05/2025
-------------------------------------------------------------
*/

//*************LIBRERIAS*************
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <time.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/timerfd.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <softServo.h>

//************* DEFINICIONES *************
#define SPI_CHANNEL       0
#define SPI_SPEED    1500000
#define ADC_CHANNEL       0
#define VDD 3.3f
#define BUZZER            27
#define LED1              23
#define LED2              24
#define LED_IOT           18
#define SERVO             22

// Entradas digitales
#define BUTTON1           12
#define BUTTON2           16
#define DIP1              20
#define DIP2              21


//*************PROTOTIPOS DE FUNCIONES*************
void get_ADC(void *ptr);
void receive_UDP(void *ptr);
void getTime(void *ptr);
void sound_buzzer(void *ptr);
void boton1(void);
void boton2(void);
void dip1(void);
void dip2(void);
int map(int value, int inMin, int inMax, int outMin, int outMax);

//*************VARIABLES GLOBALES*************
char led1_state[5];
char led2_state[5];
char button1_state[15];
char button2_state[15];
char dip1_state[5];
char dip2_state[5];
// Variables globales para guardar estados previos de los dips
int dip1_previous_state = -1;  // -1 indica que no se ha leído aún
int dip2_previous_state = -1;
char lediot_state[5];
char tempBuffer[256];
char timeString[256];
char msg[1024];
char report[1024];
char reporte[1024][1024];
char stringArray[1024][1024];
char *token;
int angle;
int socketfd;
int events;
int overcharge_flag = 0;
int undercharge_flag = 0;
int normal_flag = 0;
float adc_voltage;
unsigned int length;
struct sockaddr_in server, client, client2, from;
struct tm *timeinfo;
struct timeval tv;
time_t u, t;
sem_t semaphore1;

//*************HILO PRINCIPAL*************
int main(int argc, char *argv[])
{
    int portno; //Variable para guardar número de puerto
    strcpy(led1_state, "OFF"); //Iniciar con estados por defecto en apagado
    strcpy(led2_state, "OFF"); //Iniciar con estados por defecto en apagado
    strcpy(lediot_state, "OFF"); //Iniciar con estados por defecto en apagado
    strcpy(button1_state, "UNPRESSED"); //Iniciar con estados por defecto en no presionado
    strcpy(button2_state, "UNPRESSED"); //Iniciar con estados por defecto en no presionado

    if (argc != 2) //Si el programa se ejecuta sin argumentos...
    {
        printf("Debe especificar número de puerto: %s no.de.puerto\n", argv[0]);
        fflush(stdout); //Escribir
        exit(0); //Salir del programa
    }

    else //Si se escribe un argumento
    {
        if(atoi(argv[1]) < 2000) //Si el argumento fue menor a 2000 o inválido...
        {
            printf("Numero de puerto inválido o menor a 2000\n");
            fflush(stdout); //Escribir 
            exit(0); //Salir del porgrama
        }

        else //Si el puerto fue un número válido
        {
            portno = atoi(argv[1]); //Asignar a la variable
            printf("Número de puerto %d\n", portno); //Escribir
            fflush(stdout); //Mostrar
        }
    }

    sem_init(&semaphore1, 0, 1); //Inicializar semaphore en 1

    int i; //Variable para ciclo for
    pthread_t ADC; //Variable para hilo del ADC
    pthread_t receive; //Variable para hilo del recibir comandos
    pthread_t calTime; //Variable para hilo del tiempo
    pthread_t buzzer_var; //Variable para hilo de buzzer

    //--Configuracion de GPIOs--//
    wiringPiSetupGpio(); //setup de pines de wiringPi para usar los nums de GPIO

    //LEDS y Buzzer
    pinMode(LED1, OUTPUT); //pin LED1 como salida
    pinMode(LED2, OUTPUT); //pin LED2 como salida
    pinMode(LED_IOT, OUTPUT); //pin LED_IOT como salida
    pinMode(BUZZER, OUTPUT); //pin BUZZER como salida
    
    digitalWrite(LED1, LOW); //Iniciar con led1 apagado
    digitalWrite(LED2, LOW); //Iniciar con led2 apagado
    digitalWrite(LED_IOT, LOW); //Iniicar con led_iot apagado
    digitalWrite(BUZZER, LOW); //Iniicar con buzzer apagado
    
    //Botones
    pinMode(BUTTON1, INPUT); //pin BUTTON1 como entrada
    pullUpDnControl(BUTTON1, PUD_UP); //pin BUTTON1 con pull-up interno
    wiringPiISR(BUTTON1, INT_EDGE_FALLING, &boton1); //Asignar interrupción al pin BUTTON1 en flanco de bajada

    pinMode(BUTTON2, INPUT); //pin BUTTON2 como entrada
    pullUpDnControl(BUTTON2, PUD_UP); //pin BUTTON2 con pull-up interno
    wiringPiISR(BUTTON2, INT_EDGE_FALLING, &boton2); //Asignar interrupción al pin BUTTON2 en flanco de bajada

    //DIP SWITCHES
    pinMode(DIP1, INPUT); //pin DIP1 como entrada
    pullUpDnControl(DIP1, PUD_DOWN); //pin DIP1 con pull-down interno
    wiringPiISR(DIP1, INT_EDGE_BOTH, &dip1); //Asignar interrupción al pin DIP1 en flanco de ambos lados

    pinMode(DIP2, INPUT); //pin DIP2 como entrada
    pullUpDnControl(DIP2, PUD_DOWN); //pin DIP2 con pull-down interno
    wiringPiISR(DIP2, INT_EDGE_BOTH, &dip2); //Asignar interrupción al pin DIP2 en flanco de ambos lados
    
    softServoSetup (SERVO, -1, -1, -1, -1, -1, -1, -1); //Función para inicializar el servo

    if(wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) < 0) //Setear SPI con el canal definido y la velocidad de reloj definida y chequear error
    {
        printf("wiringPiSPISetup falló.\n");
        fflush(stdout); //Escribir
        exit(-1); //Salir del programa
    }

    //Lecturas iniciales
    if(digitalRead(DIP1) == LOW) //Si el DIP1 está en bajo...
    {
        strcpy(dip1_state, "OFF"); //Estado del DIP1 apagado
    }
    
    else //Si el DIP1 está en alto...
    {
        strcpy(dip1_state, "ON"); //Estado del DIP1 encendido
    }
    
    if(digitalRead(DIP2) == LOW) //Si el DIP2 está en bajo...
    {
        strcpy(dip2_state, "OFF"); //Estado del DIP2 apagado
    }
    
    else //Si el DIP2 está en alto...
    {
        strcpy(dip2_state, "ON"); //Estado del DIP2 encendido
    }

    socketfd = socket(AF_INET, SOCK_DGRAM, 0); //Se crea el socket con protocolo UDP
    if(socketfd == -1) //Si el socket devuelve -1
    {
        printf("Error al crear socket\n");
        fflush(stdout); //Escrbir
        exit(-1); //Salir del programa
    }

    length = sizeof(server); //Variable para tener el tamaño de la estructura
	memset((char *)&server, 0, length); //Se limpia la estructura

    server.sin_family = AF_INET;
	server.sin_port = htons(portno); //Puerto para trabajar
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(socketfd, (struct sockaddr *)&server, length) == -1) //Asociar el socket con la estructura proporcionada y chequear error
    {
        printf("Error al asociar socket con estructura\n");
        fflush(stdout); //Escribir
        exit(-1); //Salir del programa
    }
    
    memset(msg, 0, sizeof(msg)); //Limpiar de mensaje

    if(recvfrom(socketfd, msg, sizeof(msg), 0, (struct sockaddr *)&client, &length) == -1) //Recibir mensaje para identificar
    {
        printf("Error a recibir comando\n");
        fflush(stdout); //Escribir
        exit(-1); //Salir del programa
    }
    
    pthread_create(&receive, NULL, (void*)&receive_UDP, NULL); //Iniciar hilo para recibir comandos
    pthread_create(&calTime, NULL, (void*)&getTime, NULL); //Iniciar hilo para 
    pthread_create(&ADC, NULL, (void*)&get_ADC, NULL); //Iniciar hilo para obtener ADC
    pthread_create(&buzzer_var, NULL, (void*)&sound_buzzer, NULL); //Iniciar hilo para buzzer
    
    //Ciclo
    while(1)
    {
        sem_wait(&semaphore1); //Iniciar semaphore para que no se ejecuten más eventos
		
        //Obtener reporte
        sprintf(report, "RTU1 REPORT %s\nButton 1: %s, Button 2: %s,\nInterruptor 1: %s, Interruptor 2: %s,\nLed 1: %s, Led 2: %s,\nIot Led: %s,\nVoltage on line 1: %.2f\n", timeString, button1_state, button2_state, dip1_state, dip2_state, led1_state, led2_state, lediot_state, adc_voltage);
        
		
		for(i = 0; i < events; i++)
		{
			if(sendto(socketfd, reporte[i], strlen(reporte[i]) + 1, 0, (struct sockaddr *)&client, length) == -1)
			{
				printf("Error al enviar mensaje\n");
				fflush(stdout);
				exit(-1);
			}
			memset(reporte[i], 0, sizeof(reporte[i]));
		}
		events = 0;
        
        //Enviar reporte
        if(sendto(socketfd, report, sizeof(report), 0, (struct sockaddr *)&client, length) == -1) //Enviar al cliente y chequear error...
        {
            printf("Error al enviar mensaje\n");
            fflush(stdout); //Escribir
            exit(-1); //Salir del programa
        }
        
        memset(report, 0, sizeof(report));

        events = 0; //Reiniciar eventos
        sem_post(&semaphore1); //Devolver semaphore
        usleep(1000); //Delay mínimo
        sleep(2); //Delay de 2s
    }

    return 0;
}

//*************HILO PARA RECIBIR COMANDOS*************//
void receive_UDP(void *ptr)
{
    //Ciclo
    while(1)
    {
        memset(msg, 0, sizeof(msg)); //Limpiar el mensaje
        if(recvfrom(socketfd, msg, sizeof(msg), 0, (struct sockaddr *)&from, &length) == -1) //Si se recibió algo del client y chequear error
        {
            printf("Error a recibir comando\n"); 
            fflush(stdout); //Escribir
            exit(-1); //Salir
        }
        
        printf("%s\n", msg);
        
        if(strstr(msg, "reconnect") != NULL)
        {
            client = from;
        }

        if(strstr(msg, "RTU1") != NULL) //Chequear si el mensaje va para la RTU1 o no
        {
            if(strstr(msg, "led1_on") != NULL) //Si el mensaje contiene led1_on...
            {
                if(digitalRead(LED1) == LOW) //Si la led1 estaba apagada encenderla
                {
                    digitalWrite(LED1, HIGH); //Encender led1
                    strcpy(led1_state, "ON"); //Poner estado de led1 en encendido
                    sem_wait(&semaphore1); //Tomar semaphore
                    //sprintf(stringArray[events], "Command received: Led 1 turned ON at %s\n", timeString); //Guardar en eventos
                    //Obtener reporte
					sprintf(reporte[events], "RTU1 REPORT AT EVENT: LED1 turned on at %s\nButton 1: %s, Button 2: %s,\nInterruptor 1: %s, Interruptor 2: %s,\nLed 1: %s, Led 2: %s,\nIot Led: %s,\nVoltage on line 1: %.2f\n", timeString, button1_state, button2_state, dip1_state, dip2_state, led1_state, led2_state, lediot_state, adc_voltage);
					events++;
                    sem_post(&semaphore1); //Devolver semaphore
                    usleep(1000); //Delat mínimo
                }
            }

            else if(strstr(msg, "led1_off") != NULL) //Si el mensaje contiene led1_off...
            {
                if(digitalRead(LED1) == HIGH) //Si la led1 estaba encendida apagarla
                {
                    digitalWrite(LED1, LOW); //Apagar led1
                    strcpy(led1_state, "OFF"); //Poner estado de led1 en apagado
                    sem_wait(&semaphore1); //Tomar semaphore
                    //sprintf(stringArray[events], "Command received: Led 1 turned OFF at %s\n", timeString); //Guardar en eventos
                    //Obtener reporte
					sprintf(reporte[events], "RTU1 REPORT AT EVENT: LED1 turned off at %s\nButton 1: %s, Button 2: %s,\nInterruptor 1: %s, Interruptor 2: %s,\nLed 1: %s, Led 2: %s,\nIot Led: %s,\nVoltage on line 1: %.2f\n", timeString, button1_state, button2_state, dip1_state, dip2_state, led1_state, led2_state, lediot_state, adc_voltage);
					events++;
                    sem_post(&semaphore1); //Devolver semaphore
                    usleep(1000);
                }
            }

            else if(strstr(msg, "led2_on") != NULL) //Si el mensaje contiene led2_on...
            {
                if(digitalRead(LED2) == LOW) //Si la led1 estaba apagada encenderla
                {
                    digitalWrite(LED2, HIGH); //Encender led2
                    strcpy(led2_state, "ON"); //Poner estado de led2 en encendido
                    sem_wait(&semaphore1); //Tomar semaphore
                    //sprintf(stringArray[events], "Command received: Led 2 turned ON at %s\n", timeString); //Guardar en eventos
                    //Obtener reporte
					sprintf(reporte[events], "RTU1 REPORT AT EVENT: LED2 turned on at %s\nButton 1: %s, Button 2: %s,\nInterruptor 1: %s, Interruptor 2: %s,\nLed 1: %s, Led 2: %s,\nIot Led: %s,\nVoltage on line 1: %.2f\n", timeString, button1_state, button2_state, dip1_state, dip2_state, led1_state, led2_state, lediot_state, adc_voltage);
					events++;
                    sem_post(&semaphore1); //Devolver semaphore
                    usleep(1000); //Delay mínimo
                }
            }

            else if(strstr(msg, "led2_off") != NULL) //Si el mensaje contiene led2_off...
            {
                if(digitalRead(LED2) == HIGH) //Si la led2 estaba encendida apagarla
                {
                    digitalWrite(LED2, LOW); //Apagar led2
                    strcpy(led2_state, "OFF"); //Poner estado de led2 en apagada
                    sem_wait(&semaphore1); //Tomar semaphore
                    //sprintf(stringArray[events], "Command received: Led 2 turned OFF at %s\n", timeString); //Guardar en eventos
                    //Obtener reporte
					sprintf(reporte[events], "RTU1 REPORT AT EVENT: LED2 turned off at %s\nButton 1: %s, Button 2: %s,\nInterruptor 1: %s, Interruptor 2: %s,\nLed 1: %s, Led 2: %s,\nIot Led: %s,\nVoltage on line 1: %.2f\n", timeString, button1_state, button2_state, dip1_state, dip2_state, led1_state, led2_state, lediot_state, adc_voltage);
					events++;
                    sem_post(&semaphore1); //Devolver semaphore
                    usleep(1000); //Delay mínimo
                }
            }
            
            else if(strstr(msg, "led_IoT") != NULL) //Si el mensaje contiene led1_on...
            {
                digitalWrite(LED_IOT, !digitalRead(LED_IOT));
                if(digitalRead(LED_IOT) == HIGH) //Si la led1 estaba apagada encenderla
                {
                    strcpy(lediot_state, "ON"); //Poner estado de led1 en encendido
                    sem_wait(&semaphore1); //Tomar semaphore
                    //sprintf(stringArray[events], "RTU1: Led IoT ON at %s\n", timeString); //Guardar en eventos
                    //Obtener reporte
					sprintf(reporte[events], "RTU1 REPORT AT EVENT: LED IoT turned on at %s\nButton 1: %s, Button 2: %s,\nInterruptor 1: %s, Interruptor 2: %s,\nLed 1: %s, Led 2: %s,\nIot Led: %s,\nVoltage on line 1: %.2f\n", timeString, button1_state, button2_state, dip1_state, dip2_state, led1_state, led2_state, lediot_state, adc_voltage);
					events++;
                    sem_post(&semaphore1); //Devolver semaphore
                    usleep(1000); //Delat mínimo
                }
                
                else
                {
                    strcpy(lediot_state, "OFF"); //Poner estado de led1 en encendido
                    sem_wait(&semaphore1); //Tomar semaphore
                    //sprintf(stringArray[events], "RTU1: Led IoT OFF at %s\n", timeString); //Guardar en eventos
                    //Obtener reporte
					sprintf(reporte[events], "RTU1 REPORT AT EVENT: LED IoT turned off at %s\nButton 1: %s, Button 2: %s,\nInterruptor 1: %s, Interruptor 2: %s,\nLed 1: %s, Led 2: %s,\nIot Led: %s,\nVoltage on line 1: %.2f\n", timeString, button1_state, button2_state, dip1_state, dip2_state, led1_state, led2_state, lediot_state, adc_voltage);
					events++;
                    sem_post(&semaphore1); //Devolver semaphore
                    usleep(1000); //Delat mínimo
                }
            
            else if(strstr(msg, "Servo") != NULL) //Si se recibe servo...
            {
                token = strtok(msg, ":"); //Dividir el string para obtener ángulo
                token = strtok(NULL, ":"); //Dividir
                softServoWrite(SERVO, atoi(token)); //Enviarlo a la función para el servo
                angle = map(atoi(token), -500, 1500, 0, 180); //Mapear para mostrar el ángulo de 0 a 180
                sem_wait(&semaphore1); //Tomar el semapohre
                //sprintf(stringArray[events], "Servo moved at angle %d at %s\n", angle, timeString); //Añadirlo
                //Obtener reporte
				sprintf(reporte[events], "RTU1 REPORT AT EVENT: Servo moved at angle %d at %s\nButton 1: %s, Button 2: %s,\nInterruptor 1: %s, Interruptor 2: %s,\nLed 1: %s, Led 2: %s,\nIot Led: %s,\nVoltage on line 1: %.2f\n", angle, timeString, button1_state, button2_state, dip1_state, dip2_state, led1_state, led2_state, lediot_state, adc_voltage);
				events++;
                sem_post(&semaphore1); //Devolver semaphore
                usleep(1000); //Delay mínimo
            }
            

            else //Si el comando no es ninguno definido...
            {
                printf("Unkown command\n");
                fflush(stdout); //Escribir
            }
        }

        else
        {
            printf("Not me\n"); //No me escribieron a mi
            fflush(stdout); //Escribir
        }
    }

    pthread_exit(0); //Salir del hilo
}

//*************HILO PARA EL ADC*************//
void get_ADC(void *ptr)
{
    uint8_t spiData[2]; // La comunicación usa dos bytes
    uint16_t adc; //Variable para guardar los 10 bits del ADC

    while(1)
    {
        spiData[0] = 0b01101000 | (ADC_CHANNEL << 4); //Enviar 
        spiData[1] = 0;
        wiringPiSPIDataRW(SPI_CHANNEL, spiData, 2); //Escrbir configuración y leer 
        adc = (spiData[0] << 8) | spiData[1]; //
        adc_voltage = ((float)adc/ 1023) * VDD; //Obtener valor en float
		usleep(1000); //Delay mínimo 

        if ((adc_voltage <= 2.5) && (adc_voltage >= 0.5) && (normal_flag == 0)) //Si se está en operación normal...
        {
            normal_flag = 1; //Setear bandera de carga normal a 1
            overcharge_flag = 0; //Setear bandera de sobrecarga a 0
            undercharge_flag = 0; //Setear bandera de baja carga a 0
            printf("Normal Operation\n");
			fflush(stdout);
            sem_wait(&semaphore1); //Tomar semaphore
            //sprintf(stringArray[events], "RTU1: Normal Operation on line at %s\n", timeString); //Guardar en eventos
            //Obtener reporte
			sprintf(reporte[events], "RTU1 REPORT AT EVENT: Normal Operation on line at %s\nButton 1: %s, Button 2: %s,\nInterruptor 1: %s, Interruptor 2: %s,\nLed 1: %s, Led 2: %s,\nIot Led: %s,\nVoltage on line 1: %.2f\n", timeString, button1_state, button2_state, dip1_state, dip2_state, led1_state, led2_state, lediot_state, adc_voltage);
            events++;
            sem_post(&semaphore1); //Devolver semaphore
            usleep(1000); //Delay mínimo
            usleep(500000); 
        }

        else if((adc_voltage > 2.45) && (overcharge_flag == 0)) //Si el adc es mayor a 2.5V
        {
            printf("Overcharge on line\n");
            fflush(stdout);
            normal_flag = 0; //Setear bandera de carga normal a 0
            undercharge_flag = 0; //Setear bandera de baja carga a 0
            overcharge_flag = 1; //Setear bandera de sobrecarga a 1
            sem_wait(&semaphore1); //Tomar semaphore
            //sprintf(stringArray[events], "RTU1: Overcharge on line at %s\n", timeString); //Guardar en eventos
            //Obtener reporte
			sprintf(reporte[events], "RTU1 REPORT AT EVENT: Overcharge on line at %s\nButton 1: %s, Button 2: %s,\nInterruptor 1: %s, Interruptor 2: %s,\nLed 1: %s, Led 2: %s,\nIot Led: %s,\nVoltage on line 1: %.2f\n", timeString, button1_state, button2_state, dip1_state, dip2_state, led1_state, led2_state, lediot_state, adc_voltage);
            events++;
            sem_post(&semaphore1); //Devolver semaphore
            usleep(1000); //Delay mínimo
            usleep(500000); 
        }

        else if((adc_voltage < 0.45) && (undercharge_flag == 0))
        {
            printf("Undercharge on line\n");
            fflush(stdout); //Escribir
            overcharge_flag = 0; //Setear bandera de sobrecarga a 0
            normal_flag = 0; //Setear bandera de carga normal a 0
            undercharge_flag = 1; //Setear bandera de baja carga a 1
            sem_wait(&semaphore1); //Tomar semaphore
            //sprintf(stringArray[events], "RTU1: Undercharge on line at %s\n", timeString); //Guardar en eventos
            //Obtener reporte
			sprintf(reporte[events], "RTU1 REPORT AT EVENT: Undercharge on line at %s\nButton 1: %s, Button 2: %s,\nInterruptor 1: %s, Interruptor 2: %s,\nLed 1: %s, Led 2: %s,\nIot Led: %s,\nVoltage on line 1: %.2f\n", timeString, button1_state, button2_state, dip1_state, dip2_state, led1_state, led2_state, lediot_state, adc_voltage);
            events++;
            sem_post(&semaphore1); //Devolver semaphore
            usleep(1000); //Delay mínimo 
            usleep(500000);
        }
    }

    pthread_exit(0); //Salir del hilo
}

//*************HILO PARA OBTENER EL TIEMPO*************//
void getTime(void*ptr)
{
    while(1)
    {
        if(digitalRead(BUTTON1) == HIGH) //Si el botón 1 no está presionado...
        {
            strcpy(button1_state, "UNPRESSED"); //Establecer como no presionado
        } 

        else //Si el botón 1 está presionado...
        {
            strcpy(button1_state, "PRESSED"); //Establecer como presionado
        }

        if(digitalRead(BUTTON2) == HIGH) //Si el botón 2 no está presionado...
        {
            strcpy(button2_state, "UNPRESSED"); //Establecer como no presionado
        } 

        else //Si el botón 2 está presionado...
        {
            strcpy(button2_state, "PRESSED"); //Establecer como presionado
        }
        
        gettimeofday(&tv, NULL); //Obtner tiempo basado en la estrucutura tv
        t = tv.tv_sec; //Segundos a t
        u = tv.tv_usec; //Microsegundos a u
        timeinfo = localtime(&t); //Obtener tiempo local
        strftime(tempBuffer, sizeof(tempBuffer), "%A %B %d at %H:%M:%S:", timeinfo); // Format hour, minute, second
        sprintf(timeString, "%s%ld", tempBuffer, u/1000); //Finalizar string formateada
    }

    pthread_exit(0); //Salir del hilo
}

//*************HILO PARA SONAR BUZZER*************//
void sound_buzzer(void *ptr)
{
    while(1)
    {
        while((overcharge_flag == 1) || (undercharge_flag == 1)) //Si hay sobrecarga o baja carga...
        {
            digitalWrite(BUZZER, HIGH); //Encender pin
        }

        digitalWrite(BUZZER, LOW); //Mantener en bajo
    }

    pthread_exit(0);
}

//*************FUNCIONES PARA INTERRUPCIONES*************//
void boton1(void)
{
    delay(20); // Delay inicial para estabilizar
    if(digitalRead(BUTTON1) == LOW) // Confirma que sigue presionado
    {
        usleep(10000); // Debounce adicional
        if(digitalRead(BUTTON1) == LOW) // Verificación final
        {
            printf("RTU1: Button 1 pressed\n");
            fflush(stdout);
            sem_wait(&semaphore1);
            //sprintf(stringArray[events], "RTU1: Button 1 pressed at: %s\n", timeString);
            //Obtener reporte
			sprintf(reporte[events], "RTU1 REPORT AT EVENT: Button 1 pressed at %s\nButton 1: %s, Button 2: %s,\nInterruptor 1: %s, Interruptor 2: %s,\nLed 1: %s, Led 2: %s,\nIot Led: %s,\nVoltage on line 1: %.2f\n", timeString, button1_state, button2_state, dip1_state, dip2_state, led1_state, led2_state, lediot_state, adc_voltage);
            events++;
            sem_post(&semaphore1);
        }
    }
}

void boton2(void)
{
    delay(20);
    if(digitalRead(BUTTON2) == LOW)
    {
        usleep(10000);
        if(digitalRead(BUTTON2) == LOW)
        {
            printf("RTU1: Button 2 pressed\n");
            fflush(stdout);
            sem_wait(&semaphore1);
            //sprintf(stringArray[events], "RTU1: Button 2 pressed at: %s\n", timeString);
            //Obtener reporte
			sprintf(reporte[events], "RTU1 REPORT AT EVENT: Button 2 pressed at %s\nButton 1: %s, Button 2: %s,\nInterruptor 1: %s, Interruptor 2: %s,\nLed 1: %s, Led 2: %s,\nIot Led: %s,\nVoltage on line 1: %.2f\n", timeString, button1_state, button2_state, dip1_state, dip2_state, led1_state, led2_state, lediot_state, adc_voltage);
            events++;
            sem_post(&semaphore1);
        }
    }
}

void dip1(void)
{
    delay(50);
    int current_state = digitalRead(DIP1);
    usleep(100000); // debounce adicional
    if(current_state == digitalRead(DIP1)) // Confirmación tras debounce
    {
        if(current_state != dip1_previous_state) // solo si realmente cambió
        {
            dip1_previous_state = current_state;

            if(current_state == LOW)
            {
                strcpy(dip1_state, "OFF");
                printf("RTU1: Switch 1 changed state to OFF\n");
            }
            else
            {
                strcpy(dip1_state, "ON");
                printf("RTU1: Switch 1 changed state to ON\n");
            }

            fflush(stdout);
            sem_wait(&semaphore1);
            //sprintf(stringArray[events], "RTU1: Switch 1 changed state to %s at: %s\n", dip1_state, timeString);
            //Obtener reporte
			sprintf(reporte[events], "RTU1 REPORT AT EVENT: Switch 1 changed state at %s\nButton 1: %s, Button 2: %s,\nInterruptor 1: %s, Interruptor 2: %s,\nLed 1: %s, Led 2: %s,\nIot Led: %s,\nVoltage on line 1: %.2f\n", timeString, button1_state, button2_state, dip1_state, dip2_state, led1_state, led2_state, lediot_state, adc_voltage);
            events++;
            sem_post(&semaphore1);
        }
    }
}

void dip2(void)
{
    delay(50);
    int current_state = digitalRead(DIP2);
    usleep(100000); // debounce adicional
    if(current_state == digitalRead(DIP2)) // Confirmación tras debounce
    {
        if(current_state != dip2_previous_state) // solo si realmente cambió
        {
            dip2_previous_state = current_state;

            if(current_state == LOW)
            {
                strcpy(dip2_state, "OFF");
                printf("RTU1: Switch 2 changed state to OFF\n");
            }
            else
            {
                strcpy(dip2_state, "ON");
                printf("RTU1: Switch 2 changed state to ON\n");
            }

            fflush(stdout);
            sem_wait(&semaphore1);
            //sprintf(stringArray[events], "RTU1: Switch 2 changed state to %s at: %s\n", dip2_state, timeString);
			
			//Obtener reporte
			sprintf(reporte[events], "RTU1 REPORT AT EVENT: Switch 2 changed state at %s\nButton 1: %s, Button 2: %s,\nInterruptor 1: %s, Interruptor 2: %s,\nLed 1: %s, Led 2: %s,\nIot Led: %s,\nVoltage on line 1: %.2f\n", timeString, button1_state, button2_state, dip1_state, dip2_state, led1_state, led2_state, lediot_state, adc_voltage);
            events++;
            sem_post(&semaphore1);
        }
    }
}


//*************FUNCIONES PARA MAPEAR*************//
int map(int value, int inMin, int inMax, int outMin, int outMax)
{
    return ((value - inMin) * (outMax - outMin))/(inMax - inMin) + outMin;
}
