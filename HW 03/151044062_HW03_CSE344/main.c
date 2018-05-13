#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h> 
#include <termios.h> 
#include <string.h>
#ifndef PATH_MAX
    #define PATH_MAX 255
#endif


#define ECHOFLAGS (ECHO | ECHOE | ECHOK | ECHONL)
#define FIFO_PERM (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define ERASAE_BUTTON 127
#define ERASE   "\010 \010"
#define ESCAPE  '\033'
#define ESC_2   '\133'
#define UP      '\101'
#define DOWN    '\102'
#define PWD     "pwd"
#define LS      "ls"
#define CD      "cd"
#define CAT     "cat"
int read_line(char * buffer);

int get_command(char ** input, int current_p);

int monitor(int fd);

char ** parse(char * input, char parser);

void childWork(char * args, int input, int output);

char * get_filename(char * input);

void clear_dums(char ** args);

int parsed_work(char * input);
    
int pwd();

void history_show(char ** arr);

void help_show();

int main()
{
    char ** history,
         termbuf[L_ctermid];
    int i = 0,
        fd = 0;
    struct termios old;

    history = (char **) malloc(sizeof(char *) * 100);    
    if (ctermid(termbuf) == NULL) 
    {
        perror("Failed to get terminal name");
        errno = ENODEV;
        exit(EXIT_FAILURE);
    }
    if ((fd = open(termbuf, O_RDWR)) == -1)
    {
        perror("Openning terminal");
        exit(EXIT_FAILURE);
    }
    tcgetattr(fd, &old);
    monitor(fd);

    while(get_command(history, i))
    {
        if(**(history + i) == '\0');
        else 
        {
            if(strcmp(*(history + i), "history") == 0)
                history_show(history);
            else if(strcmp(*(history + i), "help") == 0)
                help_show();
            else if(strncmp(*(history + i), PWD, 3) == 0 ||
                    strncmp(*(history + i), CAT, 3) == 0 ||
                    strncmp(*(history + i), LS, 2) == 0)
                parsed_work(*(history + i));                
            else
                write(STDERR_FILENO, "UNKNOWN COMMAND\n", 17);
            ++i;
        }
    }
    ++i;
    while(i > 0)
        free(*(history + --i));
    free(history);
    history = NULL;
    tcsetattr(fd, TCSANOW, &old);
    close(fd);
    return 0;
}

void help_show()
{
    printf("cat (<, >) : girdiyi iletir.\n");
    printf("help : bilgi verir.\n");
    printf("ls dirname : dirname ve alt klasorleri listeler.\n");
    printf("pwd : bulunulan klasor bilgisi.\n");
}

void history_show(char ** arr)
{
    char buffer[PATH_MAX];
    for(int i = 0; i < 100 && *(arr + i) != NULL; ++i)
    {
        sprintf(buffer, "%d: %s\n", i, *(arr + i));
        write(STDOUT_FILENO, buffer, strlen(buffer));
    }
}

int pwd()
{
    char working_dir[PATH_MAX];
    if(getcwd(working_dir, PATH_MAX) == NULL)
        perror("pwd error.");
    return write(STDOUT_FILENO, working_dir, strlen(working_dir));
    
}

int get_command(char ** input, int current_p)
{
    pwd();
    write(STDOUT_FILENO, ": ", 2);
    *(input + current_p) = (char *) malloc(sizeof(char) * 100);
    *(*(input + current_p)) = 0;
    for(int i = 0, k = 0; k = read_line(*(input + current_p));)
    {
        if(i + k > 0 || i + k + current_p < 0);
        else
        {
            for(int x = 0; *(*(input + current_p) + x) != 0; ++x)
                write(STDOUT_FILENO, ERASE, strlen(ERASE));
            if((i += k) == 0)
                **(input + current_p) = 0;
            else
                strcpy(*(input + current_p), *(input + i + current_p));
            write(STDOUT_FILENO, *(input + current_p), strlen(*(input + current_p)));
        }
    }
    if(strncmp(*(input +  current_p), "exit()", 6) == 0)
        return 0;
    return 1;
}

int monitor(int fd) 
{
    int error;
    struct termios term;
    if (tcgetattr(fd, &term) == -1)
        return -1;
    else
    {
        term.c_iflag |= IUTF8;        
        term.c_lflag &= ~ECHOFLAGS;
        term.c_lflag &= ~ICANON;
        term.c_cc[VMIN] = 1;
        term.c_cc[VTIME] = 0;
        term.c_cc[VERASE] = ERASAE_BUTTON;

    }
    while (((error = tcsetattr(fd, TCSANOW, &term)) == -1) && (errno == EINTR)) ;
    return error;
}

int read_line(char * buffer)
{
    int     i           = strlen(buffer);
    char    escaping    = 0;
    while(read(STDIN_FILENO, buffer + i, 1))
    {
        if(*(buffer + i) == ERASAE_BUTTON && i > 0)
            write(STDOUT_FILENO, ERASE, 3), --i;
        else if(*(buffer + i) == ESCAPE)
        {
            *(buffer + i) = 0;
            read(STDIN_FILENO, &escaping, 1);
            if(escaping == ESC_2)
            {
                read(STDIN_FILENO, &escaping, 1);
                switch(escaping)
                {
                    case UP:
                        return -1;
                        break;
                    case DOWN:
                        return 1;
                        break;
                    default:
                        write(STDERR_FILENO, " UNKNOWN\n", 10);
                        break;
                }
            }
        }
        else if(*(buffer + i) == '\12')
        {
            write(STDOUT_FILENO, "\n", 1);
            *(buffer + i) = 0;
            return 0;
        }
        else 
            write(STDOUT_FILENO, buffer + i++, 1);
    }
    return 0;
}

int parsed_work(char * input)
{
    int i, child, flag = 0;    
    char ** tokens;
    char ** temp;
    char * args;
    char carrier[PATH_MAX];
    int pd[2];
    sprintf(carrier, "./q%s", input);
    tokens = parse(carrier, '|');
    pipe(pd);
    for (i = 0; (args = *(tokens + i)); i++)
    {
        temp = parse(args, '>');
        if(*(temp + 1) != NULL)
        {
            flag = 1;
            dup2(open(get_filename(*(temp + 1)), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU), pd[1]);
        }
        clear_dums(temp);
        if(flag == 0)
        {
            temp = parse(args, '<');
            if(*(temp + 1) != NULL)
                dup2(open(get_filename(*(temp + 1)), O_RDONLY | O_CREAT, S_IRWXU), pd[1]);
            flag = 0;
            clear_dums(temp);
        }
        if((child = fork()) == -1)
            perror("!");
        else if(child == 0)
        {
            if(i % 2 == 0)
            {
                close(pd[0]);
                childWork(args, STDIN_FILENO, pd[1]);
            }
            else
            {
                close(pd[1]);                    
                childWork(args, pd[0], STDOUT_FILENO);
            }
        }
    }
    close(pd[1]);
    clear_dums(tokens);    
    if(i == 1)
    {
        i = '\n';
        do{
            write(STDOUT_FILENO, &i, 1);
        }
        while(read(pd[0], &i, 1));
    }
    else
        while(wait(NULL) > 0);    
    close(pd[0]);
    return 0;
}

void clear_dums(char ** args)
{
    int i;
    for (i = 0; *(args + i); i++)
        free(*(args + i));
    free(args);
    args = NULL;
}

char * get_filename(char * input)
{
    int k;
    char * filedest;
    for(filedest = input; *filedest == ' '; ++filedest);
    for(k = 0; *(filedest + k) != 0 && *(filedest + k) != ' ' ; ++k);
    *(filedest + k + 1) = 0;
    return filedest;
}

void childWork(char * args, int input, int output)
{
    char * a = args;
    while(*a == ' ')
        a = ++args;
    while(*a != ' ' && *a != 0)
        ++a;
    *(a++) = 0;
    if(strlen(a) == 0) a = NULL;
    dup2(input, STDIN_FILENO);
    dup2(output, STDOUT_FILENO);
    execl(args, args, a, NULL);
    perror(a);
    _exit(EXIT_FAILURE);
}


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
