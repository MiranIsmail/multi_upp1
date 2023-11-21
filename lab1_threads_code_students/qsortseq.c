/***************************************************************************
 *
 * Sequential version of Quick sort
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define KILO (1024)
#define MEGA (1024 * 1024)
#define MAX_ITEMS (64 * MEGA)
#define swap(v, a, b) \
    {                 \
        unsigned tmp; \
        tmp = v[a];   \
        v[a] = v[b];  \
        v[b] = tmp;   \
    }
static int *v;

#define NUM_THREADS 16
pthread_mutex_t mutex_q;

typedef struct
{
    int *v;
    unsigned high;
    unsigned low;
} task_info;
task_info Task_queue[NUM_THREADS * NUM_THREADS];
int task_count = 0;

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
    v = (int *)malloc(MAX_ITEMS * sizeof(int));
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
    while (low <= high)
    {
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
void add_task(task_info task)
{
    pthread_mutex_lock(&mutex_q);
    Task_queue[task_count] = task;
    task_count++;
    pthread_mutex_unlock(&mutex_q);
}
void quick_sort(int *v, unsigned low, unsigned high)
{
    task_info task1;
    task_info task2;
    unsigned pivot_index;

    /* no need to sort a vector of zero or one element */
    if (low >= high)
        return;

    /* select the pivot value */
    pivot_index = (low + high) / 2;

    /* partition the vector */
    pivot_index = partition(v, low, high, pivot_index);

    /* sort the two sub arrays */
    if (low < pivot_index)
    {
        // quick_sort(v, low, pivot_index - 1);
        task1.v = v;
        task1.high = pivot_index - 1;
        task1.low = low;
        add_task(task1);
    }
    if (pivot_index < high)
    {
        // quick_sort(v, pivot_index + 1, high);
        task2.v = v;
        task2.low = pivot_index + 1;
        task2.high = high;
        add_task(task2);
    }
}
void excute_task(task_info *task)
{
    int *v = task->v;
    unsigned int low = task->low;
    unsigned int high = task->high;
    quick_sort(v, low, high);
}

void *thread_starter(void *params)
{
    while (1)
    {
        int exists = 0;
        task_info task;
        pthread_mutex_lock(&mutex_q);
        if (task_count > 0)
        {
            exists = 1;
            task = Task_queue[0];
            int i;
            for (i = 0; i < task_count; i++)
            {
                Task_queue[i] = Task_queue[i + 1];
            }
            task_count--;
        }
        pthread_mutex_unlock(&mutex_q);
        if (exists == 1)
        {
            excute_task(&task);
        }
    }
}

int main(int argc, char **argv)
{
    pthread_mutex_init(&mutex_q, NULL);
    init_array();
    // quick_sort(v, 0, MAX_ITEMS - 1);
    task_info main_task;
    main_task.v = v;
    main_task.low = 0;
    main_task.high = MAX_ITEMS;
    add_task(main_task);
    pthread_t threads[NUM_THREADS];
    int i;
    for (i = 0; i < NUM_THREADS; i++)
    {
        if (pthread_create(&threads[i], NULL, &thread_starter, NULL) != 0)
        {
            perror("fail");
        }
    }
    for (i = 0; i < NUM_THREADS; i++)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            perror("fail2");
        }
    }
    print_array();
    pthread_mutex_destroy(&mutex_q);
}
