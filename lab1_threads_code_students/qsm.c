/***************************************************************************
 *
 * "multithreaded qs with threadpool time mesuared inside 1,8s without init_array()"
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define KILO (1024)
#define MEGA (1024 * 1024)
#define MAX_ITEMS (MEGA * 64)
#define swap(v, a, b) \
    {                 \
        unsigned tmp; \
        tmp = v[a];   \
        v[a] = v[b];  \
        v[b] = tmp;   \
    }

static int *v;
/*****************************************************************/
#define NUM_TASKS 16
#define MAX_THREADS 32
#define GRANULARITY 200000
typedef struct Task
{
    unsigned low;
    unsigned high;
    int *v;

} Task;
Task task_q[NUM_TASKS];
int task_count = 0;
pthread_mutex_t taking_q_mutex;

pthread_mutex_t no_task_mutex;
pthread_cond_t no_task_cond;
int num_of_threads_running = MAX_THREADS;

void add_task(Task task)
{
    task_q[task_count] = task;
    task_count++;
    pthread_cond_signal(&no_task_cond);
}
/*****************************************************************/

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

void exe_task(Task *task)
{
    unsigned low = task->low;
    unsigned high = task->high;
    int *v = task->v;
    Task task_left;
    Task task_right;
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
        task_left.v = v;
        task_left.low = low;
        task_left.high = pivot_index - 1;
        if (task_count < NUM_TASKS - 1 && task_left.high - task_left.low > GRANULARITY)
        {
            pthread_mutex_lock(&taking_q_mutex);
            add_task(task_left);
            pthread_mutex_unlock(&taking_q_mutex);
        }
        else
        {
            exe_task(&task_left);
        }
    }

    if (pivot_index < high)
    {
        task_right.v = v;
        task_right.low = pivot_index + 1;
        task_right.high = high;
        if (task_count < NUM_TASKS - 1 && task_right.high - task_right.low > GRANULARITY)
        {
            pthread_mutex_lock(&taking_q_mutex);
            add_task(task_right);
            pthread_mutex_unlock(&taking_q_mutex);
        }
        else
        {
            exe_task(&task_right);
        }
    }
}
int check_if_sorted(int *v, int size)
{
    for (int i = 0; i < size - 1; i++)
    {
        if (v[i] > v[i + 1])
        {
            return 0; // Return 0 if the array is not sorted
        }
    }
    return 1; // Return 1 if the array is sorted
}
void *start_thread()
{
    Task task;
    while (1)
    {
        pthread_mutex_lock(&taking_q_mutex);
        if (task_count > 0)
        {
            task = task_q[0];
            for (int i = 0; i < task_count - 1; i++)
            {
                task_q[i] = task_q[i + 1];
            }
            task_count--;
            pthread_mutex_unlock(&taking_q_mutex);
            exe_task(&task);
        }
        else
        {

            pthread_mutex_unlock(&taking_q_mutex);
            pthread_mutex_lock(&no_task_mutex);
            num_of_threads_running--;
            if (num_of_threads_running == 0)
            {
                pthread_mutex_unlock(&no_task_mutex);
                pthread_cond_broadcast(&no_task_cond);
                return NULL;
            }
            pthread_cond_wait(&no_task_cond, &no_task_mutex);
            if (num_of_threads_running == 0)
            {
                pthread_mutex_unlock(&no_task_mutex);
                return NULL;
            }
            num_of_threads_running++;
            pthread_mutex_unlock(&no_task_mutex);
        }
    }

    return NULL;
}

int main(int argc, char **argv)
{
    struct timeval start, end;
    long mtime, seconds, useconds;

    init_array();
    gettimeofday(&start, NULL);
    pthread_mutex_init(&taking_q_mutex, NULL);
    pthread_mutex_init(&no_task_mutex, NULL);
    pthread_cond_init(&no_task_cond, NULL);

    pthread_t threads[MAX_THREADS];
    Task task;
    task.high = MAX_ITEMS - 1;
    task.low = 0;
    task.v = v;
    add_task(task);
    int i;

    for (i = 0; i < MAX_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, &start_thread, NULL);
    }
    ////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////
    for (i = 0; i < MAX_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&taking_q_mutex);
    pthread_mutex_destroy(&no_task_mutex);
    pthread_cond_destroy(&no_task_cond);

    gettimeofday(&end, NULL);
    seconds = end.tv_sec - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;
    mtime = ((seconds) * 1000 + useconds / 1000.0) + 0.5;
    printf("Elapsed time: %ld milliseconds\n", mtime);
    // if (check_if_sorted(v, MAX_ITEMS))
    // {
    //     printf("sorted");
    // }
}
