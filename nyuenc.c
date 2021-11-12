#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

void print_rle(char c, unsigned char count);

struct Task {
    long start_i;
    long end_i;
    long task_i;
} Task;

// Static values
#define ONE_GB (1024 * 1024 * 1024)
#define TWO_GB ((long) ONE_GB * 2)
#define FOUR_KB (1024 * 4)
#define MAX_TASKS (ONE_GB / FOUR_KB)

// Global static arrays
unsigned char INPUT_FILES[ONE_GB];

// Task pickup & dropoff
struct Task TASK_PICKUP[MAX_TASKS];
long task_pickup_idx = 0;
pthread_mutex_t TASK_PICKUP_LOCK;
unsigned char TASK_DROPOFF[TWO_GB];

// Syncronization Semaphores
pthread_mutex_t TASK_PICKUP_READY[MAX_TASKS];
pthread_mutex_t TASK_DROPOFF_READY[MAX_TASKS];

// Thread ID array
pthread_t WORKERS[10];

int n_jobs = -1;
int total_size = 0;
int num_tasks_total = 0;

int tasks_available = 0;
pthread_mutex_t TASKS_AVAILABLE_LOCK;

const int TEST = 0;

// =================================================================== //
// =========================== WORKER THREAD ========================= //
// =================================================================== //
int get_next_task() {   
    // Grab the index of the next task to wait for
    pthread_mutex_lock(&TASK_PICKUP_LOCK);
    int my_task_idx = task_pickup_idx++;
    pthread_mutex_unlock(&TASK_PICKUP_LOCK);

    if (TEST) printf("\tTHREAD: Waiting for task %d to be available\n", my_task_idx);
    pthread_mutex_lock(&TASK_PICKUP_READY[my_task_idx]);
    if (TEST) printf("\tTHREAD: Task %d is available\n", my_task_idx);

    return my_task_idx;
}

void *thread_do() {
    int my_task_idx, start_i, end_i, task_i;
    while (1) {
        my_task_idx = get_next_task();

        start_i = TASK_PICKUP[my_task_idx].start_i;
        end_i = TASK_PICKUP[my_task_idx].end_i;
        task_i = TASK_PICKUP[my_task_idx].task_i;

        int dropoff_idx = start_i * 2; // Because the previous chunk could use 2x the space (if the compression is poor)
        if (TEST) printf("\tTHREAD: Beginning RLE on task %d, [%d - %d]\n", task_i, start_i, end_i);
        for (int i = start_i; i < end_i; i++) {
            unsigned char count = 1;
            while (i < end_i - 1 && INPUT_FILES[i] == INPUT_FILES[i + 1]) {
                count++;
                i++;
            }
            // Place the result
            TASK_DROPOFF[dropoff_idx] = INPUT_FILES[i];
            TASK_DROPOFF[dropoff_idx + 1] = count;

            dropoff_idx += 2; // 1 char, 1 count
        }

        if (TEST) printf("\tTHREAD: Dropped of task %d\n", task_i);
        pthread_mutex_unlock(&TASK_DROPOFF_READY[task_i]);
    }
    return NULL;
}




// =================================================================== //
// =========================== MAIN THREAD =========================== //
// =================================================================== //
void add_task(int start_i, int end_i, int i) {
    // Add a task to the queue
    TASK_PICKUP[i].start_i = start_i;
    TASK_PICKUP[i].end_i = end_i;
    TASK_PICKUP[i].task_i = i;
    pthread_mutex_unlock(&TASK_PICKUP_READY[i]);
    // if (TEST) printf("MAIN: Opened up available task at index %d\n", i);
}

//https://www.geeksforgeeks.org/mutex-lock-for-linux-thread-synchronization/
void init_mutex(pthread_mutex_t *mtx, char* err_msg, int lock) {
    if (pthread_mutex_init(mtx, NULL) != 0) {
        printf(err_msg);
        exit(1);
    }
    if (lock)
        pthread_mutex_lock(mtx);
    else
        pthread_mutex_unlock(mtx);
}

void init_semaphores() {
    init_mutex(&TASKS_AVAILABLE_LOCK, "Task Available lock init has failed\n", 0);
    init_mutex(&TASK_PICKUP_LOCK, "Task Pickup lock has failed\n", 0);

    for (int i = 0; i < num_tasks_total + 10; i++) {        
        init_mutex(&TASK_PICKUP_READY[i], "Task Pickup Mutex init has failed\n", 1);
        init_mutex(&TASK_DROPOFF_READY[i], "Task Dropoff Mutex init has failed\n", 1);
    }
}

void create_thread_pool() {
    for (int i = 0; i < n_jobs; i++) {
        pthread_create(&WORKERS[i], NULL, thread_do, NULL);
    }
}

void create_tasks() {
    int i, start_i, end_i;
    for (i = 0; i < num_tasks_total; i++) {
        start_i = i * FOUR_KB;
        end_i = MIN((i + 1) * FOUR_KB, total_size);
        add_task(start_i, end_i, i);
    }
}

void output_results() {
    unsigned char last_byte = '\0', last_count = 0;
    int task_idx = 0;
    while (task_idx < num_tasks_total) {
        if (TEST) printf("MAIN: Waiting for task %d\n", task_idx);
        pthread_mutex_lock(&TASK_DROPOFF_READY[task_idx]);
        if (TEST) printf("MAIN: Received dropped off task %d\n", task_idx);
        // Output the data
        long dropoff_idx = task_idx * FOUR_KB * 2; //Because of the potential expansion

        // Potentially merge the last byte with the new byte
        if (last_byte != '\0') {
            // The chunk border matches so let's combine the count
            if (last_byte == TASK_DROPOFF[dropoff_idx]) {
                // add the two counts together
                TASK_DROPOFF[dropoff_idx + 1] += last_count;
            } else {
                // Just print the last byte from the previous chunk
                print_rle(last_byte, last_count);
            }
        }

        while (TASK_DROPOFF[dropoff_idx + 2] != '\0') {
            print_rle(TASK_DROPOFF[dropoff_idx], TASK_DROPOFF[dropoff_idx + 1]);
            dropoff_idx+=2;
        }

        // Save the last byte from this task to merge with the next task chunk.
        last_byte = TASK_DROPOFF[dropoff_idx]; 
        last_count = TASK_DROPOFF[dropoff_idx + 1];
        dropoff_idx+=2;
        
        task_idx++;
    }

    // Finally, print the final byte
    print_rle(last_byte, last_count);
}

void rle_parallel() {
    // Calculate the number of tasks we'll need
    num_tasks_total = ceil((float)total_size / FOUR_KB);
    if (TEST) {
        printf("Num tasks = %d\n", num_tasks_total);
    }
    init_semaphores();
    create_thread_pool();
    create_tasks();
    output_results();
}



// =========================== SEQUENTIAL/HOUSEKEEPING =========================== //
void parse_opts(int argc, char *argv[]) {
    //https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
    int c;
    opterr = 0;

    while ((c = getopt (argc, argv, "j:")) != -1)
    switch (c) {
        case 'j':
            n_jobs = atoi(optarg);
            break;
        case '?':
            if (optopt == 'j')
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
            return;
            default:
            abort ();
    }
}

/**
 * Map all files into memory
 */
void map_files(int argc, char *argv[]) {
    // https://stackoverflow.com/questions/55928474/how-to-read-multiple-txt-files-into-a-single-buffer
    // TODO truncate files that are too large
    unsigned char *p = INPUT_FILES;
    for (int index = optind; index < argc; index++) {
        char* filename = argv[index];
        FILE *fp = fopen(filename, "rb");

        fseek(fp, 0, SEEK_END);
        long bytes = ftell(fp);
        fseek(fp, 0, SEEK_SET);   

        fread(p, (size_t) bytes, 1, fp);

        p+= bytes;
        total_size += bytes;

        fclose(fp);
    }
}

void print_rle(char c, unsigned char count) {
    if (TEST) {
        printf("%c%d", c, count);
    } else {
        printf("%c%c", c, count);
    }
}

// https://www.geeksforgeeks.org/run-length-encoding/
void rle_sequential() {
    for (int i = 0; i < total_size; i++) {
        unsigned char count = 1;
        while (i < total_size - 1 && INPUT_FILES[i] == INPUT_FILES[i + 1]) {
            count++;
            i++;
        }
        print_rle(INPUT_FILES[i], count);
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        return 0;
    }

    // Setup the process
    parse_opts(argc, argv);
    map_files(argc, argv);

    if (n_jobs <= 0) {
        rle_sequential();
    } else {
        rle_parallel();
    }

    if (TEST) printf("\n\n"); 
    return 0;
}
