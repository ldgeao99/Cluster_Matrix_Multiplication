#include "stdafx.h"
#include "mpi.h"
#include <time.h>
#include <math.h>
#include <stdlib.h>

// char buffer size
#define MAX_DATA 100

// 배열의 크기 : LENGTH by LENGTH
#define LENGTH 400

// 행렬의 크기가 커질 수록 행렬 곱의 결과 값이 커진다. 
// 값이 정해진 자료형 범위 밖으로 나가는것을 막기 위해 long long int 를 설정하였다.  
typedef unsigned long long LLT;

// 2차원 배열을 위해 메모리 공간을 동적 할당 해주는 함수
LLT **alloc_2d_llt(int rows, int cols);
int **alloc_2d_int(int rows, int cols);

// 2차원 배열을 위한 int 형 동적 할당
int **alloc_2d_int(int rows, int cols) {
	int *data = (int *)malloc(rows*cols*sizeof(int));
	int **array = (int **)malloc(rows*sizeof(int*));
	for (int i = 0; i < rows; i++)
		array[i] = &(data[cols*i]);

	return array;
}

// 2차원 배열을 위한 long long int 형 동적 할당
LLT **alloc_2d_llt(int rows, int cols){
	LLT *data = (LLT *)malloc(rows*cols*sizeof(LLT));
	LLT **array = (LLT **)malloc(rows*sizeof(LLT*));
	for (int i = 0; i < rows; i++)
		array[i] = &(data[cols*i]);

	return array;
}


int main(int argc, char **argv){
	
	int rank;			// 현재 작동하고 있는 프로세서의 인덱스 값 (총 4개의 프로세서라면, 0 ~ 3)
	int size;			// 행렬 곱에 참여하는 전체 프로세서의 개수
	int namelen;		// 프로세서의 이름의 길이
	char processor_name[MPI_MAX_PROCESSOR_NAME];		// 프로세서의 이름
	char buff[MAX_DATA];			// 프로세서가 보내는 메시지를 담기 위한 버퍼

	int **a, **b;		// 2차원 행렬 a, b
	LLT **c;			// 행렬 곱 a * b 의 결과 c (LENGTH by LENGTH)


	// 2차원 행렬 a에 메모리를 동적 할당한다.
	a = alloc_2d_int(LENGTH, LENGTH);

	// 2차원 행렬 b에 메모리를 동적 할당한다.
	b = alloc_2d_int(LENGTH, LENGTH);

	// 2차원 행렬 c에 메모리를 동적 할당한다.
	c = alloc_2d_llt(LENGTH, LENGTH);

	// 클러스터링 MPI 작업을 통한 결과를 담는 변수
	MPI_Status status;

	// MPICH2 를 이용하기 위한 초기화 작업
	MPI_Init(&argc, &argv);

	// MPI_Comm_size 함수는 mpiexec 명령어를 이용해 몇 개의 프로세스가 생성 되었는지를 알려준다.
	// mpiexec n -4 CEU.exe 로 실행한 경우에는 size 의 값이 4 가된다.
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	// MPI_Comm_rank 함수는 mpiexec 프로그램을 이용해 생성된 다수의 프로세스를 식별할 수 있는 고유번호를 할당해 준다.
	//. 즉, mpiexec n -4 CEU.exe 명령어를 이용하여 4 개의 프로세스를 생성한다면 0~3 의 rank 가진 프로세스가 생성되는 것이다.
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// 현재 프로세서의 이름을 가져와서 namelen에 넣는다.
	MPI_Get_processor_name(processor_name, &namelen);		

	// 각 프로세서마다 행렬 곱 a * b 의 계산량 LEGNTH/size 만큼 해야 한다.
	// 즉, a의 LEGNTH/size * LENGTH 크기과 b의 LENGTH * LENGTH 를 곱하여 LEGNTH/size * LENGTH 사이즈의 d를 생성한다. 
	// 이때 정확히 LEGNTH 가 size로 나눠지지 않을 경우 1만큼 추가하여 행렬 곱의 안전성을 더한다.
	int dv;

	if (LENGTH % size == 0){
		dv = LENGTH / size;
	}
	else {
		dv = (int)(floor( (double)(LENGTH / size)) + 1);
	}

	LLT **d;

	d = alloc_2d_llt(dv, LENGTH);

	/* 여기까지 모든 프로그램이 동일하게 수행된다. rank 는 각 프로세스를 구별하기 위해 사용된다.
	주로 rank 0 은 특별한 용도로 사용된다.*/
	/* 이 부분부터 프로세스의 rank 에 따라 수행할 코드가 달라진다. */
	int i;

	// 결과값 c 의 초기화
	for (i = 0; i < LENGTH; i++){
		for (int j = 0; j < LENGTH; j++){
			c[i][j] = 0;
		}
	}

	if (rank == 0){
		printf("### Process %d from %s ###\n", rank, processor_name); fflush(stdout);
		// 다른 프로세서로 부터 메시지를 받아서 출력한다. 연결이 됬음을 확인하는 작업이다.
		for (i = 1; i<size; i++){
			MPI_Recv(buff, MAX_DATA, MPI_CHAR, i, 0, MPI_COMM_WORLD, &status);
			printf("%s\n", buff); fflush(stdout);
		}

		clock_t startTime = clock();			/*행렬 곱 수행 전 시각을 구한다.*/

		// matrix 각 a 의 값은 다음과 같이 채워진다. a[0] = 1, a[1] = 2 , ... a[n] = n + 1
		for (int i = 0; i < LENGTH; i++) {
			for (int j = 0; j < LENGTH; j++) {
				*(*(a + i) + j) = (j + 1) + (i * LENGTH);
			}
		}

		// matrix 각 b 의 값은 다음과 같이 채워진다. b[0] = 1, b[1] = 2 , ... b[n] = n + 1
		for (int i = 0; i < LENGTH; i++) {
			for (int j = 0; j < LENGTH; j++) {
				*(*(b + i) + j) = (j + 1) + (i * LENGTH);
			}
		}

		// 초기화가 완료된 행렬 a 와 b 는 각각 나머지 rank 를 가진 프로세서로 보내진다. 
		// 나머지 프로세서는전체 행렬 곱 계산량 중에서 LENGTH/size 또는 LENGTH/size + 1 만큼을 수행한다.
		for (i = 1; i < size; i++){
			MPI_Send(&(a[0][0]), LENGTH * LENGTH, MPI_INT, i, 0, MPI_COMM_WORLD);
			MPI_Send(&(b[0][0]), LENGTH * LENGTH, MPI_INT, i, 0, MPI_COMM_WORLD);

		}

		// 행렬 d 를 초기화 한다.
		for (int i = 0; i < dv; i++) {
			for (int j = 0; j < LENGTH; j++) {
				*(*(d + i) + j) = 0;
			}
		}

		// a 와 b의 행렬 곱을 dv 만큼 만 수행한다.
		// 예를 들어 총 행렬 곱에 참여한 프로세서가 4개면 1/4 만큼만 계산을 완료하고, 계산된 결과값을 행렬 d 에 넣는다.
		LLT sum = 0;

		for (int i = rank * dv, w = 0; i < (rank * dv) + dv; i++, w++) {
			for (int j = 0; j < LENGTH; j++) {
				for (int k = 0; k < LENGTH; k++) {
					sum += (a[i][k] * b[k][j]);
				}
				d[w][j] = sum;
				sum = 0;
			}
		}

	
		for (i = 0; i < size; i++){
			// LENGTH 가 정확히 size로 나눠지지 않을 경우 마지막 최종 프로세서가 보낸 행렬 d를 행렬 c 에 넣을때
			// index가 초과될 수 있다. 
			/*(Ex. 행렬 크기 30 * 30, 프로세서 4개가 참여
			 Ex. 30/4 = 7 -> 행렬 d 의 크기는  8 * 30, 하지만 행렬 c 의 크기는  30 * 30
			 즉, 총 4개의 행렬 d 를 c 에 넣게되면 32 * 30 이 된다. (because 8 * 4 = 32)
			*/
			// 이를 방지하기위해 boundary 변수를 설정하여 마지막 행렬 d는 최대 LENGTH * LENGTH 만큼 크기를 가지도록 설정한다.
			int boundary = (i * dv) + dv;

			if (boundary > LENGTH){
				boundary = LENGTH;
			}

			// 다른 프로세서로 부터 계산된 행렬 d 를 받아온다. 
			if (i >= 1)
				MPI_Recv(&(d[0][0]), dv *LENGTH, MPI_LONG_LONG_INT, i, 0, MPI_COMM_WORLD, &status);

			// 받아온 행렬 d를 행렬 c 에 총 size 만큼 쌓는다.
			for (int j = i * dv, w = 0; j < boundary; j++, w++) {
				for (int k = 0; k < LENGTH; k++){
					c[j][k] = d[w][k];
				}
			}
		}

		clock_t endTime = clock();				/*행렬 곱 수행 후 구한다.*/
		double nProcessExcuteTime = ((double)(endTime - startTime)) / CLOCKS_PER_SEC;		// 두 시간의 차이가 곧 수행 시간
		printf("------------ ------------ ------------\n");

		//Print result
		printf("Multiplication using %d processor is done. \nExcute time:%f sec\n", size, nProcessExcuteTime);
	}
	else{
		sprintf(buff, "### Process %d from %s ###", rank, processor_name); fflush(stdout);
		// 메인이 아닌 프로세서라면 여기서 자신이 연결되었음을 메인(rank = 0) 프로세서에게 알린다. 
		MPI_Send(buff, MAX_DATA, MPI_CHAR, 0, 0, MPI_COMM_WORLD);

		// 메인 프로세서가 받아온 행렬 a와 b를 받는다.
		MPI_Recv(&(a[0][0]), LENGTH * LENGTH, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
		MPI_Recv(&(b[0][0]), LENGTH * LENGTH, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

		// 행렬 d를 초기화 한다.
		for (int i = 0; i < dv; i++) {
			for (int j = 0; j < LENGTH; j++) {
				*(*(d + i) + j) = 0;
			}
		}

		LLT sum = 0;

		// LENGTH 가 정확히 size로 나눠지지 않을 경우 마지막 최종 프로세서가 보낸 행렬 d를 행렬 c 에 넣을때
		// index가 초과될 수 있다. 
		/*(Ex. 행렬 크기 30 * 30, 프로세서 4개가 참여
		Ex. 30/4 = 7 -> 행렬 d 의 크기는  8 * 30, 하지만 행렬 c 의 크기는  30 * 30
		즉, 총 4개의 행렬 d 를 c 에 넣게되면 32 * 30 이 된다. (because 8 * 4 = 32)
		*/
		// 이를 방지하기위해 boundary 변수를 설정하여 마지막 행렬 d는 최대 LENGTH * LENGTH 만큼 크기를 가지도록 설정한다.
		int boundary = (rank * dv) + dv;

		if (boundary > LENGTH){
			boundary = LENGTH;
		}

		// a 와 b의 행렬 곱을 dv 만큼 만 수행한다.
		// 예를 들어 총 행렬 곱에 참여한 프로세서가 4개면 1/4 만큼만 계산을 완료하고, 계산된 결과값을 행렬 d 에 넣는다.
		for (int i = rank * dv, w = 0; i < boundary; i++, w++) {
			for (int j = 0; j < LENGTH; j++) {
				for (int k = 0; k < LENGTH; k++) {
					sum += (a[i][k] * b[k][j]);
				}
				d[w][j] = sum;
				sum = 0;
			}
		}

		// 각 프로세서가 곱한 결과 행렬 d 를 메인 프로세서에게 넘겨준다.
		MPI_Send(&(d[0][0]), dv * LENGTH, MPI_LONG_LONG_INT, 0, 0, MPI_COMM_WORLD);
	}
	
	// MPI_Finalizer 함수는 MPI 를 이용한 프로그램이 종료되기 전에 항상 호출해야 한다
	MPI_Finalize();

	// 동적 할당된 메모리를 모두 회수하고 프로그램을 종료한다.
	free(a[0]);
	free(a);
	free(b[0]);
	free(b);
	free(c[0]);
	free(c);
	free(d[0]);
	free(d);

	return 0;
}