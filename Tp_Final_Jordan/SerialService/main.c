

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

#define size 128

//Buena práctica utilizar un flag de este tipo de datos dentro del Handler
volatile sig_atomic_t flagSIGINT = 0;
volatile sig_atomic_t flagMain   = 0;

void SIGINT_Handler(int sig)
{
    //Buena práctica utilizar write en lugar de printf, es una función más segura,
    //la de más bajo nivel para imprimir caracteres por consola
    //Hay que indicarle el número de bytes exactos, incluído el \n.
    //El primer parámetro 1 significa stdout
	write(1,"\nCtrl+C detected!!\n",19); //Aviso por terminal
	flagSIGINT = 1;
}


void configuraSIGINT    ( void );
void configuraSIGNALS   ( void );
void bloquearSIGNALS    ( void );
void desbloquearSIGNALS ( void );

char bufferRx[size];
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
            printf("Recibí %d bytes: %s\r\n",cantBytes, bufferRx);
            // Enviamos mensaje a cliente
                if (write (fd_com, bufferRx, sizeof(bufferRx)) == -1)
                {
                    perror("Error escribiendo mensaje en socket");
                    exit (1);
                }
        }
        sleep(0.1);
    }
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
	char bufferTx[128];
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
        fprintf(stderr,"ERROR invalid server IP\r\n");
        exit(1);
    }

	// Abrimos puerto con bind(),
	// Establece la DIRECCIÓN LOCAL del socket (IP+PUERTO)
	// para saber a qué socket establecer dirección, se le envía su fd
	if (bind(fd_listen, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1)
	{
		close(fd_listen);
		perror("listener: bind");
		exit(1);
	}

	// Seteamos socket en modo Listening
	// listen() permite al socket escuchar conexiones entrantes
	if (listen (fd_listen, 10) == -1) // backlog=10
  	{
    	perror("error en listen");
    	exit(1);
  	}

  	//---------- Bucle TCP --------//
  	int socketConectado = 0;
	while(flagMain==0)
    {
		addr_len = sizeof(struct sockaddr_in);
		printf  ("Esperando aceptar nuevo cliente...\n");

		//Intento aceptar conexión de nuevo Cliente
		socketConectado = 0;
    	if ( (fd_com = accept(fd_listen, (struct sockaddr *)&clientaddr, &addr_len)) == -1)
      	{
		      perror("error en accept");
		      exit(1);
	    }
	    else
	    {
            socketConectado = 1;
            printf  ("server:  conexion desde:  %s\n", inet_ntoa(clientaddr.sin_addr));
	    }

        while(socketConectado==1 && flagMain==0)
        {
            // --------- Leemos Mensaje del Cliente -------- //
            if( (n = read(fd_com,bufferTx,sizeof(bufferTx))) == -1 )
            {
                perror("Error leyendo mensaje en socket");
                exit(1);
            }
            bufferTx[n]=0;
            printf("Recibi %d bytes.:%s\n",n,bufferTx);
            if(n==0) //Se desconectó el ciente (ojo, mejor SIGNAL)
            {
                socketConectado = 0;
                printf  ("Se desconectó el cliente :( \n");
            }
            else
            {
                serial_send(bufferTx,strlen(bufferTx));
            }
        }
		// Cerramos conexion con cliente
    	if(socketConectado==0)
    	{
            printf("Cierro conexion con cliente\r\n");
            close(fd_com);
    	}
	}
    // Cerramos conexion con cliente
    close(fd_com);
    // Cerramos lector
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

    while(flagSIGINT==0)
    {
        sleep(0.1);
    }
    flagMain = 1;

    printf("Esperando que termine hilo Serial...\n");
    pthread_join(t1,NULL);

    printf("Esperando que termine hilo TCP...\n");
    pthread_join(t2,NULL);

	printf("Cierro el puerto serie y salgo.\r\n");
    serial_close();
	exit(EXIT_SUCCESS);
	return 0;
}

void configuraSIGINT( void )
{
    flagSIGINT = 0;
    struct sigaction sa;                    //Estructura para configuración de Handlers de Señales

	/* Campos de la estructura */
	sa.sa_handler = SIGINT_Handler;     //Nombre del Handler
	//sa.sa_flags = SA_RESTART;             //Flags de comportamiento de las syscalls interrumpidas
	sa.sa_flags = 0;
	//sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	/* Función que configura el Handler mediante su estructura */
	if (sigaction(SIGINT,&sa,NULL) == -1)     //(signal,&actual_struct, &older_struct)
    {
        perror("sigaction error\n");
        exit(1);
    }
    //Como configuramos SIGINT, va a entrar al handler cuando se envía Ctrl+C
}

void configuraSIGNALS( void )
{
    configuraSIGINT();
}

void bloquearSIGNALS()
{
    sigset_t set;
    //int s;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    if ( pthread_sigmask(SIG_BLOCK, &set, NULL) == -1 )
    {
        printf("Error creando mascara de bloqueo de hilos: ");
        exit(1);
    }
}

void desbloquearSIGNALS()
{
    sigset_t set;
    //int s;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    if ( pthread_sigmask(SIG_UNBLOCK, &set, NULL) == -1 )
    {
        perror("Error creando máscara de bloqueo de hilos: ");
        exit(1);
    }
}
