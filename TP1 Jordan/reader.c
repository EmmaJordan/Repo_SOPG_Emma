#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#define FIFO_NAME "miColaNombrada" //Mismo nombre que en writer
#define BUFFER_SIZE 300

int main(void)
{
    printf("TP 1 - SOPG - Jordán\n");
    printf("Soy el proceso LECTOR\n");

	char inputBuffer[BUFFER_SIZE];
	uint32_t bytesRead;
	int32_t returnCode, fd;

	//Archivos de texto
	FILE *f_log;
    f_log = fopen("log.txt","a+t");
    if(f_log==NULL)
    {
        printf("Error en la apertura del archivo log\n");
    }

    FILE *f_sig;
    f_sig = fopen("signals.txt","a+t");
    if(f_sig==NULL)
    {
        printf("Error en la apertura del archivo sig\n");
    }

    /* Create named fifo. -1 means already exists so no action if already exists */
    if ( (returnCode = mknod(FIFO_NAME, S_IFIFO | 0666, 0) ) < -1  ) //CREA LA COLA SI NO ESTÁ CREADA, NO LA ABRE
    {
        printf("Error creando la Cola Nombrada: %d\n", returnCode);
        exit(1);
    }

    /* Open named fifo. Blocks until other process opens it */
	printf("Esperando escritores...\n");
	if ( (fd = open(FIFO_NAME, O_RDONLY) ) < 0 ) //ABRIR SÓLO LECTURA
    {
        printf("Error abriendo la Cola Nombrada: %d\n", fd);
        exit(1);
    }
    /* open syscalls returned without error -> other process attached to named fifo */
	printf("Ya tengo un escritor\n");

    /* Loop until read syscall returns a value <= 0 */
	do
	{
        /* read data into local buffer */
        // LEO DE LA COLA NOMBRADA y guardo en inputBuffer
		if ((bytesRead = read(fd, inputBuffer, BUFFER_SIZE)) == -1)
        {
			perror("read");
        }
        else
		{
			inputBuffer[bytesRead] = '\0';
			printf("reader: read %d bytes: \"%s\"\n", bytesRead, inputBuffer);
			char miChar;
			if(inputBuffer[0]=='D')
			{
                for(uint32_t i=0; i<bytesRead; i++)
                {
                    miChar = inputBuffer[i];
                    fputc(miChar,f_log);
                }
			}
			else if(inputBuffer[0]=='S')
			{
                for(uint32_t i=0; i<bytesRead; i++)
                {
                    miChar = inputBuffer[i];
                    fputc(miChar,f_sig);
                }
			}
            fputc('\n',f_log);
            fputc('\n',f_sig);
		}
	}
	while (bytesRead > 0); //sale cuando bytesRead es igual a 0
	fclose(f_log);
	fclose(f_sig);
	return 0;
}
