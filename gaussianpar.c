/***************************************************************************
 *
 * Parallel version of Gaussian elimination
 *
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define MAX_SIZE 2048
#define NUM_CORES 16

typedef double matrix[MAX_SIZE][MAX_SIZE];
int N;              /* matrix size */
int maxnum;         /* max number of element*/
char *Init;         /* matrix init type */
int PRINT;          /* print switch */
matrix A;           /* matrix A */
double b[MAX_SIZE]; /* vector b */
double y[MAX_SIZE]; /* vector y */
/* forward declarations */
void work(void);
void Init_Matrix(void);
void Print_Matrix(void);
void Init_Default(void);
int Read_Options(int, char **);


pthread_mutex_t *mutex_start; //Signals thread to start
pthread_mutex_t *mutex_done; //Signals that thread finished

typedef struct {
    int thread_id;
    int start_row;
    int end_row;
    int active;
    int pivot;
} threadArgs;

void init_mutexes() {

    mutex_start = malloc(NUM_CORES * sizeof(pthread_mutex_t));
    mutex_done = malloc(NUM_CORES * sizeof(pthread_mutex_t));
    for (int i = 0; i < NUM_CORES; i++) {

        pthread_mutex_init(&mutex_start[i], NULL);
        pthread_mutex_init(&mutex_done[i], NULL);

    }
}

void *gaussian_row(void *params)
{


    threadArgs *args = (threadArgs *)params;
    int curr = args->thread_id; // Current thread ID

    // Wait for signal to start working
    pthread_mutex_lock(&mutex_start[curr]);
    pthread_mutex_unlock(&mutex_done[curr]);// Unlock the done mutex (signal ready)
    while (args->active == 1) {// Wait for signal
        if (args->active == 0){ // Exits loop if thread becomes inactive
            return NULL;
        }

        int start_row = args->start_row;
        int end_row = args->end_row;


        int k = args->pivot;
        // printf("The value of start k: %d\n", k);
        for (int i = start_row; i < end_row && i < N; i++) {
            // printf("The value of i is: %d\n", i);
            // printf("The value of end is: %d\n", end_row);
            for (int j = k + 1; j < N; j++){
                if (i < 0 || i >= N || j < 0 || j >= N) {
                    //printf("Thread %d: Accessing out-of-bounds element A[%d][%d] or b[%d]\n", curr, i, j, i);
                    return NULL;
                }
                A[i][j] = A[i][j] - A[i][k] * A[k][j]; /*Elimination step */
            }
            b[i] = b[i] - A[i][k] * y[k];
            A[i][k] = 0.0;
        }
        // Thread is done
        //printf("THREAD IS DONEEEEEEE");
        // Wait for the next signal from the main thread
        pthread_mutex_lock(&mutex_start[curr]);
        pthread_mutex_unlock(&mutex_done[curr]); // Signal complete
    }
}

int main(int argc, char **argv)
{
    init_mutexes(); //Initialize mutexes, HELPER FUNCTION

    Init_Default();           /* Init default values */

    Read_Options(argc, argv); /* Read arguments */
    Init_Matrix();            /* Init the matrix */
    //printf("hej");
    work();
    if (PRINT == 1)
        Print_Matrix();
}

void work(void){

    threadArgs args[NUM_CORES]; //store arguments for each thread
    pthread_t children[NUM_CORES]; // Array of thread handles

    //calculate number of rows per thread by using ceiling division
    int rows_per_thread = (N + NUM_CORES - 1) / NUM_CORES;
    // Initialize all threads
    for (int t = 0; t < NUM_CORES; t++){ //pass args
        args[t].thread_id = t;
        args[t].start_row = t * rows_per_thread;
        args[t].pivot = -1;
        int end = (t + 1) * rows_per_thread - 1; //block number * rows
        if (end < N - 1) { //if end does not go out of range
            args[t].end_row = end;
        } else {
            args[t].end_row = N - 1; //last row
        }
        // printf("t is: %d\n", t);
        // printf("The value of end is: %d\n", args[t].end_row);
        args[t].active = 1; // Set thread as active
        //printf("Creating thread %d with start_row = %d, end_row = %d\n", t, args[t].start_row, args[t].end_row);
        pthread_create(&(children[t]), NULL, gaussian_row, (void *)&args[t]);
    }
    // for (int i = 0; i < NUM_CORES; i++) { //REMOVE
    //     pthread_mutex_lock(&mutex_done[i]);
    //     pthread_mutex_unlock(&mutex_done[i]);
    // }
    for (int k = 0; k < N; k++){ /*Outer loop */
        double diag = A[k][k]; //diagonal element

        for (int j = k + 1; j < N; j++)
            A[k][j] = A[k][j] / diag; /*Division step */
        y[k] = b[k] / diag;
        A[k][k] = 1.0;

        int div = N / NUM_CORES; //division
        int counter = 0;

        for (int i = k + 1; i < N; i += div){
            //printf("kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk: %d\n", k);
            args[counter].pivot = k;
            args[counter].start_row = i;
            args[counter].end_row = i + div;
            pthread_mutex_lock(&mutex_done[counter]);
            pthread_mutex_unlock(&mutex_start[counter]);
            counter++;
        }
        for (int j = counter; j < NUM_CORES; j++) {
            if (args[j].active == 1) { //Terminate thread
                args[j].active = 0;
                pthread_mutex_unlock(&mutex_start[j]); //Threads are waiting
                pthread_join(children[j], NULL);
            }
        }
        // Wait for threads to finish
        for (int h = 0; h < counter; h++) {
            pthread_mutex_lock(&mutex_done[h]);
            pthread_mutex_unlock(&mutex_done[h]);
        }
    }

    for (int i = 0; i < NUM_CORES; i++) {
        pthread_mutex_destroy(&mutex_start[i]);
        pthread_mutex_destroy(&mutex_done[i]);
    }

    free(mutex_start);
    free(mutex_done);
}
void Init_Matrix()
{
    int i, j;
    printf("\nsize = %dx%d ", N, N);
    printf("\nmaxnum = %d \n", maxnum);
    printf("Init = %s \n", Init);
    printf("Initializing matrix...");
    if (strcmp(Init, "rand") == 0)
    {
        for (i = 0; i < N; i++)
        {
            for (j = 0; j < N; j++)
            {
                if (i == j) /* diagonal dominance */
                    A[i][j] = (double)(rand() % maxnum) + 5.0;
                else
                    A[i][j] = (double)(rand() % maxnum) + 1.0;
            }
        }
    }
    if (strcmp(Init, "fast") == 0)
    {
        for (i = 0; i < N; i++)
        {
            for (j = 0; j < N; j++)
            {
                if (i == j) /* diagonal dominance */
                    A[i][j] = 5.0;
                else
                    A[i][j] = 2.0;
            }
        }
    }
    /* Initialize vectors b and y */
    for (i = 0; i < N; i++)
    {
        b[i] = 2.0;
        y[i] = 1.0;
    }
    printf("done \n\n");
    if (PRINT == 1)
        Print_Matrix();
}
void Print_Matrix()
{
    int i, j;
    printf("Matrix A:\n");
    for (i = 0; i < N; i++)
    {
        printf("[");
        for (j = 0; j < N; j++)
            printf(" %5.2f,", A[i][j]);
        printf("]\n");
    }
    printf("Vector b:\n[");
    for (j = 0; j < N; j++)
        printf(" %5.2f,", b[j]);
    printf("]\n");
    printf("Vector y:\n[");
    for (j = 0; j < N; j++)
        printf(" %5.2f,", y[j]);
    printf("]\n");
    printf("\n\n");
}
void Init_Default()
{
    N = 2048;
    Init = "rand";
    maxnum = 15.0;
    PRINT = 0;
}
int Read_Options(int argc, char **argv)
{
    char *prog;
    prog = *argv;
    while (++argv, --argc > 0)
        if (**argv == '-')
            switch (*++*argv)
            {
            case 'n':
                --argc;
                N = atoi(*++argv);
                break;
            case 'h':
                printf("\nHELP: try sor -u \n\n");
                exit(0);
                break;
            case 'u':
                printf("\nUsage: gaussian [-n problemsize]\n");
                printf(" [-D] show default values \n");
                printf(" [-h] help \n");
                printf(" [-I init_type] fast/rand \n");
                printf(" [-m maxnum] max random no \n");
                printf(" [-P print_switch] 0/1 \n");
                exit(0);
                break;
            case 'D':
                printf("\nDefault: n = %d ", N);
                printf("\n Init = rand");
                printf("\n maxnum = 5 ");
                printf("\n P = 0 \n\n");
                exit(0);
                break;
            case 'I':
                --argc;
                Init = *++argv;
                break;
            case 'm':
                --argc;
                maxnum = atoi(*++argv);
                break;
            case 'P':
                --argc;
                PRINT = atoi(*++argv);
                break;
            default:
                printf("%s: ignored option: -%s\n", prog, *argv);
                printf("HELP: try %s -u \n\n", prog);
                break;
            }
}
