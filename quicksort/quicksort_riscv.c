#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define ARRLEN 20
#define VL 4

//int32_t array[] = {3,7,17,8,5,2,1,9,5,4};
int32_t array[ARRLEN]; 

void quicksort_vectorized(int*, int, int);
int partition_vectorized(int*, int, int);
void print_array(int32_t*, int);
void print_mask(__epi_8xi1 mask, int arrlen);
void print_epi_8xi32(__epi_8xi32 epi, int gvl);


__epi_8xi1 vnot(__epi_8xi1 mask, unsigned int gvl);

int store_arrays(int *address, unsigned int lo, unsigned int hi, __epi_8xi32 vec1, __epi_8xi32 vec2, 
                       __epi_8xi1 mask1, __epi_8xi1 mask2, 
                       int gvl1, int gvl2, int pivot);

void vswap(int32_t *address,
		__epi_8xi32 a1, __epi_8xi32 a2,
	       	__epi_8xi32 b1,  __epi_8xi32 b2,
		unsigned int lo, unsigned int j, 
		unsigned  int num_to_swap1,
		unsigned  int num_to_swap2,
		unsigned int vector_length);

__epi_8xi32 __builtin_epi_vcompress_8xi32__(__epi_8xi32 a, __epi_8xi1 mask, unsigned long int gvl);



void generate_array(int32_t *addr, unsigned int arrlen){
    for(int i=0; i<arrlen; i++)
        *(addr+i) = (double)rand()/(double)RAND_MAX*100;
}



int main(){
    //quicksort(array, 0, ARRLEN-1);
    generate_array(array, ARRLEN);
    print_array(array, ARRLEN);

//    return 0;

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
    int j = hi-1;
    int i = 0;
    int k = 0;

    //while(j>lo){
    while(1){
	printf("=============================================================\n");
        printf("\n=== BEGIN \n==== j: %d |  lo: %d | hi: %d | pivot: %d ====\n", j, lo, hi, A[hi]);
      
        // define vector lengths
        int vector_length = VL;
        
        if(lo>=hi){
            break;
        }
        if(hi==lo+1){
            if(A[hi]<A[lo]){
		//printf("::: we do the swap :::\n");
                int tmp = A[hi];
                A[hi] = A[lo];
                A[lo] = tmp;
				hi--;
            }
			lo++;
            break;
        }
        //else if(hi-lo<2*VL)
        else if(hi-lo-1<VL)
            vector_length = hi-lo-1;

        //j -= vector_length;
	__epi_8xi32 pivot_vec = __builtin_epi_vbroadcast_8xi32(pivot, vector_length);
 	
	__epi_8xi32 vec1 = __builtin_epi_vload_strided_8xi32(A+hi-1, -sizeof(int32_t), vector_length);
	//print_epi_8xi32(vec1, vector_length);

	__epi_8xi1 result1 = __builtin_epi_vmslt_8xi32(vec1, pivot_vec, vector_length);

        // read vec2
        //j -= vector_length;

	__epi_8xi32 vec2 = __builtin_epi_vload_strided_8xi32(A+hi-vector_length-1, -sizeof(int32_t), vector_length);

	__epi_8xi1 result2 = __builtin_epi_vmslt_8xi32(vec2, pivot_vec, vector_length);
	//print_epi_8xi32(vec2, vector_length);

        // now we store the data
        //k = store_arrays(A, lo, hi, vec1, vec2, result1, result2, vector_length, vector_length, pivot);
        k = store_arrays(A, lo, hi, vec1, vec2, result1, result2, vector_length, vector_length, pivot);
        lo += k;
        j += k;
        hi = hi - vector_length + k;

	printf("Loop result: ");
        print_array(A, ARRLEN);

        //if(j==lo)
        //    break;
        //else
        //j = hi-1-2*vector_length+k;
        //printf("===  vector length: %d |  lo: %d |  pivot pos: %d\n", vector_length, lo, j);
            //j--;

        // for debugging
        //break;
        i++;
        if(i>10) break;
    }

	//printf("_______________ we return: %d _____________________", j);

    return hi;
}


void print_array(int32_t *A, int arrlen){
    for(int i=0; i<arrlen; i++){
        printf("%d ", A[i]);
    }
    printf("\n\n");
}


void print_epi_8xi32(__epi_8xi32 epi, int gvl){
    int32_t *m = (int32_t*)malloc(8*sizeof(int32_t));
    __builtin_epi_vstore_8xi32(m, epi, gvl);
    
    for(int i=0; i<gvl; i++){
        printf("%d ", m[i]);
    }
    printf("\n\n");
    free(m);
}


void print_mask(__epi_8xi1 mask, int gvl){
    __epi_8xi8 mask_array = __builtin_epi_cast_8xi8_8xi1(mask);	
    char *m = (char*)malloc(8*sizeof(char));
    __builtin_epi_vstore_8xi8(m, mask_array, gvl);
    
    for(int i=0; i<gvl; i++){
        printf("%d", m[i]);
    }
    printf("\n\n");
    free(m);
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

    // store lessers
    //vec1:
    __epi_8xi32 v1 = __builtin_epi_vcompress_8xi32__(vec1, mask1, gvl1);

    //vcompress(v1, vec1, mask1, gvl1);
   // int pc1 = vpopc(mask1, gvl1);
    unsigned int pc1 = __builtin_epi_vmpopc_8xi1(mask1, gvl1);
    // store greaters
    //vstore_strided(address, v1, 1, pc1);

    //vec2:
    __epi_8xi32 v2 = __builtin_epi_vcompress_8xi32__(vec2, mask2, gvl2);

    unsigned int pc2 = __builtin_epi_vmpopc_8xi1(mask2, gvl2);

    // mask for greaters
    __epi_8xi1 nmask1 = vnot(mask1,  gvl1);
     
    v1 = __builtin_epi_vcompress_8xi32__(vec1, nmask1, gvl1);
    printf("compressed3: ");
    print_epi_8xi32(v1, gvl1);
    printf("To store: %d\n", gvl1-pc1);
    printf("We store at index: %d\n", hi);

    // store greater for vec1
    __builtin_epi_vstore_strided_8xi32(address+hi, v1, -sizeof(int32_t), gvl1-pc1);

    //vec2:
    /* __epi_8xi1 nmask2 = vnot(mask2, gvl2);
    v2 = __builtin_epi_vcompress_8xi32__(vec2, nmask2, gvl2);
    printf("compressed4: ");
    print_epi_8xi32(v2, gvl2);
    printf("To store: %d\n", gvl2-pc2);
    
    // store greater for vec2
    __builtin_epi_vstore_strided_8xi32(address+hi-(gvl1-pc1), v2, -sizeof(int32_t), gvl2-pc2);
    */

    // we store pivot
    int32_t pivot_position = hi-(gvl1-pc1);
    *(address+pivot_position) = pivot;


    //printf("====== array: ");
    //print_array(array, ARRLEN);
    //printf("\n");

   
   // if short vector do not load anything 
   if(hi==lo+gvl1){
    	v1 = __builtin_epi_vcompress_8xi32__(vec1, mask1, gvl1);
    	__builtin_epi_vstore_8xi32(lo, v1, pc1);

	return pc1;
  	 
   }
    
    // read vectors from beginning
    __epi_8xi32 start1 = __builtin_epi_vload_8xi32(address+lo, gvl1);
    printf("loaded from start 1: ");
    print_epi_8xi32(start1, gvl1);
    __epi_8xi32 start2 = __builtin_epi_vload_8xi32(address+lo+gvl1, gvl2);
    printf("loaded from start 2: ");
    print_epi_8xi32(start2, gvl2);

    v1 = __builtin_epi_vcompress_8xi32__(vec1, mask1, gvl1);
    printf("compressed1: ");
    print_epi_8xi32(v1, gvl1);
    v2 = __builtin_epi_vcompress_8xi32__(vec2, mask2, gvl2);
    printf("compressed2: ");
    print_epi_8xi32(v2, gvl2);

    // and here we store the data
    vswap(address,
		v1, start1,
	       	v2,  start2,
		lo, pivot_position-1, 
		pc1, pc2,
		gvl1);

    return pc1;
}



void vswap(int32_t *address,
		__epi_8xi32 a1, __epi_8xi32 start1,
	       	__epi_8xi32 b1,  __epi_8xi32 start2,
		unsigned int lo, unsigned int hi, 
		unsigned  int num_to_swap1,
		unsigned  int num_to_swap2,
		unsigned int vector_length) {
    
    //printf("We need to swap at: %d %d\n", lo, hi);
    //a = __builtin_epi_vload_8xi32(a1, to_swap);
    //__epi_8xi32 a0 = __builtin_epi_vload_8xi32(a2, to_swap);

    if(num_to_swap1>0){
	/*a1  = __builtin_epi_vxor_8xi32(a1, b1, num_to_swap1);
	b1 = __builtin_epi_vxor_8xi32(b1, a1,  num_to_swap1);
	a1  = __builtin_epi_vxor_8xi32(a1, b1, num_to_swap1);*/

	__builtin_epi_vstore_8xi32(address+lo, a1,  num_to_swap1);
	__builtin_epi_vstore_strided_8xi32(address+hi, start1, -sizeof(int32_t), num_to_swap1);
    }

    return;

    //printf("We finished the first swap\n");

    if(num_to_swap2>0){
	    //__epi_8xi32 a = __builtin_epi_vload_8xi32(a1 + vector_length,  to_swap);
	    //__epi_8xi32 a0 = __builtin_epi_vload_8xi32(a2 + vector_length, to_swap);

	    /*a2  = __builtin_epi_vxor_8xi32(a2, b2, num_to_swap2);
	    b2 = __builtin_epi_vxor_8xi32(b2, a2,  num_to_swap2);
	    a2  = __builtin_epi_vxor_8xi32(a2, b2, num_to_swap2);*/

	__builtin_epi_vstore_8xi32(address+lo+num_to_swap1, b1, num_to_swap2);
	__builtin_epi_vstore_strided_8xi32(address+hi-num_to_swap1, start2, -sizeof(int32_t), num_to_swap2);

    }
}



__epi_8xi32 __builtin_epi_vcompress_8xi32__(__epi_8xi32 a, __epi_8xi1 mask, unsigned long int gvl){
	__epi_8xi8 mask_array = __builtin_epi_cast_8xi8_8xi1(mask);	
	char *m = (char*)malloc(8*sizeof(char));
	int32_t *tmp = (int32_t*)malloc(8*sizeof(int32_t));
	int32_t *result = (int32_t*)malloc(8*sizeof(int32_t));
 	__builtin_epi_vstore_8xi32(tmp, a, gvl);
 	__builtin_epi_vstore_8xi8(m, mask_array, gvl);

	int j=0;
	int i=0;
	for(; i<gvl; i++){
		if(m[i]){
			result[j] = tmp[i];
			j++;
		}	
	}
	for(; j<8; j++) 
		result[j] = 0;
	a = __builtin_epi_vload_8xi32(result, gvl);
	
	free(m);
	free(tmp);
	free(result);
	return a;
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
