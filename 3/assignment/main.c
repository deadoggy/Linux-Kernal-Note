

#define MAT_SIZE_N  1024
#define MAT_SIZE_NP1 1025



#include<stdio.h>
#include<time.h>
#include<stdlib.h>

int mat_n[MAT_SIZE_N][MAT_SIZE_N];
int mat_np1[MAT_SIZE_NP1][MAT_SIZE_NP1];

int main(int argc, char** argv){
    
    int b;
    time_t start, end;

    for(int i=0; i<MAT_SIZE_NP1; i++){
        for(int j=0; j<MAT_SIZE_NP1; j++){    
            if(i<MAT_SIZE_N&&j<MAT_SIZE_N){
                mat_n[i][j] = rand();
            }
            mat_np1[i][j] = rand();
        }

    }

    printf("Matrix MAT_SIZE_N...\n");
    start = clock();
    for(int i=0; i<MAT_SIZE_N; i++){
        for(int j=i+1; j<MAT_SIZE_N; j++){
            b = mat_n[i][j];
            mat_n[i][j] = mat_n[j][i];
            mat_n[j][i] = b;
        }
    }
    end = clock();
    printf("Matrix MAT_SIZE_N, time = %d\n", end-start);



    printf("\nMatrix MAT_SIZE_NP1...\n");
    start = clock();
    for(int i=0; i<MAT_SIZE_NP1; i++){
        for(int j=i+1; j<MAT_SIZE_NP1; j++){
            b = mat_np1[i][j];
            mat_np1[i][j] = mat_np1[j][i];
            mat_np1[j][i] = b;
        }
    }
    end = clock();
    printf("Matrix MAT_SIZE_NP1, time = %d\n", end-start);

    return 0;
}