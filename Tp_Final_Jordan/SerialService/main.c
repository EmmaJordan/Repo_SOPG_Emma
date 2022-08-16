#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "SerialManager.h"
#include "mySignals.h"

#define size 128

char bufferRx[size];
char bufferTx[size];

typedef struct
{
    int socketConectado;
}Data;

Data data;
pthread_mutex_t mutexData = PTHREAD_MUTEX_INITIALIZER;

volatile sig_atomic_t flagMain    = 0;

int cantBytes;
int fd_com;  //file descriptor para comunicarse Cliente y Servidor
void* serial_thread ( void* )
{
    if( serial_open(1,115200)!=0 )
	{
        perror("Error abriendo puerto serie\r\n");
        exit(1);
	}

    while(flagMain==0)
    {
        cantBytes = serial_receive(bufferRx,size);
        if(cantBytes>0 && cantBytes<255)
        {
            printf("Serial: recibí %d bytes: %s\r\n",cantBytes, bufferRx);
            // Enviamos mensaje a cliente
            if (write (fd_com, bufferRx, sizeof(bufferRx)) == -1)
            {
                perror("Serial: error escribiendo mensaje en socket");
                exit (1);
            }
        }
        usleep(100);
    }
    printf("Serial: cierro el puerto serie\r\n");
    serial_close();
    return NULL;
}

void* TCP_thread ( void* )
{
    //---------- Configuración TCP --------//
    // Según el tipo de socket se tienen distintos tipos de estructuras para inicializar
    // Para el caso de Internet Socket, sockaddr (ipv4) contiene IP+PUERTO
	socklen_t addr_len;
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;

	int n;              //bytes recibidos del cliente

	// Creamos socket de Internet (AF_INET), tipo Stream Socket (TCP)
    // fd_listen: file descriptor que escuchará conexiones Entrantes
	int fd_listen = socket(PF_INET,SOCK_STREAM, 0);

	// El socket fue creado pero aún no tiene asignada ninguna dirección
	// Cargamos dirección del server (IP+PUERTO) en estructura
    bzero( (char*) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;        //Internet Socket
    serveraddr.sin_port = htons(10000);      //Puerto donde escuchará el Server
    //htons: Host (mi procesador) to Network SHORT
    //serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //pton: Presentation (formato humano) to Network
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if(serveraddr.sin_addr.s_addr==INADDR_NONE)
    {
        fprintf(stderr,"Server TCP: error, invalid server IP\r\n");
        exit(1);
    }

	// Abrimos puerto con bind(),
	// Establece la DIRECCIÓN LOCAL del socket (IP+PUERTO)
	// para saber a qué socket establecer dirección, se le envía su fd
	if (bind(fd_listen, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1)
	{
		close(fd_listen);
		perror("Server TCP: bind");
		exit(1);
	}

	// Seteamos socket en modo Listening
	// listen() permite al socket escuchar conexiones entrantes
	if (listen (fd_listen, 10) == -1) // backlog=10
  	{
    	perror("Server TCP: error en listen");
    	exit(1);
  	}

  	//---------- Bucle TCP --------//
	while(flagMain==0)
    {
		addr_len = sizeof(struct sockaddr_in);
		printf  ("Server TCP: Esperando aceptar nuevo cliente...\n");

		//Intento aceptar conexión de nuevo Cliente
		pthread_mutex_lock(&mutexData);
		data.socketConectado = 0;
		pthread_mutex_unlock(&mutexData);
    	if ( (fd_com = accept(fd_listen, (struct sockaddr *)&clientaddr, &addr_len)) == -1)
      	{
		      perror("Server TCP: error en accept:");
		      exit(1);
	    }
	    else
	    {
            pthread_mutex_lock(&mutexData);
            data.socketConectado = 1;
            pthread_mutex_unlock(&mutexData);
            printf  ("Server TCP: conexion desde:  %s\n", inet_ntoa(clientaddr.sin_addr));
	    }

        while(data.socketConectado==1 && flagMain==0)
        {
            // --------- Leemos Mensaje del Cliente -------- //
            if( (n = read(fd_com,bufferTx,sizeof(bufferTx))) == -1 )
            {
                perror("TCP: Error leyendo mensaje en socket");
                exit(1);
            }
            bufferTx[n]=0;
            printf("TCP: Recibi %d bytes.:%s\n",n,bufferTx);

            if(n==0) //Se desconectó el ciente (ojo, mejor SIGNAL)
            {
                pthread_mutex_lock(&mutexData);
                data.socketConectado = 0;
                pthread_mutex_unlock(&mutexData);
                printf  ("TCP: Se desconectó el cliente!!\n");
            }
            else
            {
                serial_send(bufferTx,strlen(bufferTx));
            }
        }

		// Cerramos conexion con cliente
    	if(data.socketConectado==0)
    	{
            printf("Cierro conexion con cliente\r\n");
            close(fd_com);
    	}
	}
    // Cerramos conexion con cliente
    close(fd_com);
    // Cerramos lector de conexiones entrantes
    close(fd_listen);
    return NULL;
}

int main(void)
{
    printf("Inicio Serial Service\r\n");

    configuraSIGNALS();
    bloquearSIGNALS();      //Bloqueo señales antes de crear hilos

	pthread_t t1, t2;
	pthread_create(&t1,  NULL, serial_thread, NULL);
	pthread_create(&t2,  NULL, TCP_thread,    NULL);

    desbloquearSIGNALS();  //Desbloqueo señales después de crear hilos

    while(flagSIGINT==0 && flagSIGTERM==0)
    {
        sleep(0.1);
    }
    flagMain = 1;

    printf("Esperando que termine hilo Serial...\n");
    pthread_join(t1,NULL);

    printf("Esperando que termine hilo TCP...\n");
    pthread_cancel(t2);
    pthread_join(t2,NULL);

	exit(EXIT_SUCCESS);
	return 0;
}

