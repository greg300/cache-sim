#include <stdio.h>

#define R1 5
#define R2 5
#define C1 5
#define C2 5

// Multiplies two square matrices together.
void matrixMult(int mat1[][C1], int mat2[][C2], int result[R1][C2])
{
    for (int i = 0; i < R1; i++) {
        for (int j = 0; j < C2; j++) {
            result[i][j] = 0;
 
            for (int k = 0; k < R2; k++) {
                result[i][j] += mat1[i][k] * mat2[k][j];
            }
        }
    }
}

int main()
{
    int mat1[R1][C1] = {
        {1, 1, 1, 1, 1},
        {2, 2, 2, 2, 2},
        {3, 3, 3, 3, 3},
        {4, 4, 4, 4, 4},
        {5, 5, 5, 5, 5}
    };
    int mat2[R2][C2] = {
        {1, 1, 1, 1, 1},
        {2, 2, 2, 2, 2},
        {3, 3, 3, 3, 3},
        {4, 4, 4, 4, 4},
        {5, 5, 5, 5, 5}
    };

    int result[R1][C2];

    matrixMult(mat1, mat2, result);
}