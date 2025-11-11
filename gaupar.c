/***************************************************************************
 *
 * Sequential version of Gaussian elimination
 *
 ***************************************************************************/

#include <stdio.h>
#include <pthread.h>

#define MAX_SIZE 4096
#define NUM_THREADS 16

typedef double matrix[MAX_SIZE][MAX_SIZE];

int	N;		/* matrix size		*/
int	maxnum;		/* max number of element*/
char	*Init;		/* matrix init type	*/
int	PRINT;		/* print switch		*/
matrix	A;		/* matrix A		*/
double	b[MAX_SIZE];	/* vector b             */
double	y[MAX_SIZE];	/* vector y             */

double pivot;

int curr_phase = 0;
int count_threads = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* forward declarations */
void *work(void *args);
void work_par(void);
void Init_Matrix(void);
void Print_Matrix(void);
void Init_Default(void);
int Read_Options(int, char **);
void sync_threads();

/* Thread arguments */
typedef struct {
    int id;
} thread_args;

void
sync_threads(){
    pthread_mutex_lock(&mutex);
    int prev_phase = curr_phase;
    count_threads++;
    if(count_threads == NUM_THREADS){
        // last thread has arrived
        // we reset and move to the next phase
        count_threads = 0; 
        curr_phase++;
        pthread_mutex_unlock(&mutex);
        return;
    }
    pthread_mutex_unlock(&mutex);

    // otherwise wait until all arrive and current phase is complete

    while (1)
    {
        pthread_mutex_lock(&mutex);
        // When the phase is updated, we move on
        if(curr_phase != prev_phase){
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);
    }
    
}

int
main(int argc, char **argv)
{
    int i, timestart, timeend, iter;

    Init_Default();		/* Init default values	*/
    Read_Options(argc,argv);	/* Read arguments	*/
    Init_Matrix();		/* Init the matrix	*/
    work_par();

    if (PRINT == 1)
	   Print_Matrix();
}

void
work_par(){
    pthread_t threads[NUM_THREADS];
    thread_args targs[NUM_THREADS];

    /* Create worker threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        targs[i].id = i; // id of thread
        pthread_create(&threads[i], NULL, work, &targs[i]); // Begin work for current thread
    }

    /* Wait for all threads to complete */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    /* cleanup */
    pthread_mutex_destroy(&mutex);
}

void *work(void *arg)
{
    int i, j, k;
    int tid = ((thread_args *)arg)->id;

    /* Gaussian elimination algorithm, Algo 8.4 from Grama */
    for (k = 0; k < N; k++) {/* Outer loop */

        // thread 0 normalizes the pivot row
        if (tid == 0) {
            double piv = A[k][k]; 
            
            for (j = k+1; j < N; j++) {  
                A[k][j] = A[k][j] / piv; /* Division step */
            }
            y[k] = b[k] / piv; 
            A[k][k] = 1.0;
        }

        // wait for division to be done by threads before we move on to elimination
        sync_threads();

        // each thread eliminates a block of rows
        {
            int rows = N - (k + 1);
            if (rows > 0) {
                int chunk = (rows + NUM_THREADS - 1) / NUM_THREADS;
                int first_row = k + 1 + tid * chunk;
                int last_row = first_row + chunk;
                if (first_row < N) {
                    if (last_row > N) last_row = N;
                    for (i = first_row; i < last_row; i++) {
                        double aik = A[i][k];
                        if (aik != 0.0) {
                            for (j = k+1; j < N; j++) {
                                A[i][j] = A[i][j] - aik * A[k][j]; /* Elimination step */
                            }
                            b[i] -= b[i] - aik * y[k];
                            A[i][k] = 0.0;
                        }
                    }
                }
            }
        }
        sync_threads(); // all threads finish elimination for this k
    }
    return NULL;
}

void
Init_Matrix()
{
    int i, j;

    printf("\nsize      = %dx%d ", N, N);
    printf("\nmaxnum    = %d \n", maxnum);
    printf("Init	  = %s \n", Init);
    printf("Initializing matrix...");

    if (strcmp(Init,"rand") == 0) {
        for (i = 0; i < N; i++){
            for (j = 0; j < N; j++) {
                if (i == j) /* diagonal dominance */
                    A[i][j] = (double)(rand() % maxnum) + 5.0;
                else
                    A[i][j] = (double)(rand() % maxnum) + 1.0;
            }
        }
    }
    if (strcmp(Init,"fast") == 0) {
        for (i = 0; i < N; i++) {
            for (j = 0; j < N; j++) {
                if (i == j) /* diagonal dominance */
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

    printf("done \n\n");
    if (PRINT == 1)
        Print_Matrix();
}

void
Print_Matrix()
{
    int i, j;

    printf("Matrix A:\n");
    for (i = 0; i < N; i++) {
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

void
Init_Default()
{
    N = 2048;
    Init = "rand";
    maxnum = 15.0;
    PRINT = 0;
}

int
Read_Options(int argc, char **argv)
{
    char    *prog;

    prog = *argv;
    while (++argv, --argc > 0)
        if (**argv == '-')
            switch ( *++*argv ) {
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
                    printf("           [-D] show default values \n");
                    printf("           [-h] help \n");
                    printf("           [-I init_type] fast/rand \n");
                    printf("           [-m maxnum] max random no \n");
                    printf("           [-P print_switch] 0/1 \n");
                    exit(0);
                    break;
                case 'D':
                    printf("\nDefault:  n         = %d ", N);
                    printf("\n          Init      = rand" );
                    printf("\n          maxnum    = 5 ");
                    printf("\n          P         = 0 \n\n");
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
