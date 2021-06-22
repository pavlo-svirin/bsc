#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define ARRLEN 500
#define VL 8

//int32_t array[] = {3,7,17,8,5,2,1,9,5,4};
int32_t array[ARRLEN]; 

void quicksort_vectorized(int*, int, int);
int partition_vectorized(int*, int, int);
void print_array(int32_t*, int);


__epi_8xi1 vnot(__epi_8xi1 mask, unsigned int gvl);

int store_arrays(int *address, unsigned int lo, unsigned int hi, __epi_8xi32 vec1, __epi_8xi32 vec2, 
                       __epi_8xi1 mask1, __epi_8xi1 mask2, 
                       int gvl1, int gvl2, int pivot);

void vswap(int32_t *address,
		__epi_8xi32 a1, __epi_8xi32 a2,
	       	__epi_8xi32 b1,  __epi_8xi32 b2,
		unsigned int lo, unsigned int j, 
		unsigned  int num_to_swap,
		unsigned int vector_length);



void generate_array(int32_t *addr, unsigned int arrlen){
    for(int i=0; i<arrlen; i++)
        *(addr+i) = (double)rand()/(double)RAND_MAX*100;
}



int main(){
    //quicksort(array, 0, ARRLEN-1);
    generate_array(array, ARRLEN);
    print_array(array, ARRLEN);

    return 0;

    quicksort_vectorized(array, 0, ARRLEN-1);

    //partition_vectorized(array, 0, ARRLEN-1);

    printf("Result: ");
    print_array(array, ARRLEN);

    return 0;
}


void quicksort_vectorized(int *A, int lo, int hi){
    if(lo<hi){
       int p = partition_vectorized(A, lo, hi);
       quicksort_vectorized(A, lo, p-1);
       quicksort_vectorized(A, p+1, hi);
    }
}


int partition_vectorized(int *A, int lo, int hi){
    int32_t pivot = A[hi];
    int j = hi;
    int i = 0;

    //int *vec1 = (int*) malloc(VL*sizeof(int));
    //int *vec2 = (int*) malloc(VL*sizeof(int));
    //int *result1 = (int*) malloc(VL*sizeof(int));
    //int *result2 = (int*) malloc(VL*sizeof(int));

    int k = 0;

    //while(j>lo){
    while(1){
        //printf("\n=== BEGIN \n==== j: %d |  lo: %d | hi: %d ====\n", j, lo, hi);
      
        // define vector lengths
        int vector_length = VL;
        
        if(j==lo){
            break;
        }
        if(j==lo+1){
            if(A[j]<A[lo]){
		//printf("::: we do the swap :::\n");
                int tmp = A[j];
                A[j] = A[lo];
                A[lo] = tmp;
				j--;
        		print_array(A, ARRLEN);
            }
			lo++;
            break;
        }
        else if(j-lo<2*VL)
            vector_length = (j-lo)/2;

        j -= vector_length;
	__epi_8xi32 pivot_vec = __builtin_epi_vbroadcast_8xi32(pivot, vector_length);
 	
	/* __epi_8xi32 __builtin_epi_vload_32xi16(const signed short int *address, unsigned long int gvl); */
        /* vload(vec1, A+j, vector_length); */
	__epi_8xi32 vec1 = __builtin_epi_vload_8xi32(A+j, vector_length);

        //print_array(vec1, vector_length);
        /* vmslt(result1, vec1, pivot, vector_length); */

	__epi_8xi1 result1 = __builtin_epi_vmslt_8xi32(vec1, pivot_vec, vector_length);

        // read vec2
        j -= vector_length;


	/* __epi_8xi32 __builtin_epi_vload_32xi16(const signed short int *address, unsigned long int gvl); */
        /* vload(vec2, A+j, vector_length); */
	__epi_8xi32 vec2 = __builtin_epi_vload_8xi32(A+j, vector_length);
        //print_array(vec2, vector_length );
        // test vec1

 	/* __epi_32xi1 __builtin_epi_vmslt_32xi16(__epi_32xi16 a, __epi_32xi16 b, unsigned long int gvl); */
        /* vmslt(result2, vec2, pivot, vector_length); */
	__epi_8xi1 result2 = __builtin_epi_vmslt_8xi32(vec2, 
							pivot_vec,
			                                vector_length);

        // now we store the data
        k = store_arrays(A+j, lo, hi, vec1, vec2, result1, result2, vector_length, vector_length, pivot);

        // here we do swap
        //vswap(A+lo, A+j, k);

        /*
        src = __builtin_epi_vxor_32xi16(*src, *dst, gvl);
        dst = __builtin_epi_vxor_32xi16(*dst, *src, gvl);
        src = __builtin_epi_vxor_32xi16(*src, *dst, gvl);
        */

        lo += k;

	printf("Loop result: ");
        print_array(A, ARRLEN);

        printf("=== j: %d , vector length: %d lo: %d  pivot pos: %d\n", j, vector_length, lo, k);

        //if(j==lo)
        //    break;
        //else
        j += k;
            //j--;


        // for debugging
        //break;
        //i++;
        //if(i>5) break;
    }

	printf("_______________ we return: %d _____________________", j);

    return j;
}


void print_array(int32_t *A, int arrlen){
    for(int i=0; i<arrlen; i++){
        printf("%d ", A[i]);
    }
    printf("\n\n");
}


/*
MERGE rd, a b, mask

for element = 0 to VLMAX
   if mask[element] then
     result[element] = b[element]
   else
     result[element] = a[element]
*/


void vmerge(int *result, int *a, int* b, int *mask, int VLMAX){
    for(int i=0; i<VLMAX; i++)
       if(mask[i])
         result[i] = b[i];
       else
         result[i] = a[i];
}





__epi_8xi1 vnot(__epi_8xi1 mask, unsigned int gvl){
    __epi_8xi8 xor_vector = __builtin_epi_vbroadcast_8xi8(1, gvl);
    __epi_8xi1 xor_mask = __builtin_epi_cast_8xi1_8xi8(xor_vector);

    return  __builtin_epi_vmxor_8xi1(mask, xor_mask, gvl);
}






int store_arrays(int *address, 
                    unsigned int lo,
                    unsigned int hi,
                    __epi_8xi32 vec1, 
                    __epi_8xi32 vec2, 
                    __epi_8xi1 mask1, 
                    __epi_8xi1 mask2, 
                    int gvl1, 
                    int gvl2, 
                    int pivot){
    // to init v1
    //int* v1 = malloc(gvl1*sizeof(int));
    //int* nmask1 = malloc(gvl1*sizeof(int));
    // to init v2
    //int* v2 = malloc(gvl2*sizeof(int));
    //int* nmask2 = malloc(gvl2*sizeof(int));

    // store lessers
    //vec1:

    __epi_8xi32 v1 = __builtin_epi_vcompress_8xi32(vec1, mask1, gvl1);

    //vcompress(v1, vec1, mask1, gvl1);
   // int pc1 = vpopc(mask1, gvl1);
    unsigned int pc1 = __builtin_epi_vmpopc_8xi1(mask1, gvl1);
    // store greaters
    //vstore_strided(address, v1, 1, pc1);

    //vec2:
    __epi_8xi32 v2 = __builtin_epi_vcompress_8xi32(vec2, mask2, gvl2);
    //vcompress(v2, vec2, mask2, gvl2);
    //int pc2 = vpopc(mask2, gvl2);
    unsigned int pc2 = __builtin_epi_vmpopc_8xi1(mask2, gvl2);
    //vstore_strided(address+pc1, v2, 1, pc2);


    printf("====== array: ");
    print_array(array, ARRLEN);
    printf("\n");

    // store pivot
    int *pivot_position = address + pc1 + pc2;
    *pivot_position = pivot;

    int *greater_start = pivot_position+1;

    // store greaters
    //vec1:
    //__epi_8xi8 xor_arg = __builtin_epi_vbroadcast_8xi8(0xFF, 8);
    __epi_8xi1 nmask1 = vnot(mask1,  gvl1);
     
    //src = __builtin_epi_vxor_32xi16(*src, *dst, gvl);
    v1 = __builtin_epi_vcompress_8xi32(vec1, nmask1, gvl1);
    //vcompress(v1, vec1, nmask1, gvl1);
    //__epi_8xi32 v2 = __builtin_epi_vcompress_8xi32(vec2, nmask2, gvl2);
    //vstore_strided(greater_start, v1, 1, gvl1-pc1);
    //free(v1);
    //free(nmask1);

    //vec2:
    __epi_8xi1 nmask2 = vnot(mask2, gvl2);
    v2 = __builtin_epi_vcompress_8xi32(vec2, nmask2, gvl2);
    //vcompress(v2, vec2, nmask2, gvl2);
    //vstore_strided(greater_start+gvl1-pc1, v2, 1, gvl2-pc2);
    
    // read vectors from beginning
    __epi_8xi32 v3 = __builtin_epi_vload_8xi32(address+lo, gvl1);
    __epi_8xi32 v4 = __builtin_epi_vload_8xi32(address+lo+gvl1, gvl2);

    // and here we store the data
    vswap(address,
		v1, v3,
	       	v2,  v4,
		lo, hi, 
		pc1+pc2,
		gvl1+gvl2);

    return pc1+pc2;
}



void vswap(int32_t *address,
		__epi_8xi32 a1, __epi_8xi32 a2,
	       	__epi_8xi32 b1,  __epi_8xi32 b2,
		unsigned int lo, unsigned int j, 
		unsigned  int num_to_swap,
		unsigned int vector_length) {
    
    int to_swap = num_to_swap>vector_length ? vector_length : num_to_swap;
    //a = __builtin_epi_vload_8xi32(a1, to_swap);
    //__epi_8xi32 a0 = __builtin_epi_vload_8xi32(a2, to_swap);

	a1  = __builtin_epi_vxor_8xi32(a1, b1, to_swap);
	b1 = __builtin_epi_vxor_8xi32(b1, a1, to_swap);
	a1  = __builtin_epi_vxor_8xi32(a1, b1, to_swap);

	__builtin_epi_vstore_8xi32(address+lo, a1,  to_swap);
	__builtin_epi_vstore_8xi32(address+j, b1, to_swap);

    if(num_to_swap>vector_length){
	    to_swap = num_to_swap - vector_length;
	    //__epi_8xi32 a = __builtin_epi_vload_8xi32(a1 + vector_length,  to_swap);
	    //__epi_8xi32 a0 = __builtin_epi_vload_8xi32(a2 + vector_length, to_swap);

	    a2  = __builtin_epi_vxor_8xi32(a2, b2, to_swap);
	    b2 = __builtin_epi_vxor_8xi32(b2, a2, to_swap);
	    a2  = __builtin_epi_vxor_8xi32(a2, b2, to_swap);

	__builtin_epi_vstore_8xi32(address+vector_length, a2, to_swap);
	__builtin_epi_vstore_8xi32(address+vector_length+j, b2, to_swap);

    }
}





/*
 * INTRINSICS TO BE USED for epi_32xi16
 *
 * __epi_32xi16 __builtin_epi_vcompress_32xi16(__epi_32xi16 a, __epi_32xi1 mask, unsigned long int gvl);
 *
 * __epi_32xi16 __builtin_epi_vload_32xi16(const signed short int *address, unsigned long int gvl);
 * 
 * void __builtin_epi_vstore_32xi16(signed short int *address, __epi_32xi16 value, unsigned long int gvl);
 *
 * __epi_32xi1 __builtin_epi_vmslt_32xi16(__epi_32xi16 a, __epi_32xi16 b, unsigned long int gvl);
 *
 * signed long int __builtin_epi_vmpopc_32xi1(__epi_32xi1 a, unsigned long int gvl);
 *
 *  b = 0xFFFFFFFF; __epi_32xi1 __builtin_epi_vmxor_32xi1(__epi_32xi1 a, __epi_32xi1 b, unsigned long int gvl);     // vnot
 *
 *
 *  */
