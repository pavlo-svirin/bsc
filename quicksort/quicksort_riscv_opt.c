#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <math.h>

#define ARRLEN 20
#define VL 8

int32_t array[] = {5,13,55,35,84,58,42,76,78,2,86,3,33,64,63,76,61,93,64,21};
//int32_t array[ARRLEN]; 

void quicksort_vectorized(int*, int, int);
int partition_vectorized_opt(int*, int, int);
void print_array(int32_t*, int);
void fprint_array(FILE*, int32_t*, int);
void print_mask(__epi_8xi1 mask, int arrlen);
void print_epi_8xi32(__epi_8xi32 epi, int gvl);


__epi_8xi1 vnot(__epi_8xi1 mask, unsigned int gvl);

int store_arrays(int *address, unsigned int lo, unsigned int hi, __epi_8xi32 vec1, __epi_8xi32 vec2, 
                       __epi_8xi1 mask1, __epi_8xi1 mask2, 
                       int gvl1, int gvl2, int pivot);

void vswap(int32_t *address,
		__epi_8xi32 a1, __epi_8xi32 a2,
		unsigned int lo, unsigned int j, 
		unsigned  int num_to_swap1,
		unsigned int vector_length);

//__epi_8xi32 __builtin_epi_vcompress_8xi32__(__epi_8xi32 a, __epi_8xi1 mask, unsigned long int gvl);



void generate_array(int32_t *addr, unsigned int arrlen){
    srand(time(NULL));
    for(int i=0; i<arrlen; i++)
        *(addr+i) = (double)rand()/(double)RAND_MAX*100+1;
}



int main(){
    //quicksort(array, 0, ARRLEN-1);
    //generate_array(array, ARRLEN);
    FILE *f = fopen("initial_array", "w");
    print_array(array, ARRLEN);
    fprint_array(f, array, ARRLEN);


//    return 0;

    //quicksort_vectorized(array, 0, ARRLEN-1);

    partition_vectorized_opt(array, 0, ARRLEN-1);

    printf("Result: ");
    print_array(array, ARRLEN);
    fprint_array(f, array, ARRLEN);
    fclose(f);

    return 0;
}


void quicksort_vectorized_opt(int *A, int lo, int hi){
    if(lo<hi){
       int p = partition_vectorized_opt(A, lo, hi);
       printf("Pivot position returned: %d", p);
       quicksort_vectorized_opt(A, lo, p-1);
       quicksort_vectorized_opt(A, p+1, hi);
    }
}


int partition_vectorized_opt(int *A, int lo, int hi){
	int i = lo;
	int j = hi-1;
	int32_t pivot = A[hi];
	int new_pivot_position = j;

	while(i<=j){
		int vector_length = VL;
		if(i==j){
			break;
		}
		else if(j-i<2*VL){
			vector_length = (j-i)/2;
		}
		else if(j-i==1){
			print_array(A+i, 2);
			break;
		}

		// set pivot vector
		__epi_8xi32 vec_pivot = __builtin_epi_vbroadcast_8xi32(pivot, vector_length);

		// read "vector_length" from the end
		__epi_8xi32 vec_end = __builtin_epi_vload_strided_8xi32(A+j, -sizeof(int32_t), vector_length);
		// read "vector_length" from beginning
		__epi_8xi32 vec_beg = __builtin_epi_vload_strided_8xi32(A+i, sizeof(int32_t), vector_length);

		// Checking the max and min values for vectors, this can be useful to make quick jumps
		// All of these operations can be done in vector form, to be optimized later
		// <TO_BE_OPTIMIZED>
		// check maximum
		__epi_8xi32 vec_beg_max_epi = __builtin_epi_vredmax_8xi32(vec_beg, vec_pivot, vector_length);
		__epi_8xi32 vec_end_max_epi = __builtin_epi_vredmax_8xi32(vec_end, vec_pivot, vector_length);

		// check minimum
		__epi_8xi32 vec_end_min_epi = __builtin_epi_vredmin_8xi32(vec_end, vec_pivot, vector_length);
		__epi_8xi32 vec_beg_min_epi = __builtin_epi_vredmin_8xi32(vec_beg, vec_pivot, vector_length);

		int32_t vec_beg_max = __builtin_epi_vextract_8xi32(vec_beg_max_epi, 0);
		int32_t vec_end_max = __builtin_epi_vextract_8xi32(vec_end_max_epi, 0);
		int32_t vec_beg_min = __builtin_epi_vextract_8xi32(vec_beg_min_epi, 0);
		int32_t vec_end_min = __builtin_epi_vextract_8xi32(vec_end_min_epi, 0);

		// all of the ending vector values are greater or equal than pivot
		// no need to perform swaps, just save them and go to the next iteration
		if(vec_end_min>pivot){
			// save it to hi, stride -1
        	__builtin_epi_vstore_strided_8xi32(A+hi, vec_end, -sizeof(int32_t), vector_length);
			// put pivot
			j -= vector_length;
			A[j+1] = pivot;
			if(vec_beg_max<pivot){
				i += vector_length;
			}
			continue;
		}

		// we also can keep read vectors for future iterations, can be considered later

		// </TO_BE_OPTIMIZED>

		print_epi_8xi32(vec_beg, vector_length);
		print_epi_8xi32(vec_end, vector_length);

		// load pivot against pivot
		
		// compare and handle results
		__epi_8xi1 mask_lt = __builtin_epi_vmslt_8xi32(vec_end, vec_pivot, vector_length);
    	__epi_8xi32 put_to_beg = __builtin_epi_vcompress_8xi32(vec_end, mask_lt, vector_length);
		// number of lesser than values in the set
    	unsigned int lt_count = __builtin_epi_vmpopc_8xi1(mask_lt, vector_length); 
    	unsigned int gt_count = vector_length - lt_count;
    	__epi_8xi1 mask_gt = vnot(mask_lt,  vector_length);
    	// compressed greaters 
    	__epi_8xi32 put_to_end = __builtin_epi_vcompress_8xi32(vec_end, mask_gt, vector_length);

		__epi_8xi32 indexes = __builtin_epi_vid_8xi32(vector_length);

		i += vector_length;
		j -= vector_length;
	}

	return new_pivot_position;
}



		//printf("Vector length: %d\n", vector_length);
		//printf("Beginning vector: ");
		//printf("Ending vector: ");
		//printf("i: %d | j: %d\n", i, j);
		//printf("Last element: %d\n", A[i]);


int partition_vectorized(int *A, int lo, int hi){
    int32_t pivot = A[hi];
    int j = hi-1;
    int i = 0;
    int k = 0;
    int pivot_position = hi;

    printf("=======================================================================================\n");
    while(1){
	printf("=============================================================\n");
        printf("\n=== BEGIN \n==== lo: %d | pivot position: %d | pivot: %d ====\n", lo, pivot_position, A[pivot_position]);
      
        // define vector lengths
        int vector_length = VL;
        
        if(lo>=hi){
            break;
        }

        if(hi==lo+1){
            if(A[hi]<A[lo]){
		printf("::: we do the swap :::\n");
                int tmp = A[hi];
                A[hi] = A[lo];
                A[lo] = tmp;
				hi--;
				pivot_position--;
            }
	    lo++;
	    printf("Loop result: ");
            print_array(A, ARRLEN);
            break;
        }

        if(pivot_position==lo+1){
            if(A[pivot_position]<A[lo]){
		printf("::: we do the swap with pivot :::\n");
                int tmp = A[pivot_position];
                A[pivot_position] = A[lo];
                A[lo] = tmp;
				pivot_position--;
            }
	    lo++;
	    printf("Loop result: ");
            print_array(A, ARRLEN);
            break;
        }
	//else if(hi-lo<VL)
	//   vector_length = hi-lo;

        if(hi-lo<2*VL)
	   vector_length = (hi-lo)/2;
	printf("Vector length selected: %d\n", vector_length);
        //else if(hi-lo-1<VL)
        //   vector_length = hi-lo-1;

	__epi_8xi32 pivot_vec = __builtin_epi_vbroadcast_8xi32(pivot, vector_length);
	__epi_8xi32 vec1 = __builtin_epi_vload_strided_8xi32(A+hi-1, -sizeof(int32_t), vector_length);
	__epi_8xi1 mask1 = __builtin_epi_vmslt_8xi32(vec1, pivot_vec, vector_length);

    __epi_8xi32 v1 = __builtin_epi_vcompress_8xi32(vec1, mask1, vector_length);
    unsigned int pc1 = __builtin_epi_vmpopc_8xi1(mask1, vector_length); // number of lesser than values in the set
    unsigned int pc_gt = vector_length-pc1;

    printf("Elements : lt %d  |  gt %d\n", pc1, pc_gt);

    // mask for greaters
    __epi_8xi1 nmask1 = vnot(mask1,  vector_length);
    // compressed greaters 
    v1 = __builtin_epi_vcompress_8xi32(vec1, nmask1, vector_length);

    printf("compressed greater than : ");
    print_epi_8xi32(v1, vector_length);

    // store greater for vec1
    if(pc_gt>0){
        __builtin_epi_vstore_strided_8xi32(A+hi, v1, -sizeof(int32_t), pc_gt);
        pivot_position = hi-(pc_gt);
        *(A+pivot_position) = pivot;
    }

    if(pc1>0 ){
    // read vectors from beginning, they do not have to intersect
    int num_to_load_from_start = pivot_position-lo-1>2*pc1 ? pc1 : pivot_position - lo - pc1;

    //__epi_8xi32 start1 = __builtin_epi_vload_8xi32(address+lo, gvl1);
    __epi_8xi32 start1 = __builtin_epi_vload_8xi32(A+lo, num_to_load_from_start);
    printf("loaded from start %d : ", num_to_load_from_start );
    print_epi_8xi32(start1, vector_length);

    v1 = __builtin_epi_vcompress_8xi32(vec1, mask1, vector_length);
    printf("compressed lesser than: ");
    print_epi_8xi32(v1,vector_length);

    // and here we store the data
    vswap(A,
		v1, start1,
		lo, pivot_position-1, 
		pc1,
		vector_length);
    }
	
        lo += pc1;
        j += pc1;
        hi = hi - vector_length + pc1;

	printf("Loop result: ");
        print_array(A, ARRLEN);

        i++;
        if(i>50) break;
    }

    return pivot_position;
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
    signed char *m = (signed char*)malloc(8*sizeof(char));
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





int store_arrays(int *address, 
                    unsigned int lo,
                    unsigned int hi,
                    __epi_8xi32 vec1, 
                    __epi_8xi32 vec2, 
                    __epi_8xi1 mask1, 
                    __epi_8xi1 mask2, 
                    int gvl1,  // vector length used
                    int gvl2, 
                    int pivot){

    // store lessers
    //vec1:
    __epi_8xi32 v1 = __builtin_epi_vcompress_8xi32(vec1, mask1, gvl1);
    unsigned int pc1 = __builtin_epi_vmpopc_8xi1(mask1, gvl1); // number of lesser than values in the set
    unsigned int pc_gt = gvl1-pc1;

    
    //vec2:
    __epi_8xi32 v2 = __builtin_epi_vcompress_8xi32(vec2, mask2, gvl2);
    unsigned int pc2 = __builtin_epi_vmpopc_8xi1(mask2, gvl2);

    printf("Elements : lt %d  |  gt %d\n", pc1, pc_gt);

    // mask for greaters
    __epi_8xi1 nmask1 = vnot(mask1,  gvl1);
    // compressed greaters 
    v1 = __builtin_epi_vcompress_8xi32(vec1, nmask1, gvl1);

    printf("compressed greater than : ");
    print_epi_8xi32(v1, gvl1);
    //printf("To store: %d\n", gvl1-pc1);
    //printf("We store at index: %d\n", hi);

    // store greater for vec1
    __builtin_epi_vstore_strided_8xi32(address+hi, v1, -sizeof(int32_t), pc_gt);

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
    //int32_t pivot_position = hi-(gvl1-pc1);
    int32_t pivot_position = hi-(pc_gt);
    *(address+pivot_position) = pivot;


    //printf("====== array: ");
    //print_array(array, ARRLEN);
    //printf("\n");

   
   // if short vector do not load anything 
   /*if(hi==lo){
	printf("One element left\n");
	int x;
	scanf("%d", x);
    	v1 = __builtin_epi_vcompress_8xi32(vec1, mask1, gvl1);
    	__builtin_epi_vstore_8xi32(address+lo, v1, pc1);

	return pc1;
  	 
   }*/
    
    if(pc1>0){
    // read vectors from beginning, they do not have to intersect
    int num_to_load_from_start = pivot_position-lo-1>2*pc1 ? pc1 : pivot_position - lo - pc1 -1;

    //__epi_8xi32 start1 = __builtin_epi_vload_8xi32(address+lo, gvl1);
    __epi_8xi32 start1 = __builtin_epi_vload_8xi32(address+lo, num_to_load_from_start);
    printf("loaded from start %d : ", num_to_load_from_start );
    print_epi_8xi32(start1, gvl1);

    __epi_8xi32 start2 = __builtin_epi_vload_8xi32(address+lo+gvl1, gvl2);
    //printf("loaded from start 2: ");
    //print_epi_8xi32(start2, gvl2);


    v1 = __builtin_epi_vcompress_8xi32(vec1, mask1, gvl1);
    printf("compressed lesser than: ");
    print_epi_8xi32(v1, gvl1);
    v2 = __builtin_epi_vcompress_8xi32(vec2, mask2, gvl2);
    //printf("compressed2: ");
    //print_epi_8xi32(v2, gvl2);

    // and here we store the data
    vswap(address,
		v1, start1,
		lo, pivot_position-1, 
		pc1,
		gvl1);
    }

    return pc1;
}



void vswap(int32_t *address,
		__epi_8xi32 a1, __epi_8xi32 start1,
		unsigned int lo, unsigned int hi, 
		unsigned  int num_to_swap1,
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

    /*if(num_to_swap2>0){
	    //__epi_8xi32 a = __builtin_epi_vload_8xi32(a1 + vector_length,  to_swap);
	    //__epi_8xi32 a0 = __builtin_epi_vload_8xi32(a2 + vector_length, to_swap);


	__builtin_epi_vstore_8xi32(address+lo+num_to_swap1, b1, num_to_swap2);
	__builtin_epi_vstore_strided_8xi32(address+hi-num_to_swap1, start2, -sizeof(int32_t), num_to_swap2);

    } */
}


/*
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
*/


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
