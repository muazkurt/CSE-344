#include "header/both.h"

int main(int argc, char ** argv)
{
    int     pid         = 0,
            M           = 0,
            N           = 0,
            logdest     = 0;
    char *  filedest    = NULL;
    struct sigaction sa;
    if(argc != INPUT)
    {
        write(STDERR_FILENO, argv[0], strlen(argv[0]));
        write(STDERR_FILENO, " -N <number of signals> -X <R&W filedest> -M <Count of moves>\n", 62);
        return -1;
    }
    parsearg(&argv[1], &N, &filedest, &M);
    update_sign(&sa);
    while((logdest = open(LOG_FILE, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU)) == -1)
        if(errno != EINTR)
        {
            perror("Logfile");
            _exit(EXIT_FAILURE);
        }

    if((pid = fork()) == -1)
    {
        perror("fork.");
        _exit(EXIT_FAILURE);
    }
    else if(pid == 0)
    {
        child(filedest, N, logdest);
        exit(EXIT_SUCCESS);
    }
    srand(time(NULL));
    parent(filedest, N, M, pid, logdest);
    kill(pid, SIGINT);
    wait(NULL);
    close(logdest);
    return 0;
}