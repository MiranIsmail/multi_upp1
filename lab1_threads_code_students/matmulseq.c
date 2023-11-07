/***************************************************************************
 *
 * Sequential version of Matrix-Matrix multiplication
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define SIZE 2048
#define NUM_CORES 6


static double a[SIZE][SIZE];
static double b[SIZE][SIZE];
static double c[SIZE][SIZE];

struct matmulseq_args
{
    int i[SIZE/NUM_CORES];
};

void* init_thread(void* params){
    int j,k,q;
    struct matmulseq_args *args = (struct matmulseq_args*) params;
    int i[SIZE/NUM_CORES];

    for (k = 0; k < SIZE/NUM_CORES; k++)
    {
        i[k] = args->i[k];
    }

    for ( q = 0; q < SIZE/NUM_CORES; q++)
    {
        for (j = 0; j < SIZE; j++) {
        /* Simple initialization, which enables us to easy check
            * the correct answer. Each element in c will have the same
            * value as SIZE after the matmul operation.
            */
        a[i[q]][j] = 1.0;
        b[i[q]][j] = 1.0;}
    }
}


static void
init_matrix(void)
{
    int i, j;
    int m = 0;
    pthread_t *children_init;
    struct matmulseq_args *args_init;
    args_init = malloc( NUM_CORES * sizeof(struct matmulseq_args) );
    children_init = malloc( NUM_CORES * sizeof(pthread_t) );

    for (i = 0; i < NUM_CORES; i++) {
        for (j = 0; j < SIZE/NUM_CORES; j++)
            args_init[i].i[j] = m;
            m++;
    }

    for (i = 0; i < NUM_CORES; i++)
        pthread_create(&children_init[i],NULL,init_thread,(void*)&args_init[i]);

    for (i = 0; i < NUM_CORES; i++)
        pthread_join(children_init[i], NULL);

    free(children_init);
    free(args_init);
}

void* calc_thread(void* params){
    int j, k,q;
    struct matmulseq_args *args = (struct matmulseq_args*) params;
    int i[SIZE/NUM_CORES];
    for (k = 0; k < SIZE/NUM_CORES; k++)
    {
        i[k] = args->i[k];
    }

    for ( q = 0; q < SIZE/NUM_CORES; q++)
    {
    
    for (j = 0; j < SIZE; j++) {
        c[i[q]][j] = 0.0;
        for (k = 0; k < SIZE; k++)
            c[i[q]][j] = c[i[q]][j] + a[i[q]][k] * b[k][j];
    }
    }
}

static void
matmul_seq()
{
    int i, j, k,q;
    int m = 0;
    pthread_t *children;
    struct matmulseq_args *args;
    args = malloc( NUM_CORES * sizeof(struct matmulseq_args) );
    children = malloc( NUM_CORES * sizeof(pthread_t) );

    for (i = 0; i < NUM_CORES; i++) {
        for ( q = 0; q < SIZE/NUM_CORES; q++)
        {
            args[i].i[q] = m;
            m++;    
        }
        
        ///thread begins here with i as input
        
       
    }
    for ( i = 0; i < NUM_CORES; i++)
    {
        pthread_create(&(children[i]), NULL, calc_thread, (void*)&args[i]);
    }
    
    for (i = 0; i < NUM_CORES; i++) {
        pthread_join(children[i], NULL);
    }
    free(children);
    free(args);
}

static void
print_matrix(void)
{
    int i, j;

    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++)
	        printf(" %7.2f", c[i][j]);
	    printf("\n");
    }
}

int
main(int argc, char **argv)
{
    init_matrix();
    matmul_seq();
    //print_matrix();
}
