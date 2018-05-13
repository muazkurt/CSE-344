#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#ifndef PATH_MAX
    #define PATH_MAX 255
#endif

int pwd(int pipe_write);

int main(int argc, char ** argv)
{
    pwd(1);
}


int pwd(int pipe_write)
{
    int error = 0;
    char * buffer;
    buffer = (char *) malloc(PATH_MAX * sizeof(char));
    if(getcwd(buffer, PATH_MAX) == NULL)
        perror("errno");

    else
        write(STDOUT_FILENO, buffer, strlen(buffer));
    free(buffer);
    buffer = NULL;
    return error;
}


