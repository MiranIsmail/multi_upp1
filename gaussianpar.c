#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

#define MAX_SIZE 2048
#define NUM_THREADS 8

typedef double matrix[MAX_SIZE][MAX_SIZE];
typedef struct {
    int start_row;
    int end_row;
    int pivot_row;
    int thread_id;
    int active;
} ThreadParams;

int N;              /* Matrix size */
int maxnum;         /* Maximum number for elements */
char *Init;         /* Matrix initialization type */
int PRINT;          /* Print switch */
matrix A;           /* Coefficient matrix */
double b[MAX_SIZE]; /* Right-hand side vector */
double y[MAX_SIZE]; /* Solution vector */
pthread_t threads[NUM_THREADS];
ThreadParams thread_data[NUM_THREADS];
pthread_mutex_t control_mutex[NUM_THREADS];
pthread_mutex_t sync_mutex[NUM_THREADS];

/* Forward declarations */
void work(void);
void *gaussian_row(void *params);
void Init_Matrix(void);
void Print_Matrix(void);
void Init_Default(void);
int Read_Options(int, char **);

int main(int argc, char **argv)
{
    /* Initialize mutexes */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_mutex_init(&control_mutex[i], NULL);
        pthread_mutex_init(&sync_mutex[i], NULL);
        pthread_mutex_lock(&control_mutex[i]); // Lock threads until work is assigned
    }

    Init_Default();           /* Initialize default values */
    Read_Options(argc, argv); /* Parse arguments */
    Init_Matrix();            /* Initialize the matrix */
    work();
    if (PRINT == 1) {
        Print_Matrix();
    }
    return 0;
}

void work(void)
{
    /* Create threads */
    for (int t = 0; t < NUM_THREADS; t++) {
        thread_data[t].thread_id = t;
        thread_data[t].active = 1; // Mark thread active
        pthread_create(&threads[t], NULL, gaussian_row, &thread_data[t]);
    }

    for (int pivot_row = 0; pivot_row < N; pivot_row++) {
        /* Normalize the pivot row */
        double pivot_value = A[pivot_row][pivot_row];
        for (int j = pivot_row + 1; j < N; j++) {
            A[pivot_row][j] /= pivot_value; // Division step
        }
        b[pivot_row] /= pivot_value;
        A[pivot_row][pivot_row] = 1.0;

        /* Distribute row elimination tasks */
        int rows_per_thread = (N - pivot_row - 1 + NUM_THREADS - 1) / NUM_THREADS; // Ceil division
        int thread_count = 0;

        for (int start_row = pivot_row + 1; start_row < N; start_row += rows_per_thread) {
            int end_row = start_row + rows_per_thread;
            if (end_row > N) {
                end_row = N;
            }

            thread_data[thread_count].start_row = start_row;
            thread_data[thread_count].end_row = end_row;
            thread_data[thread_count].pivot_row = pivot_row;

            pthread_mutex_unlock(&control_mutex[thread_count]); // Signal thread to start
            thread_count++;
        }

        /* Synchronize active threads */
        for (int t = 0; t < thread_count; t++) {
            pthread_mutex_lock(&sync_mutex[t]); // Wait for thread completion
        }
    }

    /* Terminate all threads */
    for (int t = 0; t < NUM_THREADS; t++) {
        thread_data[t].active = 0; // Mark thread inactive
        pthread_mutex_unlock(&control_mutex[t]); // Allow thread to exit
        pthread_join(threads[t], NULL);
    }
}

void *gaussian_row(void *params)
{
    ThreadParams *data = (ThreadParams *)params;

    while (data->active) {
        /* Wait for work assignment */
        pthread_mutex_lock(&control_mutex[data->thread_id]);
        if (!data->active) break; // Exit if marked inactive

        /* Perform row elimination */
        int pivot_row = data->pivot_row;
        for (int i = data->start_row; i < data->end_row; i++) {
            double pivot_factor = A[i][pivot_row];
            for (int j = pivot_row + 1; j < N; j++) {
                A[i][j] -= pivot_factor * A[pivot_row][j];
            }
            b[i] -= pivot_factor * b[pivot_row];
            A[i][pivot_row] = 0.0; // Zero out the pivot column
        }

        /* Signal completion */
        pthread_mutex_unlock(&sync_mutex[data->thread_id]);
    }

    return NULL;
}


void Init_Matrix()
{
    int i, j;
    if (strcmp(Init, "rand") == 0) {
        for (i = 0; i < N; i++) {
            for (j = 0; j < N; j++) {
                if (i == j) /* Diagonal dominance */
                    A[i][j] = (double)(rand() % maxnum) + 5.0;
                else
                    A[i][j] = (double)(rand() % maxnum) + 1.0;
            }
        }
    } else if (strcmp(Init, "fast") == 0) {
        for (i = 0; i < N; i++) {
            for (j = 0; j < N; j++) {
                if (i == j) /* Diagonal dominance */
                    A[i][j] = 5.0;
                else
                    A[i][j] = 2.0;
            }
        }
    }

    /* Initialize vectors b and y */
    for (i = 0; i < N; i++) {
        b[i] = 2.0;
        y[i] = 1.0;
    }
}

void Print_Matrix()
{
    int i, j;
    printf("Matrix A:\n");
    for (i = 0; i < N; i++) {
        printf("[");
        for (j = 0; j < N; j++) {
            printf(" %5.2f,", A[i][j]);
        }
        printf("]\n");
    }
    printf("Vector b:\n[");
    for (j = 0; j < N; j++) {
        printf(" %5.2f,", b[j]);
    }
    printf("]\n");
    printf("Vector y:\n[");
    for (j = 0; j < N; j++) {
        printf(" %5.2f,", y[j]);
    }
    printf("]\n");
}

void Init_Default()
{
    N = 2048;
    Init = "rand";
    maxnum = 15;
    PRINT = 0;
}

int Read_Options(int argc, char **argv)
{
    char *prog = *argv;
    while (++argv, --argc > 0) {
        if (**argv == '-') {
            switch (*++*argv) {
            case 'n':
                --argc;
                N = atoi(*++argv);
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
                break;
            }
        }
    }
}
