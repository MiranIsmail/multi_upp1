/***************************************************************************
 *
 * Parallel version of Gaussian elimination
 *
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

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

pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_barrier_t barrier;

typedef struct {
    int thread_id;
    int start_row;
    int end_row;
} threadArgs;

void *gaussian_row(void *params) {
    threadArgs *args = (threadArgs *)params;
    int tid = args->thread_id;
    int start = args->start_row;
    int end = args->end_row;

    for (int k = 0; k < N; k++) { // Outer loop
        if (tid == 0) { // Only one thread handles division for row k
            for (int j = k + 1; j < N; j++)
                A[k][j] = A[k][j] / A[k][k]; /* Division step */
            y[k] = b[k] / A[k][k];
            A[k][k] = 1.0;
        }

        // Barrier that makes sure that all threads see the updated row k
        pthread_barrier_wait(&barrier);

        // Each thread processes its assigned rows
        for (int i = start; i <= end; i++) {
            if (i > k) { // Eliminate only rows below the pivot
                for (int j = k + 1; j < N; j++)
                    A[i][j] = A[i][j] - A[i][k] * A[k][j]; /* Elimination step */
                b[i] = b[i] - A[i][k] * y[k];
                A[i][k] = 0.0; //done with this one
            }
        }

        // Barrier waits for all rows to be updated before moving to the next pivot
        pthread_barrier_wait(&barrier);
    }

    pthread_exit(NULL);
}

void work(void) {
    pthread_t threads[NUM_CORES];
    threadArgs args[NUM_CORES];

    int rows_per_thread = (N + NUM_CORES - 1) / NUM_CORES;

    // Initialize barrier
    pthread_barrier_init(&barrier, NULL, NUM_CORES);

    // Create threads
    for (int t = 0; t < NUM_CORES; t++) {
        args[t].thread_id = t;
        args[t].start_row = t * rows_per_thread;
        args[t].end_row = ((t + 1) * rows_per_thread - 1 < N - 1) ?
                  (t + 1) * rows_per_thread - 1 :
                  N - 1;
        pthread_create(&threads[t], NULL, gaussian_row, &args[t]);
    }

    // Join threads
    for (int t = 0; t < NUM_CORES; t++) {
        pthread_join(threads[t], NULL);
    }

    // Destroy barrier
    pthread_barrier_destroy(&barrier);
}


int main(int argc, char **argv)
{
    int i, timestart, timeend, iter;
    Init_Default();           /* Init default values */
    Read_Options(argc, argv); /* Read arguments */
    Init_Matrix();            /* Init the matrix */
    work();
    if (PRINT == 1)
        Print_Matrix();
}

void Init_Matrix()
{
    int i, j;
    //printf("\nsize = %dx%d ", N, N);
    //printf("\nmaxnum = %d \n", maxnum);
    //printf("Init = %s \n", Init);
    //printf("Initializing matrix...");
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
    //printf("done \n\n");
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
