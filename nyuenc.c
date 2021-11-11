#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>

#define ONE_GB (1024 * 1024 * 1024)

#define FOUR_KB (1024 * 4)

unsigned char global_buff[ONE_GB];

int n_jobs = -1;
int total_size = 0;

const int TEST = 1;

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

void map_files(int argc, char *argv[]) {
    // Map all specified files into memory
    // https://stackoverflow.com/questions/55928474/how-to-read-multiple-txt-files-into-a-single-buffer
    // TODO truncate files that are too large
    unsigned char *p = global_buff;
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
        // printf("%c", global_buff[i]);
        unsigned char count = 1;
        while (i < total_size - 1 && global_buff[i] == global_buff[i + 1]) {
            count++;
            i++;
        }

        print_rle(global_buff[i], count);
    }
}

void rle_parallel() {
    rle_sequential();
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        return 0;
    }

    parse_opts(argc, argv);

    map_files(argc, argv);

    if (n_jobs == -1) {
        rle_sequential();
    } else {
        // printf("Doing parallel...\n");
        rle_parallel();
    }

    if (TEST) {
        printf("\n\n");
    }

    return 0;
}
