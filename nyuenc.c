#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>

void rle_block(char* start, off_t len) {
    for (long long i = 0; i < len; i++) {
            printf("%c", start[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        return 0;
    }
    //https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
    int n_jobs = -1;
    int index;
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
            return 1;
            default:
            abort ();
    }

    // printf ("jflag = %d\n", n_jobs);

    int fd;
    struct stat sb;
    off_t total_size = 0, last_size = 0;
    char *global_mem_pt = NULL;

    // Map all specified files into memory
    for (index = optind; index < argc; index++) {
        char* filename = argv[index];
        
        if ((fd = open(filename, O_RDONLY, S_IRUSR | S_IWUSR)) == -1) {
            perror("Couldn't open file");
            exit(1);
        }

        if (fstat(fd, &sb) == -1) {
            perror("Couldn't get file size\n");
        }

        if (global_mem_pt == NULL) {
            global_mem_pt = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
            if (global_mem_pt == MAP_FAILED) {
                perror("Map failed");
                exit(1);
            }
        } else {
            printf("Performing secondary mmap\n");
            // Allocate alongside the last mmap
            if (mmap(global_mem_pt + last_size, sb.st_size, PROT_READ, MAP_SHARED, fd, 0) == MAP_FAILED) {
                perror("Secondary map failed");
                exit(1);
            }
        }

        last_size = sb.st_size;
        // printf("Last size = " + )
        total_size += last_size;

        close(fd);
    }

    if (n_jobs == -1) {
        rle_block(global_mem_pt, total_size);
    } else {
        printf("Parallel not yet supported\n");
        exit(1);
    }

    return 0;
}
