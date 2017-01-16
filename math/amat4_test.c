#include <stdio.h>
#include <glla.h>

int main()
{
	float mat1[] = {
		 1,  2,  3,  4,
		 5,  6,  7,  8,
		 9, 10, 11, 12,
		13, 14, 15, 16
	};
	float mat2[] = {
		 1,  2,  3,  4,
		 5,  6,  7,  8,
		 9, 10, 11, 12,
		13, 14, 15, 16
	};
	float result[16];
	amat4_buf_mult(mat1, mat2, result);
	printf("Result:\n");
	for (int i = 0; i < 4; i++)
		printf("%2f %2f %2f %2f\n", result[4*i], result[4*i + 1], result[4*i + 2], result[4*i + 3]);
	return 0;
}