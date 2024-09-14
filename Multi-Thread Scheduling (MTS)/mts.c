#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>

#define LOADING 0
#define READY 1
#define CROSSING 2
#define FINISHED 3
#define NANOS_CONVERSION 1e9

pthread_mutex_t start_timer;
pthread_mutex_t cross_rail;
pthread_mutex_t add_to_queue;
pthread_cond_t train_ready_to_load;
pthread_cond_t train_ready_to_cross;
pthread_cond_t next_train_to_cross;
bool ready_to_load = false;
bool rail_busy = false;
struct train *train_queue = NULL;    // Initialize the train queue
struct train *crossing_train = NULL; // The train that's currently crossing

struct timespec start_time = {0};

double timespec_to_sec(struct timespec *ts)
{
    return ((double)ts->tv_sec) + ((double)ts->tv_nsec) / NANOS_CONVERSION;
}

struct train
{
    int num;
    int dir;
    int state;
    int priority;
    int lt;
    int ct;
    struct train *next;
};

// Function to print the simulation time in "00:00:00:00" format
void print_simulation_time(struct timespec current_time)
{
    double simulation_time = timespec_to_sec(&current_time) - timespec_to_sec(&start_time);
    int hours = (int)(simulation_time / 3600);
    int minutes = (int)((simulation_time - (hours * 3600)) / 60);
    int seconds = (int)(simulation_time) % 60;
    int deciseconds = (int)((simulation_time - (int)simulation_time) * 10);

    printf("%02d:%02d:%02d.%01d ", hours, minutes, seconds, deciseconds);
}

// make train structure
void initialize_train(struct train *train, int num, char dir, int loading_time, int crossing_time)
{
    train->num = num;
    if (dir == 'w' || dir == 'W')
    {
        train->dir = 0;
    }
    else
    {
        train->dir = 1;
    }
    train->lt = loading_time * 100000;
    train->ct = crossing_time * 100000;
    train->state = LOADING;
    train->next = NULL;
    if (isupper(dir))
    {
        train->priority = 1;
    }
    else
    {
        train->priority = 0;
    }
}

void start_trains(void)
{
    // Initialize start_time when starting the trains
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    pthread_mutex_lock(&start_timer);
    ready_to_load = true;
    pthread_cond_broadcast(&train_ready_to_load);
    pthread_mutex_unlock(&start_timer);
}

// figure out the number of trains
int count_lines(FILE *fp)
{
    int counter = 0;
    char dir[256];
    int loading_time, crossing_time;
    while (fscanf(fp, " %s %d %d", dir, &loading_time, &crossing_time) == 3)
    {
        counter++;
    }
    // printf("There are %d lines!\n", counter);
    rewind(fp);
    return counter;
}

// add trains to priority queue (not yet finished, one thing left)
void add_train_to_queue(struct train **head, struct train *t)
{
    if (*head == NULL || t->priority > (*head)->priority)
    {
        t->next = *head;
        *head = t;
    }
    else
    {
        struct train *prev = NULL;
        struct train *cur = *head;
        int same_direction_count = 0;

        while (cur != NULL && t->priority == cur->priority)
        {
            if (t->dir == cur->dir)
            {
                if (t->state < cur->state)
                { // 4a
                    break;
                }
                else if (t->state == cur->state)
                {
                    if (t->num < cur->num)
                    {
                        break;
                    }
                }
                prev = cur;
                cur = cur->next;
            }
            else
            {
                // Check priority 4b
                struct train *last_train = *head;
                while (last_train->next != NULL)
                {
                    last_train = last_train->next;
                }
                if (last_train->dir == 0 && t->dir == 1)
                {
                    break; // Last train went Westbound, so Eastbound train has priority
                }
                prev = cur;
                cur = cur->next;
            }
            same_direction_count++;
            if (same_direction_count >= 3)
            {
                // take a train from opposite direction
                struct train *opposite_train = *head;
                while (opposite_train != NULL && opposite_train->dir == t->dir)
                {
                    opposite_train = opposite_train->next;
                }

                if (opposite_train != NULL)
                {
                    t->next = opposite_train;
                    return; // Dispatched, no need to continue
                }
            }
        }
        if (prev == NULL)
        {
            t->next = *head;
            *head = t;
        }
        else
        {
            t->next = prev->next;
            prev->next = t;
        }
    }
}


void *train_thread(void *arg)
{
    struct train *train = (struct train *)arg;
    char *dir_str = (train->dir == 1) ? "East" : "West";

    pthread_mutex_lock(&start_timer);
    while (!ready_to_load)
    {
        pthread_cond_wait(&train_ready_to_load, &start_timer);
    }
    pthread_mutex_unlock(&start_timer);

    struct timespec load_time = {train->lt};
    clock_gettime(CLOCK_MONOTONIC, &load_time);
    usleep(train->lt); // Simulate loading
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    print_simulation_time(current_time);
    printf("Train   %d is ready to go %s\n", train->num, dir_str);
    train->state = READY;

    pthread_mutex_lock(&add_to_queue);
    add_train_to_queue(&train_queue, train);
    pthread_mutex_unlock(&add_to_queue);

    // Use a condition variable to signal the next train to cross
    pthread_mutex_lock(&cross_rail);
    if (crossing_train != NULL)
    {
        // Wait if there is a train currently crossing
        pthread_cond_wait(&next_train_to_cross, &cross_rail);
    }
    crossing_train = train;
    pthread_mutex_unlock(&cross_rail);

    train->state = CROSSING;
    struct timespec crossing_start_time;
    clock_gettime(CLOCK_MONOTONIC, &crossing_start_time);
    print_simulation_time(crossing_start_time);
    printf("Train   %d is ON the main track going %s\n", train->num, dir_str);
    usleep(train->ct);
    struct timespec crossing_end_time;
    clock_gettime(CLOCK_MONOTONIC, &crossing_end_time);
    print_simulation_time(crossing_end_time);
    printf("Train   %d is OFF the main track after going %s\n", train->num, dir_str);
    train->state = FINISHED;

    pthread_mutex_lock(&cross_rail);
    crossing_train = NULL;
    // Signal the next train to cross
    pthread_cond_signal(&next_train_to_cross);
    pthread_mutex_unlock(&cross_rail);
    pthread_exit(NULL);
}

void *train_thread_wrapper(void *arg)
{
    struct train *train = (struct train *)arg;
    train_thread(train);
    return NULL;
}

void make_train_threads(FILE *fp, int num)
{
    char dir[256];
    int loading_time, crossing_time;
    int line_num = 0;
    pthread_t tid;
    pthread_t thread_ids[num];

    while (fscanf(fp, "%s %d %d", dir, &loading_time, &crossing_time) != EOF)
    {
        struct train *train_thread = (struct train *)malloc(sizeof(struct train));
        initialize_train(train_thread, line_num, dir[0], loading_time, crossing_time);

        struct timespec create_time = {0};
        clock_gettime(CLOCK_MONOTONIC, &create_time);

        int result = pthread_create(&tid, NULL, train_thread_wrapper, train_thread);
        if (result != 0)
        {
            fprintf(stderr, "Failed to create thread %d\n", line_num);
            exit(1);
        }
        thread_ids[line_num] = tid;
        line_num++;
    }

    // Start all the trains at the same time
    sleep(1);
    start_trains();

    pthread_mutex_lock(&cross_rail);
    rail_busy = false;
    pthread_mutex_unlock(&cross_rail);

    while (train_queue != NULL)
    {

        // Select the next train to cross (train with the highest priority)
        struct train *next_train = train_queue;

        pthread_mutex_lock(&cross_rail);
        if (crossing_train == NULL)
        {
            // If no train is currently crossing, let the next train cross
            crossing_train = next_train;
            next_train = next_train->next;
            pthread_cond_signal(&next_train_to_cross);
        }
        pthread_mutex_unlock(&cross_rail);

        if (crossing_train != NULL)
        {
            // Wait for the next train to cross
            pthread_mutex_lock(&cross_rail);
            pthread_cond_wait(&next_train_to_cross, &cross_rail);
            pthread_mutex_unlock(&cross_rail);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < num; i++)
    {
        pthread_join(thread_ids[i], NULL);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "No input file given: %s\n", argv[0]);
        return 1;
    }
    char *filename = argv[1];
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("Error opening the file");
        return 1;
    }
    int num = count_lines(fp);
    make_train_threads(fp, num);
    fclose(fp);
    return 0;
}