#ifndef BOTH
    #define BOTH
    
    #include "./libs.h"
    #include <math.h>    
    #define INPUT 7
    #ifndef PI
        #define PI 3.1415
    #endif
    #define MAX_BUFFER 1024
    #define LOG_FILE "logs/log.log"
    
    /**
     *  Complex data type for dfd.
     **/
    typedef struct
    {
        double reelPart;
        double complexPart;
    } Complex;

    /**
      * This is a counter for sigint
      **/
    volatile sig_atomic_t ctrlc;

    /**
      * This is a handler for Sigint
      * When this signal comes, makes counter to -1;
      **/
    void cchandler(int signo);
    
    /**
      * Takes a sigaction struct, fills it with necessary signals to handle.
      **/
    void update_sign(struct sigaction * sa);


    /**
     *  This function takes argv for get inputs.
     *  If format doesn't match, then program exits with fail.
     **/
    int parsearg(char ** argv, int * const N, char ** filedest, int * const M);


    /**
     *  Reads N times double to array from given innput file.
     **/
    int getNdouble(int fd, double ** array, int N);
    
    /**
     *  Reads a line if file size is bigger than 0.
     **/
    int getAline(int fd, double * array, int N);
    
    /**
     *  Child's all works. 
     *  N for signal count, Logdest is for log file dest.
     **/
    int child(char * filedest, int N, int logdest);

    /**
     *  DfD calculator.
     **/
    int calcDFD(const double * const input, Complex ** arr, int N);

    /**
     *  Parent's generating N times double to array.
     **/
    void generateArray(double ** array, int N);
    
    /**
     *  Writes to file if line count is less than M
     **/
    int writeFile(int fd, double * array, int N, int M);
    
    /**
     *  All parent work. Generate array, write file, take logs.
     **/
    int parent(const char * const filedest, int N, int M, int child_pid, int logdest);

    /**
     *  Takes a fd as input, locks it for writing with waiting flag.
     **/
    int block(int fd);


    /**
     *  Takes a fd as input, unlocks it with waiting flag.
     **/
    int unblock(int fd);

#endif