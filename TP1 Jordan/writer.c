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
volatile sig_atomic_t flagPIPE, flagSIGUSR1, flagSIGUSR2, flagSIGNALS;

void signalPIPE_Handler(int sig)
{
    //Buena práctica utilizar write en lugar de printf, es una función más segura,
    //la de más bajo nivel para imprimir caracteres por consola
    //Hay que indicarle el número de bytes exactos, incluído el \n.
    //El primer parámetro 1 significa stdout
	write(1,"ESCRITOR: SIGPIPE!! READER TERMINO!!\n",37);
	flagSIGNALS = 1;
	flagPIPE = 1;
}

void signalSIGUSR1_Handler(int sig)
{
    //write(1,"ESCRITOR: SIGUSR1!!\n",20);
    flagSIGNALS = 1;
    flagSIGUSR1 = 1;
}

void signalSIGUSR2_Handler(int sig)
{
    //write(1,"ESCRITOR: SIGUSR2!!\n",20);
    flagSIGNALS = 1;
    flagSIGUSR2 = 1;
}

void configuraSIGPIPE( void );
void configuraSISUSR1( void );
void configuraSIGUSR2( void );
void configuraSIGNALS( void );

int main(void)
{
    // ------- SEÑALES ---------- //
    configuraSIGNALS();

    printf("TP 1 - SOPG - Jordán\n");
    printf("Soy el proceso ESCRITOR\n");

    char outputBuffer[BUFFER_SIZE];
    char dataBuffer[6] = "DATA:";
    char sigBuffer [7] = "SIGN:?";
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
    flagPIPE = 0;
	while (flagPIPE == 0)
	{
        /* Get some text from console */
        flagSIGNALS = 0;
        while(flagSIGNALS==0)
        {
            fgets(outputBuffer, BUFFER_SIZE, stdin);
            if ( outputBuffer[0] == -1)
            {
                perror("Writer: Error en fgets");
                return -1;
            }
            //printf("Writer: fgetsOK = %d\n",fgetsOK);
            break;
        }

        /* Write buffer to named fifo. Strlen - 1 to avoid sending \n char */
		if(flagSIGNALS==0)
		{
            // ESCRIBO EN LA COLA NOMBRADA
            if ((bytesWrote = write(fd, dataBuffer, strlen(dataBuffer))) == -1)
            {
                perror("Escritor error:");
                break;
            }
            if ((bytesWrote = write(fd, outputBuffer, strlen(outputBuffer)-1)) == -1)
            {
                perror("Escritor error:");
                break;
            }
            else
            {
                printf("Escribiste %d bytes\n", bytesWrote);
            }
        }
        else
        {
            flagSIGNALS = 0;
            if(flagPIPE)
            {
                //flagPIPE = 0;
                write(fd, "PIPE", 4);
            }
            if(flagSIGUSR1)
            {
                flagSIGUSR1 = 0;
                sigBuffer[5] = '1';
                if ((bytesWrote = write(fd, sigBuffer, strlen(sigBuffer))) == -1)
                {
                    perror("Escritor error:");
                    break;
                }
                else
                {
                    printf("Escribiste %d bytes\n", bytesWrote);
                }
                //write(fd, "SIGN:1", 7);
            }
            if(flagSIGUSR2)
            {
                flagSIGUSR2 = 0;
                sigBuffer[5] = '2';
                if ((bytesWrote = write(fd, sigBuffer, strlen(sigBuffer))) == -1)
                {
                    perror("Escritor error:");
                    break;
                }
                else
                {
                    printf("Escribiste %d bytes\n", bytesWrote);
                }
                //write(fd, "SIGN:2", 7);
            }
        }
	}

	printf("Me cierro\n");
	return 0;
}

void configuraSIGPIPE( void )
{
    flagPIPE = 0;
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
    flagSIGUSR1 = 0;
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
    flagSIGUSR2 = 0;
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
