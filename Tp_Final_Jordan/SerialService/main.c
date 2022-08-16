

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
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
void* serial_thread ( void* )
{
    while(flagMain==0)
    {
        cantBytes = serial_receive(bufferRx,size);
        if(cantBytes>0 && cantBytes<255)
        {flagSIGINT
            printf("Recibí %d bytes: %s\r\n",cantBytes, bufferRx);
        }
        sleep(0.1);
    }
    return NULL;
}

void* TCP_thread ( void* )
{
    /*int cont = 0;
    char bufferTx[size];
    while(flagMain==0)
    {
        int recibiPaqueteTCP = 0;
        if(recibiPaqueteTCP==1)
        {
            // ----- Leer paquete TCP ----- //
            //guardar en bufferRx

            // ---- Reenviar a Emulador ---//
            //snprintf(bufferTx,sizeof(bufferTx),">OUT:%d,%d\r\n",X,Y);
            printf("Recibí y Envío: %s\r\n",bufferRx);
            serial_send(bufferTx,strlen(bufferTx));
        }
        //printf("Hilo TCP %d\r\n",cont++);

    }*/
    sleep(0.1);
    return NULL;
}

int main(void)
{
    printf("Inicio Serial Service\r\n");

    configuraSIGNALS();
    bloquearSIGNALS();  //Bloqueo señales antes de crear hilos

	if( serial_open(1,115200)!=0 )
	{
        perror("Error abriendo puerto serie\r\n");
        return -1;
	}

	pthread_t t1, t2;
	pthread_create(&t1,  NULL, serial_thread, NULL);
	pthread_create(&t2,  NULL, TCP_thread,    NULL);

    desbloquearSIGNALS();  //Desbloqueo señales después de crear hilos

    while(flagSIGINT==0)
    {
        sleep(0.1);
    }
    flagMain = 1;

    pthread_join(t1,NULL);
    pthread_join(t2,NULL);

	printf("Me voy, cierro el puerto serie\r\n");
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
    pthread_sigmask(SIG_BLOCK, &set, NULL);
}

void desbloquearSIGNALS()
{
    sigset_t set;
    //int s;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}
