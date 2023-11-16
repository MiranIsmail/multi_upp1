/***************************************************************************
 *
 * Sequential version of Quick sort
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define KILO (1024)
#define MEGA (1024*1024)
#define MAX_ITEMS (64*MEGA)
#define swap(v, a, b) {unsigned tmp; tmp=v[a]; v[a]=v[b]; v[b]=tmp;}
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int *v;
int NUM_THREADS = 0;
int MAX_THREADS = 0;
struct thread_data{
    int *v;
    unsigned low;
    unsigned high;
};
static void
print_array(void)
{
    int i;
    for (i = 0; i < MAX_ITEMS; i++)
        printf("%d ", v[i]);
    printf("\n");
}

static void
init_array(void)
{
    int i;
    v = (int *) malloc(MAX_ITEMS*sizeof(int));
    for (i = 0; i < MAX_ITEMS; i++)
        v[i] = rand();
}

static unsigned
partition(int *v, unsigned low, unsigned high, unsigned pivot_index)
{
    /* move pivot to the bottom of the vector */
    if (pivot_index != low)
        swap(v, low, pivot_index);

    pivot_index = low;
    low++;

    /* invariant:
     * v[i] for i less than low are less than or equal to pivot
     * v[i] for i greater than high are greater than pivot
     */

    /* move elements into place */
    while (low <= high) {
        if (v[low] <= v[pivot_index])
            low++;
        else if (v[high] > v[pivot_index])
            high--;
        else
            swap(v, low, high);
    }

    /* put pivot back between two groups */
    if (high != pivot_index)
        swap(v, pivot_index, high);
    return high;
}

static void
quick_sort(void* args)
{
    unsigned pivot_index;
    struct thread_data *main_data = (struct thread_data *) args;
    int *v = main_data->v;
    unsigned low = main_data->low;
    unsigned high = main_data->high;
    /* no need to sort a vector of zero or one element */
    if (low >= high)
        return;

    /* select the pivot value */
    pivot_index = (low+high)/2;

    /* partition the vector */
    pivot_index = partition(v, low, high, pivot_index);

    /* sort the two sub arrays */
    if (low < pivot_index){
            struct thread_data data;
            data.v = v;
            data.low = low;
            data.high = pivot_index-1;
            void *data_ptr = &data;
        if (NUM_THREADS < MAX_THREADS){
            pthread_t thread1;
            pthread_mutex_lock(&mutex);
            NUM_THREADS++;
            pthread_mutex_unlock(&mutex);
            pthread_create(&thread1, NULL, quick_sort, &data_ptr);
            pthread_join(thread1, NULL);
            pthread_mutex_lock(&mutex);
            NUM_THREADS--;
            pthread_mutex_unlock(&mutex);
        }
        else{
        quick_sort(data_ptr);
        }}
        
    if (pivot_index < high){
            struct thread_data data;
            data.v = v;
            data.low = pivot_index+1;
            data.high = high;
            struct thread_data *data_ptr = &data;
        if (NUM_THREADS < MAX_THREADS){
            pthread_t thread2;
            pthread_mutex_lock(&mutex);
            NUM_THREADS++;
            pthread_mutex_unlock(&mutex);
            pthread_create(&thread2, NULL, quick_sort, (void *)&data);
            pthread_join(thread2, NULL);
            pthread_mutex_lock(&mutex);
            NUM_THREADS--;
            pthread_mutex_unlock(&mutex);
        }
        else{
        quick_sort(data_ptr);
        }
    }
    
}

int
main(int argc, char **argv)
{
    struct thread_data main_data;
    main_data.v = v;
    main_data.low = 0;
    main_data.high = MAX_ITEMS-1;
    void *main_data_ptr = &main_data;
    if (argc > 1)
        MAX_THREADS = atoi(argv[1]);
    else
        MAX_THREADS = 0;
    init_array();
    //print_array();
    quick_sort(main_data_ptr);
    //print_array();
}
