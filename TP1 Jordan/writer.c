#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#define FIFO_NAME "miColaNombrada"
#define BUFFER_SIZE 300

int main(void)
{
    printf("TP 1 - SOPG - Jordán\n");
    printf("Soy el proceso ESCRITOR\n");

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
	while (1)
	{
        /* Get some text from console */
		fgets(outputBuffer, BUFFER_SIZE, stdin);

        /* Write buffer to named fifo. Strlen - 1 to avoid sending \n char */
		// ESCRIBO EN LA COLA NOMBRADA
		if ((bytesWrote = write(fd, outputBuffer, strlen(outputBuffer)-1)) == -1)
        {
			perror("write");
        }
        else
        {
			printf("Escritor: escriste %d bytes\n", bytesWrote);
        }
	}
	return 0;
}
