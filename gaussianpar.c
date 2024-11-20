/***************************************************************************
 *
 * Parallel version of Gaussian elimination
 * Don't recreate threads, no deadlock
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "pthread.h"
#include <math.h>
#include <string.h>

#define MAX_SIZE 4096
#define NUM_CORES 8

typedef double matrix[MAX_SIZE][MAX_SIZE];

typedef struct {
    int thread_id;
    int start_row;
    int end_row;
    int k;
    int active;
} threadArgs;

int N;              /*matrix size */
int maxnum;         /*max number of element*/
char *Init;         /*matrix init type */
int PRINT;          /*print switch */
matrix A;           /*matrix A */
double b[MAX_SIZE]; /*vector b */
double y[MAX_SIZE]; /*vector y */
pthread_t threads[NUM_CORES]; /* vector threads */
threadArgs args[NUM_CORES]; /* vector divission thread arguments */
pthread_mutex_t mutexes_1[NUM_CORES];
pthread_mutex_t mutexes_2[NUM_CORES];


/*forward declarations */
void work(void);
void *gaussian_row(void *params);
void Init_Matrix(void);
void Print_Matrix(void);
void Init_Default(void);
int Read_Options(int, char **);

int main(int argc, char **argv)
{
    for (int j = 0; j < NUM_CORES; j++) {
        pthread_mutex_init(&mutexes_1[j], NULL);
        pthread_mutex_init(&mutexes_2[j], NULL);
    }
    Init_Default();           /*Init default values */
    Read_Options(argc, argv); /*Read arguments */
    Init_Matrix();            /*Init the matrix */
    work();
    if (PRINT == 1)
        Print_Matrix();
}

void work(void)
{
    pthread_t threads[NUM_CORES];
    threadArgs args[NUM_CORES];

    int rows_per_thread = (N + NUM_CORES - 1) / NUM_CORES;
    for (int t = 0; t < NUM_CORES; t++) { // start_row threads
        args[t].thread_id = t;
        args[t].start_row = t * rows_per_thread;
        args[t].end_row = ((t + 1) * rows_per_thread - 1 < N - 1) ?
                  (t + 1) * rows_per_thread - 1 :
                  N - 1;
        // args[n].start_row = -1;
        // args[n].end_row = -1;
        args[t].k = -1;
        // args[n].n = n;
        args[t].active = 1;
        pthread_mutex_lock(&mutexes_2[t]);
        pthread_mutex_lock(&mutexes_1[t]);
        pthread_create(&threads[t], NULL, gaussian_row, &args[t]);
    }
    // Wait until all threads has start_rowed
    for (int i = 0; i < NUM_CORES; i++) {
        pthread_mutex_lock(&mutexes_2[i]);
        pthread_mutex_unlock(&mutexes_2[i]);
    }
    for (int k = 0; k < N; k++)
    { /*Outer loop */
        double k_value = A[k][k];
        for (int j = k + 1; j < N; j++)
            A[k][j] = A[k][j] / k_value; /*Division step */
        y[k] = b[k] / k_value;
        A[k][k] = 1.0;
        int step = N / NUM_CORES;
        int n = 0;
        for (int i = k + 1; i < N; i += step)
        {
            args[n].start_row = i;
            args[n].end_row = i + step;
            args[n].k = k;
            pthread_mutex_lock(&mutexes_2[n]);
            pthread_mutex_unlock(&mutexes_1[n]); // start_row thread execution
            n++;
        }
        for (int j = n; j < NUM_CORES; j++) {
            if (args[j].active == 1) {
                args[j].active = 0;
                pthread_mutex_unlock(&mutexes_1[j]); // Allow unused threads to exit
                pthread_join(threads[j], NULL);
            }
        }
        // Wait for threads to finish
        for (int j = 0; j < n; j++) {
            pthread_mutex_lock(&mutexes_2[j]);
            pthread_mutex_unlock(&mutexes_2[j]);
        }
    }
}

void *gaussian_row(void *params){
    threadArgs *args = (threadArgs *)params;
    int n = args->thread_id;
    pthread_mutex_unlock(&mutexes_2[n]);
    while (args->active == 1) {
        // Wait for program to signal run
        pthread_mutex_lock(&mutexes_1[n]);
        pthread_mutex_unlock(&mutexes_1[n]);
        if (args->active == 0) return NULL;
        int start_row = args->start_row;
        int end_row = args->end_row;
        int k = args->k;
        for (int i = start_row; i < end_row && i < N; i++) {
            for (int j = k + 1; j < N; j++)
                A[i][j] = A[i][j] - A[i][k] * A[k][j]; /*Elimination step */
            b[i] = b[i] - A[i][k] * y[k];
            A[i][k] = 0.0;
        }
        // Signal thread finished
        pthread_mutex_lock(&mutexes_1[n]);
        pthread_mutex_unlock(&mutexes_2[n]);
    }
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
                if (i == j) /*diagonal dominance */
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
                if (i == j) /*diagonal dominance */
                    A[i][j] = 5.0;
                else
                    A[i][j] = 2.0;
            }
        }
    }
    /*Initialize vectors b and y */
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
                printf("\n maxnum = 15 ");
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