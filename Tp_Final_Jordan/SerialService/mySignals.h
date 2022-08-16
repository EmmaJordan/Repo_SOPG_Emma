#ifndef MYSIGNALS_H_INCLUDED
#define MYSIGNALS_H_INCLUDED

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

//Buena práctica utilizar un flag de este tipo de datos dentro del Handler
extern volatile sig_atomic_t flagSIGINT;
extern volatile sig_atomic_t flagSIGTERM;

// -------- Handlers ------- //
void SIGINT_Handler(int sig);
void SIGTERM_Handler(int sig);

// -------- Auxiliares ------- //
void configuraSIGINT    ( void );
void configuraSIGTERM   ( void );
void configuraSIGNALS   ( void );
void bloquearSIGNALS    ( void );
void desbloquearSIGNALS ( void );


#endif // MYSIGNALS_H_INCLUDED
