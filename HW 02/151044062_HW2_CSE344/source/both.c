#include "../header/both.h"


volatile sig_atomic_t ctrlc = 0;

void cchandler(int signo)
{
	ctrlc = -1;
}

void update_sign(struct sigaction * sa)
{
    sigemptyset(&(sa->sa_mask));
    sa->sa_flags = 0;
    sa->sa_handler = cchandler;
    if (sigaction(SIGINT, sa, NULL) == -1)
        perror("sigaction"), _exit(EXIT_FAILURE);
}



int parsearg(char ** argv, int * const N, char ** filedest, int * const M)
{
    int i = 0;
    while(i < INPUT - 2)
    {
        if(strcmp(argv[i], "-X") == 0 || strcmp(argv[i], "-x") == 0)
            *filedest = argv[i + 1];
        else if(strcmp(argv[i], "-M") == 0 || strcmp(argv[i], "-m") == 0)
            sscanf(argv[i + 1], "%d", M);
        else if(strcmp(argv[i], "-N") == 0 || strcmp(argv[i], "-n") == 0)
            sscanf(argv[i + 1], "%d", N);
        else
        {
            perror("Unknown format");
            _exit(EXIT_FAILURE);
        }
        i += 2;
    }
    return 0;
}


int block(int fd)
{
    struct flock lock;
    lock.l_type     = F_WRLCK;
    lock.l_whence   = SEEK_SET;
    lock.l_len      = 0;
    lock.l_start    = 0;
    if(fcntl(fd, F_SETLKW, &lock) == -1)
        perror("fileblock");
    return-1;
}

int unblock(int fd)
{
    struct flock lock;
    lock.l_type     = F_UNLCK;
    lock.l_whence   = SEEK_SET;
    lock.l_len      = 0;
    lock.l_start    = 0;
    return fcntl(fd, F_SETLKW, &lock);
}

//child
    int child(char * filedest, int N, int logdest)
    { 
        int         fd                  = 0     ,
                    linenum             = 0     ,
                    cached              = 0     ;
        double *    calc_area           = NULL  ;
        char        buffer[MAX_BUFFER]  = ""    ;
        Complex*    result              = NULL  ;
        sigset_t    toblock                     ;
        calc_area   = (double  *) malloc(sizeof(double)  * N);
        result      = (Complex *) malloc(sizeof(Complex) * N);
        while((fd = open(filedest, O_RDWR)) == -1)
            if(errno != EINTR)
            {
                perror("Child cant open file");
                return -1;
            }
        if ((sigemptyset(&toblock) == -1) || (sigaddset(&toblock, SIGINT) == -1)){
            perror("Failed to initialize the signal mask");
            return -1;
        }
        while(ctrlc > -1)
        {
            if((linenum = getAline(fd, calc_area, N)) > 0)
            {
                cached = sprintf(buffer, "Process B: the dft of line %d (", linenum);
                for(int i = 0; i < N; ++i)
                    cached += sprintf(buffer + cached, "%.2lf ", *(calc_area + i));
                cached += sprintf(buffer + cached, ") is: ");
                calcDFD(calc_area, &result, N);
                for(int i = 0; i< N; ++i)
                    cached += sprintf(buffer + cached, "(%.2lf)+(%.2lf)*i ", (*(result + i)).reelPart, (*(result + i)).complexPart);
                cached += sprintf(buffer + cached, "\n");
                if (sigprocmask(SIG_BLOCK, &toblock, NULL) == -1)
                {
                    perror("unable to block signal.");
                    return -1;
                }
                block(logdest);
                lseek(logdest, 0, SEEK_END);
                while(write(logdest, buffer, sizeof(char) * cached) < cached);
                unblock(logdest);
                if (sigprocmask(SIG_UNBLOCK, &toblock, NULL) == -1)
                {
                    perror("unable to unblock signal");
                    return -1;
                }
            }
        }
        free(calc_area);
        free(result);
        result      = NULL;
        calc_area   = NULL;
        close(fd);
    }


    int getNdouble(int fd, double ** array, int N)
    {
        int counter;
        int ret_val = 0;

        for(counter = 0; counter < N; ++counter)
        {

            while(read(fd, ((*array) + counter), sizeof(double)) == -1)
                if(errno != EINTR)
                {
                    perror("Reading Child");
                    return -1;
                }
        }

        return ret_val;
    }

    int getAline(int fd, double * array, int N)
    {
        int linenum = 0;
        long int truncp = 0;
        block(fd);
        if((linenum = (lseek(fd, 0, SEEK_END) / (N * sizeof(double) + 1))) > 0)
        {
            truncp = lseek(fd, -(N * sizeof(double) + 1), SEEK_END);
            getNdouble(fd, &array, N);
            ctrlc = linenum - 1;
            ftruncate(fd, truncp);
        }
        unblock(fd);
        return linenum;
    }


    int calcDFD(const double * const input, Complex ** arr, int N)
    {
        double tpkn;
        for(int i = 0; i < N; ++i)
        {
            (*(*(arr) + i)).reelPart     = 0;
            (*(*(arr) + i)).complexPart  = 0;
            for(int j = 0; j < N; ++j)
            {
                tpkn = 2.0 * PI * i * j / (double)N;
                (*(*(arr) + i)).reelPart     += input[j] * cos(tpkn);
                (*(*(arr) + i)).complexPart  -= input[j] * sin(tpkn);
            }
        }
        return 0;
    }
//end
//parent
    int parent(const char * const filedest, int N, int M, int child_pid, int logdest)
    {
        int         fd                  = 0,
                    linenum             = 0,
                    cached              = 0;
        double *    signals             = NULL;
        char        buffer[MAX_BUFFER]  = "";
        sigset_t    toblock                 ;
        while((fd = open(filedest, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH)) == -1)
            if(errno != EINTR)
            {
                perror("Parent openning.");
                return -1;
            }
        signals = (double *) malloc(sizeof(double) * N);
        if ((sigemptyset(&toblock) == -1) || (sigaddset(&toblock, SIGINT) == -1)){
            perror("Failed to initialize the signal mask");
            return -1;
        }
        while(ctrlc > -1)
        
            if((linenum = writeFile(fd, signals, N, M)) != -1)
            {
                cached = sprintf(buffer, "Process A: i’m producing a random sequence for line %d:", linenum);
                for(int i = 0; i < N; ++i)
                    cached += sprintf(buffer + cached, "%.2lf ", *(signals + i));
                cached += sprintf(buffer + cached, "\n");
                if (sigprocmask(SIG_BLOCK, &toblock, NULL) == -1)
                {
                    perror("unable to block signal.");
                    return -1;
                }
                block(logdest);
                lseek(logdest, 0, SEEK_END);                
                while(write(logdest, buffer, sizeof(char) * cached) < cached);
                unblock(logdest);
                if (sigprocmask(SIG_UNBLOCK, &toblock, NULL) == -1)
                {
                    perror("unable to unblock signal");
                    return -1;
                }
            }
        close(fd);
        free(signals);
        signals = NULL;
        return 0;
    }

    void generateArray(double ** array, int N)
    {
        int     i       = 0;
        for(i = 0; i < N; ++i)
            *((*array) + i) = (double)rand() / RAND_MAX;
    }

    int writeFile(int fd, double * array, int N, int M)
    {
        int i = 0,
            ret_val = 0;
        block(fd);
        if((ret_val = (lseek(fd, 0, SEEK_END) / (N * sizeof(double) + 1) + 1)) < M)
        {
            generateArray(&array, N);
            for(i = 0; i < N; ++i)
                while(write(fd, (array + i), sizeof(double)) == -1)
                {
                    if(errno != EINTR)
                        perror("WRITE DOUBLE"), ret_val = -1;
                }
            i = '\n';
            while(write(fd, &i, sizeof(char)) == -1 && errno == EINTR);
        }
        else ret_val = -1;
        unblock(fd);

        return ret_val;
    }


//end