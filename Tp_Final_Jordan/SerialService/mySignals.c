#include "mySignals.h"

volatile sig_atomic_t flagSIGINT  = 0;
volatile sig_atomic_t flagSIGTERM = 0;

void SIGINT_Handler(int sig)
{
    //Buena práctica utilizar write en lugar de printf, es una función más segura,
    //la de más bajo nivel para imprimir caracteres por consola
    //Hay que indicarle el número de bytes exactos, incluído el \n.
    //El primer parámetro 1 significa stdout
	write(1,"\nCtrl+C detected!!\n",19); //Aviso por terminal
	flagSIGINT = 1;
}

void SIGTERM_Handler(int sig)
{
	write(1,"\nSIGTERM detected!!\n",20); //Aviso por terminal
	flagSIGTERM = 1;
}

void configuraSIGNALS( void )
{
    configuraSIGINT();
    configuraSIGTERM();
}

void bloquearSIGNALS()
{
    sigset_t set;
    //int s;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    if ( pthread_sigmask(SIG_BLOCK, &set, NULL) == -1 )
    {
        perror ("Error creando mascara de bloqueo de hilos: ");
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

void configuraSIGTERM( void )
{
    flagSIGTERM = 0;
    struct sigaction sa;                    //Estructura para configuración de Handlers de Señales

	/* Campos de la estructura */
	sa.sa_handler = SIGTERM_Handler;     //Nombre del Handler
	//sa.sa_flags = SA_RESTART;             //Flags de comportamiento de las syscalls interrumpidas
	sa.sa_flags = 0;
	//sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	/* Función que configura el Handler mediante su estructura */
	if (sigaction(SIGTERM,&sa,NULL) == -1)     //(signal,&actual_struct, &older_struct)
    {
        perror("sigaction error\n");
        exit(1);
    }
    //Como configuramos SIGTERM, va a entrar al handler cuando se envía Ctrl+C
}

