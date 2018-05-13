#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <semaphore.h>

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
int main(int argc, char *argv[])
{
    int flags, fd;
    mode_t perms;
    size_t size;
    sem_t * addr;
    int ch;
    flags = O_RDWR | O_CREAT;


    size = sizeof(sem_t) * 7;
    perms = S_IRUSR | S_IWUSR;

    /* Create shared memory object and set its size */

    if ((fd = shm_open("smem", flags, perms)) == -1)
    {
        perror("Error to creat shared memory");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(fd, size) == -1)
    {
        perror("ftrunc");
        exit(EXIT_FAILURE);
    }
    if((addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
        exit(EXIT_FAILURE);
    sem_init(addr + 7 , 1, 0);
    sem_init(addr + 6 , 1, 1);

    for(int i = 0; i < 6; ++i)
    {        
        if(sem_init(addr + i, 1, 0) == -1)
        {
            perror("Semaphore can't init");
            exit(EXIT_FAILURE);
        }
        
        if((ch = fork()) == -1)
            exit(EXIT_FAILURE);
        if(ch == 0)
        {
            while( 1)
            {
                //output lock
                sem_wait(addr + 6);
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
                sem_post(addr + 6);                
                //items lock
                sem_wait(addr + i);
                //output lock
                sem_wait(addr + 6);                
                printf("%s%d %s%s\n", CHEF, i, PREPARING, THE_DESSERT);
                printf("%s%d has delivered the dessert to the wholesaler\n", CHEF, i);
                //output relase
                sem_post(addr + 6);
                //give şekerpare
                sem_post(addr + 7);
            }
            if (munmap(addr, size) == -1) return -1;
            exit(EXIT_SUCCESS);
        }
    }
    for(int i = 0; 1; ++i)
    {
        //output lock
        sem_wait(addr + 6);        
        printf("%sdelivers ", WHSELLR);
        switch( i % 6)
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
        sem_post(addr + 6);        
        //give items
        sem_post(addr + (i % 6));
        //output lock
        sem_wait(addr + 6);        
        printf("%s%s%s\n", WHSELLR, IS_WAITING, THE_DESSERT);
        //output relase
        sem_post(addr + 6);        
        //get the items.
        sem_wait(addr + 7);
        //output lock
        sem_wait(addr + 6);        
        printf("wholesaler has obtained the dessert and left to sell it\n");
        //output relase
        sem_post(addr + 6);
    }
    wait(NULL);    
    for(int i = 0; i < 7; ++i)
        sem_destroy(addr + i);
    if (munmap(addr, size) == -1) return -1;
    shm_unlink("smem");
    exit(EXIT_SUCCESS);
}
