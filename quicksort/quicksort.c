#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ARRLEN 10
#define VL 3

int array[] = {3,7,17,8,5,2,1,9,5,4};

void quicksort(int*, int, int);
int partition(int*, int, int);
void print_array(int*, int);




int main(){
    quicksort(array, 0, ARRLEN-1);
    print_array(array, ARRLEN);

    printf("Result: ");
    print_array(array, ARRLEN);

    return 0;
}


void quicksort(int *A, int lo, int hi){
    if(lo<hi){
       int p = partition(A, lo, hi);
       quicksort(A, lo, p-1);
       quicksort(A, p+1, hi);
    }
}




int partition(int *A, int lo, int hi){
    int pivot = A[hi];
    int j = hi;
    int i = lo;

    while(i!=j){
        if(A[i]>pivot){
            // do the change
            int tmp = A[i];
            A[i] = A[j-1];
            A[j-1] = pivot;
            A[j] = tmp;
            j--;
        }
        else
            i++;
    }

    return j;
    
}

void print_array(int *A, int arrlen){
	    for(int i=0; i<arrlen; i++){
		            printf("%d ", A[i]);
			        }
	        printf("\n\n");
}
