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

unsigned char global_buff[ONE_GB];
int n_jobs = -1;

void rle_block(int start, int finish) {
    for (int i = start; i < finish; i++) {
        printf("%c", global_buff[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        return 0;
    }
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
            return 1;
            default:
            abort ();
    }

    long long total_size = 0;
    unsigned char *p = global_buff;

    // Map all specified files into memory
    // https://stackoverflow.com/questions/55928474/how-to-read-multiple-txt-files-into-a-single-buffer
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

    if (n_jobs == -1) {
        rle_block(0, total_size);
    } else {
        printf("Parallel not yet supported\n");
        exit(1);
    }

    return 0;
}
