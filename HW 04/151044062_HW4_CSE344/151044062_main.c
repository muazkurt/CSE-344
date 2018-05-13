#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#define FLAGS           O_RDWR | O_CREAT
#define PERMS           S_IRUSR | S_IWUSR
#define MAPPROTS        PROT_READ | PROT_WRITE
#define CHEF            "Chef"
#define WHSELLR         "Wholesaler "
#define BUTTER_SUGAR    "butter and sugar."
#define FLOUR_SUGAR     "flour and sugar."
#define FLOUR_BUTTER    "flour and butter."
#define EGGS_SUGAR      "eggs and sugar."
#define EGGS_BUTTER     "and butter."
#define EGGS_FLOUR      "eggs and flour."
#define IS_WAITING      "is waiting "
#define HAS_TAKEN       "has taken "
#define PREPARING       "is preparing "
#define THE_DESSERT     "the dessert."

static sig_atomic_t volatile counter = 0;

void cc_handler (int signo)
{
    ++counter;
}


int getintby(int input)
{
    int ret_val = 0;
    if((ret_val = rand() % input) < 0)
        ret_val += input;
    return ret_val;
}


int main(int argc, char *argv[])
{
    int fd, child, temp, max_val;
    size_t size;
    sem_t * chef06_rw_ws;
    struct sigaction newact, oldact;
    sigset_t block, unblock;
    
    if(argc != 2)
    {
        fprintf(stderr, "usage:\n\t%s count", argv[0]);
        exit(EXIT_FAILURE);
    }

    if((max_val = atoi(argv[1])) > max_val  || max_val < 2)
    {
        fprintf(stderr, "Range error\n");
        exit(EXIT_FAILURE);        
    }
    srand(time(NULL));
    size = sizeof(sem_t) * max_val + 2;
    if ((fd = shm_open("smemory", FLAGS, PERMS)) == -1 || ftruncate(fd, size) == -1)
    {
        perror("Error to creat shared memory");
        exit(EXIT_FAILURE);
    }
    if((chef06_rw_ws = mmap(NULL, size, MAPPROTS, MAP_SHARED, fd, 0)) == MAP_FAILED)
    {
        perror("Map error");
        exit(EXIT_FAILURE);
    }

    //fill empty
    sigemptyset(&newact.sa_mask);
    newact.sa_handler = cc_handler;
    newact.sa_flags = 0;
    newact.sa_restorer = NULL;
    newact.sa_handler = cc_handler;
    sigaction(SIGINT, &newact, &oldact);
    //

    sigemptyset(&block);
    sigaddset(&block, SIGINT);
    sem_init(chef06_rw_ws + max_val  , 1, 1);          //Init the output lock.
    sem_init(chef06_rw_ws + max_val + 1 , 1, 1);          //Race condition.
    sem_init(chef06_rw_ws + max_val + 2 , 1, 0);          //Wholeseller lock.
    for(int i = 0; i < max_val ; ++i)
    {        
        if(sem_init(chef06_rw_ws + i, 1, 0) == -1)
        {
            perror("Semaphore can't init");
            exit(EXIT_FAILURE);
        }
        
        if((child = fork()) == -1)
        {
            perror("Fork error.");
            exit(EXIT_FAILURE);
        }
        if(child == 0)
        {
            while(counter == 0)
            {
                if(sem_wait(chef06_rw_ws + max_val ) != -1 && sem_wait(chef06_rw_ws + max_val + 1) != -1)
                {
                    printf("%s%d %s", CHEF, i, IS_WAITING);
                    switch(i)
                    {
                        case 0:
                            printf(BUTTER_SUGAR);
                            break;
                        case 1:
                            printf(FLOUR_BUTTER);
                            break;
                        case 2:
                            printf(FLOUR_SUGAR);
                            break;
                        case 3:
                            printf(EGGS_SUGAR);
                            break;
                        case 4:
                            printf(EGGS_BUTTER);
                            break;
                        case 5:
                            printf(EGGS_FLOUR);
                            break;
                        default:
                            break;
                    }
                    printf("\n");
                    //output relase
                    if(sem_post(chef06_rw_ws + max_val + 1) != -1 && sem_post(chef06_rw_ws + max_val ) != -1)
                    {
                        if( sem_wait(chef06_rw_ws + i) != -1 &&     //items lock
                            sem_wait(chef06_rw_ws + max_val ) != -1 &&     //output lock
                            sem_wait(chef06_rw_ws + max_val + 1) != -1)       //race condition lock
                        {
                            printf("%s%d %s%s\n", CHEF, i, PREPARING, THE_DESSERT);
                            printf("%s%d has delivered the dessert to the wholesaler\n", CHEF, i);
                            if( sem_post(chef06_rw_ws + max_val + 1) != -1 &&     //output relase
                                sem_post(chef06_rw_ws + max_val ) != -1 &&     //race condition relase
                                sem_post(chef06_rw_ws + max_val + 2) != -1);      //give şekerpare
                        }
                    }
                }
            }
            sigprocmask(SIG_BLOCK, &block, &unblock);
            kill(-getppid(), SIGINT);
            printf("%s%d closes the shop\n", CHEF, i);
            if (munmap(chef06_rw_ws, size) == -1) return -1;
            sigprocmask(SIG_UNBLOCK, &block, NULL);
            sigaction(SIGINT, &oldact, NULL);
            exit(EXIT_SUCCESS);
        }
    }
    while(counter == 0)
    {
        if( sem_wait(chef06_rw_ws + max_val ) != -1 &&     //items lock
            sem_wait(chef06_rw_ws + max_val + 1) != -1)       //output lock
        {
            printf("%sdelivers ", WHSELLR);
            switch(temp = getintby(max_val))
            {
                case 0:
                    printf(BUTTER_SUGAR);
                    break;
                case 1:
                    printf(FLOUR_BUTTER);
                    break;
                case 2:
                    printf(FLOUR_SUGAR);
                    break;
                case 3:
                    printf(EGGS_SUGAR);
                    break;
                case 4:
                    printf(EGGS_BUTTER);
                    break;
                case 5:
                    printf(EGGS_FLOUR);
                    break;
                default:
                    break;
            }
            printf("\n");
            if(sem_post(chef06_rw_ws + max_val + 1)           != -1   &&      //output relase
                sem_post(chef06_rw_ws + max_val )          != -1   &&      //race condition relase
                sem_post(chef06_rw_ws + temp)       != -1   &&      //give items
                sem_wait(chef06_rw_ws + max_val )          != -1   &&      //output lock
                sem_wait(chef06_rw_ws + max_val + 1)          != -1)          //race condition lock
            {
                printf("%s%s%s\n", WHSELLR, IS_WAITING, THE_DESSERT);
                //output relase
                if( sem_post(chef06_rw_ws + max_val + 1) != -1 &&         //race condition relase
                    sem_post(chef06_rw_ws + max_val ) != -1 &&         //output relase
                    sem_wait(chef06_rw_ws + max_val + 2) != -1 &&         //get the items.
                    sem_wait(chef06_rw_ws + max_val ) != -1 &&         //output lock
                    sem_wait(chef06_rw_ws + max_val + 1) != -1)           //race condition locl
                {
                    printf("wholesaler has obtained the dessert and left to sell it\n");
                    //output relase
                    if( sem_post(chef06_rw_ws + max_val + 1) != -1    &&  //race condition relase
                        sem_post(chef06_rw_ws + max_val ) != -1);      //output relase
                }
            }
        }
    }
    sigprocmask(SIG_BLOCK, &block, &unblock);
    for(int i = 0; i < max_val ; ++i)
        if(wait(NULL) == -1 && errno == EINTR);
    for(int i = 0; i < max_val + 2; ++i)
        sem_destroy(chef06_rw_ws + i);
    if (munmap(chef06_rw_ws, size) == -1) return -1;
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    sigaction(SIGINT, &oldact, NULL);
    shm_unlink("smemory");
    exit(EXIT_SUCCESS);
}
