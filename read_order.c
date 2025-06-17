#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char ** argv)
{
    if(argc != 3)
    {
        puts("Usage: read_order <n> <order>\n");
        return 1;
    }

    const unsigned COLUMNS = (unsigned)atol(argv[1]);

    if(COLUMNS == 0)
        return 1;
    
    int fdin = open(argv[2], O_RDONLY);
    
    unsigned * order = (unsigned*)malloc(sizeof(unsigned)*COLUMNS);

    read(fdin, (char*)order, sizeof(unsigned)*COLUMNS);
    
    unsigned i;
    for(i = 0; i + 1 < COLUMNS; ++i)
        printf("%u,", order[i]);
    printf("%u", order[COLUMNS-1]);
    putchar('\n');

    close(fdin);
    free(order);
}
