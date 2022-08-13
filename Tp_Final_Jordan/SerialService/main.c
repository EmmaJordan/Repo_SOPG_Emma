

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "SerialManager.h"

//Buena práctica utilizar un flag de este tipo de datos dentro del Handler
volatile sig_atomic_t flagSIGINT;

void SIGINT_Handler(int sig)
{
    //Buena práctica utilizar write en lugar de printf, es una función más segura,
    //la de más bajo nivel para imprimir caracteres por consola
    //Hay que indicarle el número de bytes exactos, incluído el \n.
    //El primer parámetro 1 significa stdout
	write(1,"SIGINT!!\n",9); //Aviso por terminal
	flagSIGINT = 1;
}


void configuraSIGINT ( void );
void configuraSIGNALS( void );

int main(void)
{
    printf("Inicio Serial Service\r\n");

    configuraSIGNALS();

    int X=0;
	int Y=0;
	uint8_t size = 128;
	char bufferTx[size];
	char bufferRx[size];

	if( serial_open(1,115200)!=0 )
	{
        printf("Error abriendo puerto serie\r\n");
        return -1;
	}


	uint8_t cantBytes;
    flagSIGINT = 0;
	while(flagSIGINT==0)
	{
        if(Y==0)        Y = 1;
        else if(Y==1)   Y = 0;

        snprintf(bufferTx,sizeof(bufferTx),">OUT:%d,%d\r\n",X,Y);
        printf("Envio: %s\r\n",bufferTx);
        serial_send(bufferTx,strlen(bufferTx));

        cantBytes = serial_receive(bufferRx,size);
        if(cantBytes>0 && cantBytes<255)
        {
            printf("Recibí %d bytes\r\n",cantBytes);
            printf("Recibí: %s\r\n",bufferRx);
        }
        sleep(1);
	}
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
