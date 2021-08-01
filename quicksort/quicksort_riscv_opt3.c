#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <math.h>

#define ARRLEN 10
#define VL 8

#define min(X, Y)  ((X) < (Y) ? (X) : (Y))
#define max(X, Y)  ((X) > (Y) ? (X) : (Y))

struct {
    unsigned int gt;
    unsigned int lt;
} qs_part3_struct;

//int32_t array[] = {5,13,55,35,84,58,42,76,78,2,86,3,33,64,63,76,61,93,64,21};
//int32_t array[] = {5,13,55,35,84,58,42,76,78,2,86,3,33,64,63}; //,76,61,93,64,21};
int32_t array[] = {590, 597, 306, 778, 87, 938, 287, 928, 325, 594}; 
//int32_t array[ARRLEN]; 

void quicksort_vectorized_opt3(int*, int, int);
int partition_vectorized_opt3(int*, int, int);
void print_array(int32_t*, int);
void fprint_array(FILE*, int32_t*, int);
void print_mask(__epi_8xi1 mask, int arrlen);
void print_epi_8xi32(__epi_8xi32 epi, int gvl);


__epi_8xi1 vnot(__epi_8xi1 mask, unsigned int gvl);


int main(){
    //quicksort(array, 0, ARRLEN-1);
    //generate_array(array, ARRLEN);
	
    //read_array("/home/psvirin/dev/test/a", ARRLEN, array);
    FILE *f = fopen("initial_array", "w");
    print_array(array, ARRLEN);
    fprint_array(f, array, ARRLEN);

    quicksort_vectorized_opt3(array, 0, ARRLEN-1);

    //partition_vectorized_opt(array, 0, ARRLEN-1);

    printf("Result: ");
    print_array(array, ARRLEN);
    fprint_array(f, array, ARRLEN);
    fclose(f);

    return 0;
}


void quicksort_vectorized_opt3(int *A, int lo, int hi){
    if(lo<hi){
       qs_part3_struct part_info  = partition_vectorized_opt(A, lo, hi);
       quicksort_vectorized_opt3(A, lo, part_info.lt-1);
       quicksort_vectorized_opt3(A, part_info.gt+1, hi);
    }
}



int partition_vectorized_opt3(int *A, int lo, int hi){
    int i = lo;
    int lt = lo;
    int gt = hi;
    int pivot = A[lo];
    unsigned long int maxvlen = __builtin_epi_vsetvlmax(__epi_e32, __epi_m1);
    __epi_8xi32 vec_pivot = __builtin_epi_vbroadcast_8xi32(pivot, maxvlen);

    /*
   
     while i <= gt:      # Starting from the first element.
         if A[i] < pivot:
             A[lt], A[i] = A[i], A[lt]
             lt += 1
             i += 1
         elif A[i] > pivot:
             A[i], A[gt] = A[gt], A[i]
             gt -= 1
         else:
             i += 1

     return lt, gt
     */
    
    
    while(i<=gt){
        long gvl = __builtin_epi_vsetvl((gt-i)/2, __epi_e32, __epi_m1);
                
        // read "vector_length" from beginning
        __epi_8xi32 vec_beg = __builtin_epi_vload_strided_8xi32(A+i, sizeof(int32_t), gvl);
            
        __epi_8xi1 mask_lt_beg = __builtin_epi_vmslt_8xi32(vec_beg, vec_pivot, gvl);
        unsigned int lt_count_beg = __builtin_epi_vmpopc_8xi1(mask_lt_beg, gvl);
        __epi_8xi32 put_to_beg_lt = __builtin_epi_vcompress_8xi32(vec_beg, mask_lt_beg, gvl);
        
        __epi_8xi1 mask_eq_beg = __builtin_epi_vmseq_8xi32(vec_beg, vec_pivot, gvl);
        unsigned int eq_count_beg = __builtin_epi_vmpopc_8xi1(mask_eq_beg, gvl);
        // vcompress
        __epi_8xi32 put_to_beg_eq = __builtin_epi_vcompress_8xi32(vec_beg, mask_eq_beg, gvl);
        // vslideup
        put_to_beg_eq = __builtin_epi_vslideup_8xi32(put_to_beg_eq, lt_count_beg, gvl);
        // vmerge
        __epi_8xi1 beg_merge_mask = __builtin_epi_vbroadcast_8xi8(1, lt_count_beg);
        put_to_beg = __builtin_epi_vmerge_8xi32(put_to_beg_eq,
                                                put_to_beg_lt,
                                                beg_merge_mask,
                                                lt_count_beg+eq_count_beg);
        
        // read "vector_length" from the end
        __epi_8xi32 vec_end = __builtin_epi_vload_strided_8xi32(A+gt,
                                                                -sizeof(int32_t),
                                                                gvl-lt_count_beg-eq_count_beg);
        vec_end = __builtin_epi_vslideup_8xi32(vec_end,
                                               lt_count_beg+eq_count_beg,
                                               gvl);
        beg_merge_mask = __builtin_epi_vbroadcast_8xi8(1, lt_count_beg+eq_count_beg);
        put_to_beg = __builtin_epi_vmerge_8xi32(vec_end,
                                                put_to_beg,
                                                beg_merge_mask,
                                                lt_count_beg+eq_count_beg);
        // vstore to beginning
        __builtin_epi_vstore_8xi32(A+lt, put_to_beg, gvl);
        lt += lt_count_beg;
        i += lt_count_beg;
        
        // vstore to end
        __epi_8xi1 mask_gt_beg = __builtin_epi_vmnor_8xi1(lt_count_beg,
                                                eq_count_beg,
                                                gvl);
        vec_end = __builtin_epi_vcompress_8xi32(vec_beg, mask_gt_beg, gvl);
        __builtin_epi_vstore_strided_8xi32(A+gt,
                                           vec_end,
                                           -sizeof(int32_t),
                                           gvl-lt_count_beg-eq_count_beg);
        gt -= gvl-lt_count_beg-eq_count_beg;
    }
    
    qs_part3_struct part3_data;
    part3_data.gt = gt;
    part3_data.lt = lt;
    
    return part3_data;
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
			//break;
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

		long gvl = __builtin_epi_vsetvl((j-i)/2, __epi_e32, __epi_m1);

		vector_length = (gvl==0 || gvl==1) ? 1 : gvl;

		printf("i: %d  | j: %d |  vector_length: %d | pivot: %d | gvl %d ========\n", i, j, vector_length, pivot, gvl);
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
/*		if(vec_end_min>pivot){
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
*/


		// we also can keep read vectors for future iterations, can be considered later

		// </TO_BE_OPTIMIZED>

//		printf("Beg: ");
//		print_epi_8xi32(vec_beg, vector_length);
//		printf("End: ");
//		print_epi_8xi32(vec_end, vector_length);

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
//		printf("Swap mask (len %d): ", swap_mask_len);
//		print_mask(swap_mask, vector_length);

		// prepare swap vectors
    	__epi_8xi32 swap_vec_end = __builtin_epi_vcompress_8xi32(vec_end, 
													mask_lt_end, 
													vector_length);
    	__epi_8xi32 swap_vec_beg = __builtin_epi_vcompress_8xi32(vec_beg, 
													vnot(mask_lt_beg, vector_length), 
													vector_length);
		put_to_end = __builtin_epi_vmerge_8xi32(swap_vec_end, swap_vec_beg, swap_mask, lt_count_end);
		put_to_beg = __builtin_epi_vmerge_8xi32(swap_vec_beg, swap_vec_end, swap_mask, gt_count_beg);
		
//		printf("Compressed array end: ");
//		print_epi_8xi32(swap_vec_end, lt_count_end);
//		printf("Compressed array beg: ");
//		print_epi_8xi32(swap_vec_beg, gt_count_beg);
//		printf("This goes to end: ");
//		print_epi_8xi32(put_to_end, lt_count_end);
//		printf("This goes to beginning: ");
//		print_epi_8xi32(put_to_beg, gt_count_beg);

		__builtin_epi_vstore_strided_8xi32(A+i+lt_count_beg, 
											put_to_beg, sizeof(int32_t), gt_count_beg);
		__builtin_epi_vstore_strided_8xi32(A+new_pivot_position-1, 
											put_to_end, -sizeof(int32_t), lt_count_end);


		//printf("\nIntermediate result: ");
		//print_array(A, ARRLEN);
		//exit(0);
		
		//i += vector_length;
		i += lt_count_beg + min(gt_count_beg, lt_count_end);
		//j -= vector_length;
		j -= gt_count_end;
		iter++;
	}

	printf("We are returning from partition: %d\n", new_pivot_position);

	return new_pivot_position;
}









