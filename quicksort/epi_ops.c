//
//  epi_ops.c
//  quicksort
//
//  Created by Pavlo Svirin on 31.07.21.
//

#include <stdio.h>

void print_epi_8xi32(__epi_8xi32 epi, int gvl){
    int32_t *m = (int32_t*)malloc(gvl*sizeof(int32_t));
    __builtin_epi_vstore_8xi32(m, epi, gvl);
    
    for(int i=0; i<gvl; i++){
        printf("%d ", m[i]);
    }
    printf("\n\n");
    free(m);
}



void print_mask(__epi_8xi1 mask, int gvl){
    __epi_8xi8 mask_array = __builtin_epi_cast_8xi8_8xi1(mask);
    signed char *m = (signed char*)malloc(gvl*sizeof(char));
    __builtin_epi_vstore_8xi8(m, mask_array, gvl);
    
    for(int i=0; i<gvl; i++){
        printf("%d", m[i]);
    }
    printf("\n\n");
    free(m);
}



__epi_8xi1 vnot(__epi_8xi1 mask, unsigned int gvl){
    __epi_8xi8 xor_vector = __builtin_epi_vbroadcast_8xi8(1, gvl);
    __epi_8xi1 xor_mask = __builtin_epi_cast_8xi1_8xi8(xor_vector);

    return  __builtin_epi_vmxor_8xi1(mask, xor_mask, gvl);
}
