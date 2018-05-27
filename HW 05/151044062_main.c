#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#define LINE_SIZE 200

typedef struct
{
    char    array[10][20];
    int     size;
} charArrayList;


typedef struct _consumer
{
    char                name[20],
                        toBuy[20];
    int                 position[2];
    struct _consumer *  next;
} consumer;

typedef struct
{
    int                 position [2],
                        totalSell;
    time_t              sell_time;
    char                name[20];
    charArrayList       sell_array;
    struct timespec     proc_time;
    pthread_mutex_t     sbd_does;
} seller;


typedef struct
{
    int                     count;
    seller                  selling[10];
    consumer            **  buying;
    pthread_mutex_t         take_one_pool;
    pthread_cond_t          condition;
} input;

volatile unsigned short pop_pool = 0;
volatile unsigned int   finished = 0;


double euclidian(int * first, int * sec)
{
    return sqrt(((double)(* first) - (double)(* sec)) * ((double)(* first) - (double)(* sec))
                + 
                ((double)(* (first + 1)) - (double)(* (sec + 1))) *((double)(* (first + 1)) - (double)(* (sec + 1))));
}

void * selling(void * args)
{
    int             count           = ((input *) args)->count           ,
                    least_position  = 0                                 ,
                    i               = 0                                 ,
                    j               = 0                                 ;
    struct timeval  begin                                               , 
                    end                                                 ;
    time_t          create_time     = 0L                                ;
    double          min_path        = 0.0                               ,
                    temp            = 0.0                               ;
    seller      *   flover_sellers  = ((input *) args)->selling         ;
    consumer        one_consume                                         ;
    pthread_mutex_t sync_pool       = ((input *) args)->take_one_pool   ;
    pthread_cond_t  sync            = ((input *) args)->condition       ;
    while(*(((input *) args)->buying) != NULL || !finished)
    {
        pthread_mutex_lock(&sync_pool);
        while(pop_pool == 0)
            pthread_cond_wait(&sync, &sync_pool);
        --pop_pool;
        if(((input *) args)->buying != NULL)
        {
            one_consume     = **(((input *) args)->buying);
            ((input *) args)->buying = &(one_consume.next);
            pthread_mutex_unlock(&sync_pool);
            {
                least_position = 0;
                min_path = euclidian(flover_sellers->position, one_consume.position);
                for(i = 1; i < count; ++i)
                    for(j = 0; j < (flover_sellers + i)->sell_array.size; ++j)
                        if(strcmp((flover_sellers + i)->sell_array.array[j], one_consume.toBuy) == 0)
                        {
                            temp = euclidian((flover_sellers + i)->position, one_consume.position);
                            if(temp < min_path)
                            {
                                min_path = temp;
                                least_position = i;
                            }
                        }
                pthread_mutex_lock(&((flover_sellers + least_position)->sbd_does));
                gettimeofday(&begin, NULL);
                nanosleep(&((flover_sellers + least_position)->proc_time), NULL);
                gettimeofday(&end, NULL);
                create_time = (end.tv_usec - begin.tv_usec) / 1000;
                printf("Florist %s has delivered a(n) %s to %s in %ld ms\n"
                                                                    , (flover_sellers + least_position)->name
                                                                    , one_consume.toBuy
                                                                    , one_consume.name
                                                                    , create_time);
                ++((flover_sellers + least_position)->totalSell);
                (flover_sellers + least_position)->sell_time += create_time;
                pthread_mutex_unlock(&((flover_sellers + least_position)->sbd_does));
            }
        }
        else
            pthread_mutex_unlock(&sync_pool);
    }
    pthread_exit(NULL);
}



int read_flovers(const char * const input, charArrayList * to_array)
{
    int i = 0, pos = 0;
    for(i = 0; *(input + i) != ':'; ++i);
    i += 2;
    while(*(input + i) != 0)
    {
        if(*(input + i) == ',') 
        {
            *(*(to_array->array + to_array->size) + pos) = 0;
            ++(to_array->size);
            i += 2;
            pos = 0;
        }
        else
        {            
            *(*(to_array->array + to_array->size) + pos) = *(input + i);
            ++pos;
            ++i;
        }
    }
    *(*(to_array->array + to_array->size) + pos) = 0;
    ++(to_array->size);    
}

int parse_seller(int fd, seller * input)
{
    int     i           = 0,
            product_pos = 0,
            created     = 0,
            temp        = 0,
            flag        = 1;
    double  timeTemp    = 0.0;
    char    aLine[LINE_SIZE];
    for(created = 0; flag; ++created)
    {
        for(i = 0; read(fd, aLine + i, sizeof(char)) && *(aLine + i) != '\n'; ++i);
        *(aLine + i) = 0;

        if(i == 0)
            flag = !flag;
        else
        {           
            sscanf(aLine, "%s (%d,%d; %lf) : ", 
                        (input + created)->name,
                        (input + created)->position,
                        (input + created)->position + 1,
                        &timeTemp);
            (input + created)->sell_array.size = 0;
            read_flovers(aLine, &((input + created)->sell_array));
            (input + created)->proc_time.tv_sec     = 0;
            (input + created)->proc_time.tv_nsec  = timeTemp * 10000000;
            (input + created)->totalSell        = 0;
            (input + created)->sell_time        = 0;
            pthread_mutex_init(&((input + created)->sbd_does), NULL);
        }
    }
    return created - 1;
}


void seller_close(input * data)
{
    int i = 0;
    for(i = 0; i < data->count; ++i)
    {
        pthread_mutex_destroy(&(data->selling + i)->sbd_does);
        printf("%s is closing shop.\n", (data->selling + i)->name);
    }
    
}

consumer * delete_list(consumer * links)
{
    if(links != NULL)
    {   
        links->next = delete_list(links->next);
        free(links);
    }
    return NULL;
}


int get_consumer(int fd, consumer ** to_queue)
{
    int         i,
                temp = 0;
    char        aLine[LINE_SIZE];
    consumer * toFill = *to_queue;
    for(i = 0; (temp = read(fd, aLine + i, 1)) && *(aLine + i) != '\n'; ++i);
    *(aLine + i) = 0;    
    if(temp == 0 && i == 0)
        return 0;
    if(toFill == NULL)
        toFill = *to_queue = (consumer *) malloc(sizeof(consumer));
    else
    {
        while(toFill->next != NULL) toFill = (consumer *) toFill->next;
        toFill->next = (consumer *) malloc(sizeof(consumer));
        toFill = (consumer *) toFill->next;
    }
    sscanf(aLine, "%s (%d,%d): %s", toFill->name, toFill->position, toFill->position + 1, toFill->toBuy);
    toFill->next = NULL;
    return 1;
}


void print(input * data)
{
    int i = 0;
    printf("All requests processed.\n");
    pthread_mutex_destroy(&data->take_one_pool);
    pthread_cond_destroy(&data->condition);
    printf("Sale statistics for today.\n");
    printf("----------------------------------------------------\n");
    printf("Florist\t\t# of sales\t\tTotal time\n");
    printf("----------------------------------------------------\n");    
    for(i = 0; i < data->count; ++i)
        printf("%s\t\t%d\t\t%lums\n", (data->selling + i)->name, (data->selling + i)->totalSell, (data->selling + i)->sell_time);
    printf("----------------------------------------------------\n");    
}



int main(int argc, char ** argv)
{
    int         fd,
                i;
    input       input_item;
    consumer *  HEAD;
    pthread_t florists[10];
    if(argc < 2 || argc > 2)
    {
        fprintf(stderr, "%s filename", argv[0]);
        _exit(EXIT_FAILURE);
    }
    if((fd = open(argv[1], O_RDONLY)) == -1)
    {
        perror("File openning error.");
        _exit(EXIT_FAILURE);
    }

    printf("Florist application is initializing from file: %s\n", argv[1]);
    HEAD = NULL;
    input_item.buying = &HEAD;
    input_item.count = parse_seller(fd, input_item.selling);
    pthread_mutex_init(&input_item.take_one_pool, NULL);
    pthread_cond_init(&input_item.condition, NULL);
    for(i = 0; i < input_item.count; ++i)
        pthread_create(florists + i, NULL, selling, &input_item);
    printf("%d Florists have been created.\n", input_item.count);
    printf("Processing requests\n");
    while(get_consumer(fd, &HEAD))
    {
        ++pop_pool;
        pthread_cond_signal(&(input_item.condition));
    }
    finished = 1;
    pthread_cond_broadcast(&(input_item.condition));

    for(i = 0; i < input_item.count; ++i)
        pthread_join(*(florists + i), NULL);

    seller_close(&input_item);
    print(&input_item);
    HEAD = delete_list(HEAD);
    return 0;
}