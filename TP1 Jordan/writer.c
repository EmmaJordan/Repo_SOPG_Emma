#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>

#define FIFO_NAME "miColaNombrada"
#define BUFFER_SIZE 300

//Buena práctica utilizar un flag de este tipo de datos dentro del Handler
volatile sig_atomic_t flagWriter;
//int returnWriter = 0;

void signalPIPE_Handler(int sig)
{
    //Buena práctica utilizar write en lugar de printf, es una función más segura,
    //la de más bajo nivel para imprimir caracteres por consola
    //Hay que indicarle el número de bytes exactos, incluído el \n.
    //El primer parámetro 1 significa stdout
	write(1,"ESCRITOR: SIGPIPE!! READER TERMINO!!\n",37);

	//wait del padre para capturar el retorno del hijo cuando termina,
	//para que no quede zombie
	//wait(&returnWriter);

	//flag seguro
	flagWriter = 1;
}

void signalSIGUSR1_Handler(int sig)
{
    write(1,"ESCRITOR: SIGUSR1!!\n",20);
}

void signalSIGUSR2_Handler(int sig)
{
    write(1,"ESCRITOR: SIGUSR2!!\n",20);
}

void configuraSIGPIPE( void );
void configuraSISUSR1( void );
void configuraSIGUSR2( void );
void configuraSIGNALS( void );

int main(void)
{
    printf("TP 1 - SOPG - Jordán\n");
    printf("Soy el proceso ESCRITOR\n");

    // --------- SEÑALES ---------- //
    configuraSIGNALS();

    char outputBuffer[BUFFER_SIZE];
	uint32_t bytesWrote;
	int32_t returnCode, fd;

    /* Create named fifo. -1 means already exists so no action if already exists */
    if ( (returnCode = mknod(FIFO_NAME, S_IFIFO | 0666, 0) ) < -1 ) //CREA LA COLA SI NO ESTÁ CREADA, NO LA ABRE
    {
        printf("Error creando la Cola Nombrada: %d\n", returnCode);
        exit(1);
    }

    /* Open named fifo. Blocks until other process opens it */
	printf("Esperando lectores...\n");
	if ( (fd = open(FIFO_NAME, O_WRONLY) ) < 0 ) //ABRIR SÓLO ESCRITURA
    {
        printf("Error abriendo la cola nombrada: %d\n", fd);
        exit(1);
    }

    /* open syscalls returned without error -> other process attached to named fifo */
	printf("Ya tengo un lector, escribí algo acá: \n");

    /* Loop forever */
	while (flagWriter == 0)
	{
        /* Get some text from console */
        fgets(outputBuffer, BUFFER_SIZE, stdin);
        /* Write buffer to named fifo. Strlen - 1 to avoid sending \n char */
		// ESCRIBO EN LA COLA NOMBRADA
		if ((bytesWrote = write(fd, outputBuffer, strlen(outputBuffer)-1)) == -1)
        {
			perror("Escritor error: ");
			break;
        }
        else
        {
			printf("Escritor: escriste %d bytes\n", bytesWrote);
        }
	}
	printf("Escritor: me cierro\n");
	return 0;
}

void configuraSIGPIPE( void )
{
    struct sigaction sa;                    //Estructura para configuración de Handlers de Señales

	/* Campos de la estructura */
	sa.sa_handler = signalPIPE_Handler;     //Nombre del Handler
	//sa.sa_flags = SA_RESTART;             //Flags de comportamiento de las syscalls interrumpidas
	sa.sa_flags = 0;
	//sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	/* Función que configura el Handler mediante su estructura */
	if (sigaction(SIGPIPE,&sa,NULL) == -1)     //(signal,&actual_struct, &older_struct)
    {
        perror("sigaction error\n");
        exit(1);
    }
    //Como configuramos SIGPIPE, va a entrar al handler cuando se cierra el reader
}

void configuraSIGUSR1( void )
{
    struct sigaction sa;                    //Estructura para configuración de Handlers de Señales

    // ---- SIGPIPE
	/* Campos de la estructura */
	sa.sa_handler = signalSIGUSR1_Handler;     //Nombre del Handler
	//sa.sa_flags = SA_RESTART;             //Flags de comportamiento de las syscalls interrumpidas
	sa.sa_flags = 0;
	//sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	/* Función que configura el Handler mediante su estructura */
	if (sigaction(SIGUSR1,&sa,NULL) == -1)     //(signal,&actual_struct, &older_struct)
    {
        perror("sigaction error\n");
        exit(1);
    }
}

void configuraSIGUSR2( void )
{
    struct sigaction sa;                    //Estructura para configuración de Handlers de Señales

    // ---- SIGPIPE
	/* Campos de la estructura */
	sa.sa_handler = signalSIGUSR2_Handler;     //Nombre del Handler
	//sa.sa_flags = SA_RESTART;             //Flags de comportamiento de las syscalls interrumpidas
	sa.sa_flags = 0;
	//sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	/* Función que configura el Handler mediante su estructura */
	if (sigaction(SIGUSR2,&sa,NULL) == -1)     //(signal,&actual_struct, &older_struct)
    {
        perror("sigaction error\n");
        exit(1);
    }
}

void configuraSIGNALS( void )
{
    configuraSIGPIPE();
    configuraSIGUSR1();
    configuraSIGUSR2();
}
