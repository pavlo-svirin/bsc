//
//  array_ops.c
//  quicksort
//
//  Created by Pavlo Svirin on 31.07.21.
//

#include <stdio.h>

void generate_array(int32_t *addr, unsigned int arrlen){
    srand(time(NULL));
    for(int i=0; i<arrlen; i++)
        *(addr+i) = (double)rand()/(double)RAND_MAX*100+1;
}


void read_array(char *filename, unsigned int num, int32_t *dst){
    FILE *f = fopen(filename, "r");
    for(int i=0; i<num; i++){
        fscanf(f, "%d\n", &dst[i]);
        if(dst[i]==0)
                dst[i] = (double)rand()/(double)RAND_MAX*100000+1;
    }
    fclose(f);
}

void print_array(int32_t *A, int arrlen){
    for(int i=0; i<arrlen; i++){
        printf("%d ", A[i]);
    }
    printf("\n\n");
}


void fprint_array(FILE* f, int32_t *A, int arrlen){
    for(int i=0; i<arrlen; i++){
        fprintf(f, "%d ", A[i]);
    }
    fprintf(f, "\n\n");
}
