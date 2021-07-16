#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <math.h>

#define ARRLEN 20
#define VL 8

#define min(X, Y)  ((X) < (Y) ? (X) : (Y))
#define max(X, Y)  ((X) > (Y) ? (X) : (Y))

int32_t array[] = {5,13,55,35,84,58,42,76,78,2,86,3,33,64,63,76,61,93,64,21};
//int32_t array[] = {5,13,55,35,84,58,42,76,78,2,86,3,33,64,63}; //,76,61,93,64,21};
//int32_t array[ARRLEN]; 

void quicksort_vectorized_opt(int*, int, int);
int partition_vectorized_opt(int*, int, int);
void print_array(int32_t*, int);
void fprint_array(FILE*, int32_t*, int);
void print_mask(__epi_8xi1 mask, int arrlen);
void print_epi_8xi32(__epi_8xi32 epi, int gvl);


__epi_8xi1 vnot(__epi_8xi1 mask, unsigned int gvl);

int store_arrays(int *address, unsigned int lo, unsigned int hi, __epi_8xi32 vec1, __epi_8xi32 vec2, 
                       __epi_8xi1 mask1, __epi_8xi1 mask2, 
                       int gvl1, int gvl2, int pivot);



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

    quicksort_vectorized_opt(array, 0, ARRLEN-1);

    //partition_vectorized_opt(array, 0, ARRLEN-1);

    printf("Result: ");
    print_array(array, ARRLEN);
    fprint_array(f, array, ARRLEN);
    fclose(f);

    return 0;
}


void quicksort_vectorized_opt(int *A, int lo, int hi){
    if(lo<hi){
       int p = partition_vectorized_opt(A, lo, hi);
       quicksort_vectorized_opt(A, lo, p-1);
       quicksort_vectorized_opt(A, p+1, hi);
    }
}


int partition_vectorized_opt(int *A, int lo, int hi){
	int i = lo;
	int j = hi-1;
	int32_t pivot = A[hi];
	int new_pivot_position = hi;

	int iter = 0;

	while(i<=j){
		int vector_length = VL;
		if(j-i==1){
			//print_array(A+i, 2);
			//A[i] = min(A[i], A[j]);
			//A[j] = max(A[i], A[j]);
			vector_length = 1;
		}
		else if(j==i){
			if(A[j]>pivot){
				A[new_pivot_position] = A[j];
				A[j] = pivot;	
				new_pivot_position--;
			}
			break;
		}
		else if(j-i+1<2*VL){
			vector_length = (j-i)/2;
		}

		printf("i: %d  | j: %d |  vector_length: %d | pivot: %d ========\n", i, j, vector_length, pivot);

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
        	__builtin_epi_vstore_strided_8xi32(A+j, vec_end, -sizeof(int32_t), vector_length);
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

		printf("Beg: ");
		print_epi_8xi32(vec_beg, vector_length);
		printf("End: ");
		print_epi_8xi32(vec_end, vector_length);

		// load pivot against pivot
		
		// compare and handle results
		__epi_8xi1 mask_lt_end = __builtin_epi_vmslt_8xi32(vec_end, vec_pivot, vector_length);
		__epi_8xi1 mask_lt_beg = __builtin_epi_vmslt_8xi32(vec_beg, vec_pivot, vector_length);

		// number of lesser than values in the set
    	unsigned int lt_count_end = __builtin_epi_vmpopc_8xi1(mask_lt_end, vector_length); 
    	unsigned int gt_count_end = vector_length - lt_count_end;
    	unsigned int lt_count_beg = __builtin_epi_vmpopc_8xi1(mask_lt_beg, vector_length); 
    	unsigned int gt_count_beg = vector_length - lt_count_beg;

    	// compressed greaters 
    	__epi_8xi1 mask_gt = vnot(mask_lt_end,  vector_length);
    	__epi_8xi32 put_to_end = __builtin_epi_vcompress_8xi32(vec_end, mask_gt, vector_length);
        
		// store greaters at top
		__builtin_epi_vstore_strided_8xi32(A+new_pivot_position, put_to_end, -sizeof(int32_t), gt_count_end);

		// store pivot before greaters
		new_pivot_position = j-gt_count_end+1;
		A[new_pivot_position] = pivot;
		
		// store lessers
		// compress lessers from beginning
    	__epi_8xi32 put_to_beg = __builtin_epi_vcompress_8xi32(vec_beg, mask_lt_beg, vector_length);
		__builtin_epi_vstore_strided_8xi32(A+i, put_to_beg, sizeof(int32_t), lt_count_beg);
		
		
		
		// now we are doing the swap
		// create the shortest mask filled with ones
		__epi_8xi1 swap_mask;
		int swap_mask_len = lt_count_end>gt_count_beg ? gt_count_beg : lt_count_end;
		swap_mask = vnot(swap_mask, swap_mask_len);
		printf("Swap mask (len %d): ", swap_mask_len);
		print_mask(swap_mask, vector_length);

		// prepare swap vectors
    	__epi_8xi32 swap_vec_end = __builtin_epi_vcompress_8xi32(vec_end, 
													mask_lt_end, 
													vector_length);
    	__epi_8xi32 swap_vec_beg = __builtin_epi_vcompress_8xi32(vec_beg, 
													vnot(mask_lt_beg, vector_length), 
													vector_length);
		put_to_end = __builtin_epi_vmerge_8xi32(swap_vec_end, swap_vec_beg, swap_mask, lt_count_end);
		put_to_beg = __builtin_epi_vmerge_8xi32(swap_vec_beg, swap_vec_end, swap_mask, gt_count_beg);
		
		printf("Compressed array end: ");
		print_epi_8xi32(swap_vec_end, lt_count_end);
		printf("Compressed array beg: ");
		print_epi_8xi32(swap_vec_beg, gt_count_beg);
		printf("This goes to end: ");
		print_epi_8xi32(put_to_end, lt_count_end);
		printf("This goes to beginning: ");
		print_epi_8xi32(put_to_beg, gt_count_beg);

		__builtin_epi_vstore_strided_8xi32(A+i+lt_count_beg, 
											put_to_beg, sizeof(int32_t), gt_count_beg);
		__builtin_epi_vstore_strided_8xi32(A+new_pivot_position-1, 
											put_to_end, -sizeof(int32_t), lt_count_end);


		printf("Intermediate result: ");
		print_array(A, ARRLEN);
		
		//i += vector_length;
		i += lt_count_beg + min(gt_count_beg, lt_count_end);
		//j -= vector_length;
		j -= gt_count_end;
		iter++;
	}

	printf("We are returning from partiution: %d\n", new_pivot_position);

	return new_pivot_position;
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



