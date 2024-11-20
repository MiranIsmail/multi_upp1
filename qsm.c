/***************************************************************************
 *
 * "multithreaded quicksort with threadpool time mesuared outside with "time" 2,5s"
 * if we omit the init_array() the time is roughly 1,7s
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
#define NUM_TASKS 32       // how many tasks can be in the queue this affects the granularity somewhat but not in a big way
#define MAX_THREADS 32     // how many threads can be running at the same time
#define GRANULARITY 200000 // this is the minimum size of a task if a task is smaller than this it will be executed by the thread that took it
typedef struct Task
{
    unsigned low;
    unsigned high;
    int *v;

} Task; // this is the struct that holds the task args
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
} // this function adds a task to the queue and signals the threads that there is a new task in the queue
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
} // this function executes a task and if the task is bigger than the granularity it will add the subtasks to the queue otherwise it will execute the subtasks itself
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
} // this function checks if the array is sorted if it is it returns 1 otherwise it returns 0
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
            if (num_of_threads_running == 0) // if there is no task in the queue and no threads running it will exit. this uses a mutex and a condition variable
            {
                pthread_mutex_unlock(&no_task_mutex);
                pthread_cond_broadcast(&no_task_cond);
                return NULL;
            }
            pthread_cond_wait(&no_task_cond, &no_task_mutex);
            //if (num_of_threads_running == 0)
            //{
            //    pthread_mutex_unlock(&no_task_mutex);
            //    return NULL;
            //}
            num_of_threads_running++;
            pthread_mutex_unlock(&no_task_mutex);
        }
    }

    return NULL;
} // this function is the function that the threads run it takes a task from the queue and executes it if there is no task in the queue it will wait for a signal from the main thread
  // and if there is no task in the queue and no threads running it will exit

int main(int argc, char **argv)
{
    //struct timeval start, end;
    //long mtime, seconds, useconds;

    init_array();
    //gettimeofday(&start, NULL);
    pthread_mutex_init(&taking_q_mutex, NULL);
    pthread_mutex_init(&no_task_mutex, NULL);
    pthread_cond_init(&no_task_cond, NULL);

    pthread_t threads[MAX_THREADS];
    Task task;
    task.high = MAX_ITEMS - 1;
    task.low = 0;
    task.v = v;
    add_task(task); // add the array to the queue as a task
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

    //gettimeofday(&end, NULL);
    //seconds = end.tv_sec - start.tv_sec;
    //useconds = end.tv_usec - start.tv_usec;
    //mtime = ((seconds) * 1000 + useconds / 1000.0) + 0.5;
    //if (check_if_sorted(v, MAX_ITEMS))
    //{
    //    printf("sorted\n");
    //}
    //printf("Elapsed time: %ld milliseconds\n", mtime);
}
