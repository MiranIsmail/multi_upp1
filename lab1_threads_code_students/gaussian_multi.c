/***************************************************************************
 *
 * Parallel version of Gaussian elimination
 *
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
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

struct threadArgs
{
    unsigned int row;
    int *list;
};

void *gaussian_row(void *params)
{
    struct threadArgs *args = (struct threadArgs *)params;

    int arg_count = args->row;
    int *arg_list = args->list;
    int i, j;
    for (int v = 0; v < arg_count; v++)
    {
        int k = arg_list[v];
        for (j = k + 1; j < N; j++)
            A[k][j] = A[k][j] / A[k][k]; /* Division step */
        y[k] = b[k] / A[k][k];
        A[k][k] = 1.0;
        for (i = k + 1; i < N; i++)
        {
            for (j = k + 1; j < N; j++)
                A[i][j] = A[i][j] - A[i][k] * A[k][j]; /* Elimination step */
            b[i] = b[i] - A[i][k] * y[k];
            A[i][k] = 0.0;
        }
    }
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

void work(void)
{
    int k;
    pthread_t *children;
    struct threadArgs *args; // argument buffer
    int s = N % NUM_CORES;
    int z = N - s;           // The rest term from division
    int div = z / NUM_CORES; // How many times N can be equally divided
    int g = 0;
    int counter = 0;
    children = malloc(N * sizeof(pthread_t));             // allocate array of handles
    args = malloc(NUM_CORES * sizeof(struct threadArgs)); // args vector
                                                          /* Gaussian elimination algorithm, Algo 8.4 from Grama */
    for (k = 0; k < NUM_CORES; k++)
    {
        args[k].row = 0;
        args[k].list = malloc(N * sizeof(int)); // Allocate memory for the list array

        if (args[k].list == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
    }
    for (k = 0; k < N; k++)
    { /* Outer loop */

        if (z != 0 && counter == div + 1)
        {
            args[g].row = counter;
            g++;
            z--;
            counter = 0;
        }
        else if (z == 0 && counter == div)
        {
            args[g].row = counter;
            g++;
            counter = 0;
        }

        args[g].list[counter] = k;
        pthread_mutex_lock(&counter_mutex);
        counter++;
        pthread_mutex_unlock(&counter_mutex);
    }
    for (k = 0; k < NUM_CORES; k++)
    {
        pthread_create(&(children[k]),    // our handle for the child
                       NULL,              // attributes of the child
                       gaussian_row,      // the function it should run
                       (void *)&args[k]); // args to that function
    }
    for (k = 0; k < NUM_CORES; k++)
    {
        pthread_join(children[k], NULL);
    }
    for (k = 0; k < NUM_CORES; k++)
    {
        free(args[k].list);
    }
    free(args);     // deallocate args vector
    free(children); // deallocate array
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
