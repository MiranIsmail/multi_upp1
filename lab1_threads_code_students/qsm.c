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
/*****************************************************************/
typedef struct Task
{
    unsigned low;
    unsigned high;
    int *v;

} Task;
Task task_q[KILO];
int task_count = 0;
int array_sorted = 1;
pthread_mutex_t taking_q_mutex;
pthread_mutex_t v_mod;
pthread_cond_t cond_q;
#define MAX_THREADS 32
void check_and_print_if_sorted(int *v, int size)
{
    int is_sorted = 1;
    for (int i = 0; i < size - 1; i++)
    {
        if (v[i] > v[i + 1])
        {
            is_sorted = 0;
            break;
        }
    }
    if (is_sorted)
    {
        int i;
        for (i = 0; i < MAX_ITEMS; i++)
            printf("%d ", v[i]);
        printf("\n");
    }
}
void add_task(Task task)
{
    pthread_mutex_lock(&taking_q_mutex);
    task_q[task_count] = task;
    task_count++;
    pthread_mutex_unlock(&taking_q_mutex);
    pthread_cond_signal(&cond_q);
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
        v[i] = rand() % 100;
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
    pthread_mutex_lock(&v_mod);
    pivot_index = partition(v, low, high, pivot_index);
    pthread_mutex_unlock(&v_mod);

    /* sort the two sub arrays */
    if (low < pivot_index)
    {
        task_left.v = v;
        task_left.low = low;
        task_left.high = pivot_index - 1;
        add_task(task_left);
        // quick_sort(v, low, pivot_index - 1);
    }

    if (pivot_index < high)
    {
        task_right.v = v;
        task_right.low = pivot_index + 1;
        task_right.high = high;
        add_task(task_right);
        // quick_sort(v, pivot_index + 1, high);
    }
    array_sorted = check_if_sorted(v, MAX_ITEMS);
}
int check_if_sorted(int *v, int size)
{
    for (int i = 0; i < size - 1; i++)
    {
        if (v[i] > v[i + 1])
        {
            return 1; // Return 1 if the array is not sorted
        }
    }
    return 0; // Return 0 if the array is sorted
}
void *start_thread(void *args)
{
    while (array_sorted)
    {
        Task task;
        pthread_mutex_lock(&taking_q_mutex);
        while (task_count == 0)
        {
            pthread_cond_wait(&cond_q, &taking_q_mutex);
        }
        task = task_q[0];
        int i;
        for (i = 0; i < task_count - 1; i++)
        {
            task_q[i] = task_q[i + 1];
        }
        task_count--;

        pthread_mutex_unlock(&taking_q_mutex);

        exe_task(&task);
    }
}
int main(int argc, char **argv)
{
    pthread_mutex_init(&taking_q_mutex, NULL);
    pthread_mutex_init(&v_mod, NULL);
    pthread_cond_init(&cond_q, NULL);
    pthread_t threads[MAX_THREADS];
    init_array();
    Task task;
    task.high = MAX_ITEMS - 1;
    task.low = 0;
    task.v = v;
    add_task(task);
    int i;
    for (i = 0; i < MAX_THREADS; i++)
    {
        if (pthread_create(&threads[i], NULL, &start_thread, NULL) != 0)
        {
            perror("error");
        }
    }
    for (i = 0; i < MAX_THREADS; i++)
    {
        if (pthread_join(threads[i], NULL))
        {
            perror("error");
        }
    }

    // quick_sort(v, 0, MAX_ITEMS - 1);
    //  print_array();
    pthread_mutex_destroy(&taking_q_mutex);
    pthread_mutex_destroy(&v_mod);
    pthread_cond_destroy(&cond_q);
    // print_array();
}
