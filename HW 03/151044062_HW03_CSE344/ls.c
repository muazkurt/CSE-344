#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#define R 1
#define A 2
int is_dir(char *);
int ls(char * dest, int bytes);

char ** parse(char * input, char parser)
{
    char ** result    = 0;
    size_t count     = 0;
    char * tmp        = input;
    char * temp2 = 0;
    char delim[2];
    delim[0] = parser;
    delim[1] = 0;
    while (*tmp)
    {
        if (parser == *tmp)
        {
            count++;
            temp2 = tmp;
        }
        tmp++;
    }

    count += temp2 < (input + strlen(input) - 1);

    if((result = (char **) malloc(sizeof(char *) * ++count)) != NULL)
    {
        size_t ret_init  = 0;
        char * token = strtok(input, delim);
        do
            *(result + ret_init++) = strdup(token);
        while (token = strtok(NULL, delim));
        *(result + ret_init) = 0;
    }

    return result;
}

int main(int argc, char ** argv)
{
    char ** args;
    if((args = parse(argv[1], ' ')) != NULL)
    {
        ls(*args, R|A);
        int i;
        for (i = 0; *(args + i); i++)
            free(*(args + i));
        free(args);
    }

}

int ls(char * dest, int bytes)
{
    char temp[255];
    int anding = 0; 
    struct stat workon;
    
    if(stat(dest, &workon) == -1)
        perror("No such file or directory");
    if(S_ISDIR(workon.st_mode))
    {
        DIR *dp;
        struct dirent *ep;
        if ((dp = opendir (dest)) != NULL)
        {
            write(STDOUT_FILENO, dest, strlen(dest));
            write(STDOUT_FILENO, ":\n", 2);
            while (ep = readdir (dp))
            {
                sprintf(temp, "%s/%s", dest, ep->d_name);
                if(stat(temp, &workon) == -1)
                {
                    perror("failed to get file stat");
                    return -1;
                }
                if(S_ISDIR(workon.st_mode) > 0)
                {
                    anding = (strncmp(ep->d_name, ".", 1) != 0 && strncmp(ep->d_name, "..", 1) != 0);
                    if(bytes & R && anding) ls(temp, bytes);
                    else if(bytes & A)
                        for(char * k = ep->d_name; *k != '\0'; ++k)
                            write(STDOUT_FILENO, k, 1);
                    else if(anding)
                        for(char * k = ep->d_name; *k != '\0'; ++k)
                            write(STDOUT_FILENO, k, 1);
                }
                else
                {
                    for(char * k = ep->d_name; *k != '\0'; ++k)
                        write(STDOUT_FILENO, k, 1);
                }
                write(STDOUT_FILENO, " ", 1);
                *temp = 0;
            }
            write(STDOUT_FILENO, "\n", 1);
            (void) closedir (dp);
        }
        else
        perror ("Couldn't open the directory");
    }
    write(STDOUT_FILENO, "\n", 1);
    return 0;
}



int is_dir(char * path)
{
    struct stat workon;
    if(stat(path, &workon) == -1)
        perror("error");
    return S_ISDIR(workon.st_mode);
}
