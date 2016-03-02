/* 
 * File:   matrix.h
 * Author: jangedoo
 *
 * Created on March 2, 2016, 9:10 PM
 */

#include <mpi.h>
/**
 * Perform Matrix Vector Multiplication using MPI
 */
void matrix_vector_multiplication() {
    int ROW_SIZE, COL_SIZE, DIM_LEN, ROOT, DONE, myRank, numProcs, i, j;

    ROW_SIZE = 10;
    COL_SIZE = 5;
    ROOT = 0;
    DONE = 999999;  //tag to specify that we are done

    //we have a matrix of size 10*5 and vector of length 5
    int* matrix;
    int* vector;
    int* buffer; //for other processes to receive a row
    int tempResult; //for ROOT process to store the result other processes send
    int* finalProduct;

    vector = malloc(sizeof (int) * COL_SIZE);

    MPI_Status status;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

    if (myRank == ROOT) {

        //initialize matrix
        matrix = malloc(sizeof (int) * DIM_LEN);
        for (i = 0; i < ROW_SIZE; i++) {
            for (j = 0; j < COL_SIZE; j++) {
                matrix[i * COL_SIZE + j] = 1;
            }
        }

        //initialize vector        
        for (i = 0; i < COL_SIZE; i++) {
            vector[i] = 1;
        }

        //allocate memory for finalProduct
        finalProduct = malloc(sizeof (int) * ROW_SIZE);
    }

    //boradcast the vector to all other processes
    MPI_Bcast(vector, COL_SIZE, MPI_INT, ROOT, MPI_COMM_WORLD);

    if (myRank == ROOT) {
        //send the rows to OTHER processes one by one
        //we are assuming the number of processes is less than NUM_ROWS
        int sentRow = 0;
        for (i = 1; i < numProcs; i++) {

            int startIndex = sentRow * COL_SIZE; //start index in the matrix
            int destination = i; //rank of the process to send this to
            int tag = sentRow; //tag could be anything, we are putting the current row number
            MPI_Send(&matrix[startIndex], COL_SIZE, MPI_INT, destination, tag, MPI_COMM_WORLD);
            sentRow++;

        }

        //we are now done sending to every process.
        //Now wait for them to send the result and give them more rows if there are any
        for(i = 0; i < ROW_SIZE; i++) {
            MPI_Recv(&tempResult, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            //get the row number of the result we got and put it in the result
            finalProduct[status.MPI_TAG] = tempResult;

            //send this process another row
            MPI_Send(&matrix[sentRow * COL_SIZE], COL_SIZE, MPI_INT, status.MPI_SOURCE, sentRow, MPI_COMM_WORLD);
            
            sentRow++;
            
            if(sentRow >= ROW_SIZE){
                //at this pont we don't have any rows to send. so let others know about it
                MPI_Send(MPI_BOTTOM, 0, MPI_DOUBLE, status.MPI_SOURCE, DONE, MPI_COMM_WORLD);
            }
        }
        
        
        //finally print the result
        for(i=0; i < ROW_SIZE; i++){
            printf("result[%d] = %d\n", i, finalProduct[i]);
        }
    } else {
        buffer = malloc(sizeof (int) * COL_SIZE);
        int done = 0;
        while (!done) {
            MPI_Recv(buffer, COL_SIZE, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            done = status.MPI_TAG == DONE;
            if(done) break;
            
            int result = 0;
            for (i = 0; i < COL_SIZE; i++) {
                result += buffer[i] * vector[i];
            }

            //send the result back to root process
            //set the tag the same tag that the ROOT set it to when sending (which is the row number)
            MPI_Send(&result, 1, MPI_INT, ROOT, status.MPI_TAG, MPI_COMM_WORLD);
        }
    }


    MPI_Finalize();
}
