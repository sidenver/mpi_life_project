#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mpi.h>

void decompose_domain(int X_limit, int world_rank, int world_size, 
                      int* subX_start, int* subX_size, 
                      int** subX_start_arr, int** subX_size_arr) {
  if (world_size > X_limit) {
    // Don't worry about this special case. Assume the domain size
    // is greater than the world size.
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  //*subX_start = X_limit / world_size * world_rank;
  *subX_size_arr = (int *) malloc((world_size)*sizeof(int));
  *subX_start_arr = (int *) malloc((world_size)*sizeof(int));
  int base_size = X_limit / world_size;
  int left = X_limit % world_size;
  int i;
  for (i = 0; i < world_size; ++i)
  {
      (*subX_size_arr)[i] = base_size;
      if (left > 0)
      {
          (*subX_size_arr)[i] += 1;
          left--;
      }
      if (i == 0)
      {
          (*subX_start_arr)[i] = 0;
      } else {
          (*subX_start_arr)[i] = (*subX_start_arr)[i-1] + (*subX_size_arr)[i-1];
      }
      
  }
  *subX_start = (*subX_start_arr)[world_rank];
  *subX_size = (*subX_size_arr)[world_rank];

  printf("process %d start with %d with size %d\n", world_rank, *subX_start, *subX_size);
}

void initialize_boards(char* filename, int world_rank, int world_size, 
                        int X_limit, int Y_limit,
                        int subX_start, int subX_size,
                        int* subX_start_arr, int* subX_size_arr,
                        bool ***coordinate, bool ***nextCoordinate, bool ***totalCoordinate) {
    MPI_Status status;
    bool *longCoordinate;
    if (world_rank == 0)
    {
        *totalCoordinate = (bool **) malloc((X_limit)*sizeof(bool*));
        (*totalCoordinate)[0] = (bool *) calloc ((X_limit)*(Y_limit+2), sizeof(bool));
        longCoordinate = (*totalCoordinate)[0];
        int i, x;
        for(x = 0; x < X_limit; x++){
            (*totalCoordinate)[x] = (**totalCoordinate + (Y_limit+2) * x);
        }

        FILE *fp;
        fp = fopen (filename, "r");
        int row;
        int col;
        while(fscanf( fp,"%d %d", &row, &col )==2) {
            (*totalCoordinate)[row][col + 1] = true;
        }
        fclose(fp);

        // for (i = 1; i < world_size; ++i)
        // {
        //     if (i!=world_size-1){
        //         MPI_Send(totalCoordinate[i * subX_size], 
        //             subX_size*(Y_limit+2), MPI_C_BOOL, i, 0, MPI_COMM_WORLD);
        //     } else {
        //         MPI_Send(totalCoordinate[i * subX_size], 
        //             (subX_size + X_limit % world_size)*(Y_limit+2), MPI_C_BOOL, i, 0, MPI_COMM_WORLD);
        //     }
        // }
    }


    (*coordinate) = (bool **) malloc((subX_size+2)*sizeof(bool*));
    (*coordinate)[0] = (bool *) calloc ((subX_size+2)*(Y_limit+2), sizeof(bool));
    int x;
    for(x = 0; x < subX_size+2; x++){
        (*coordinate)[x] = (**coordinate + (Y_limit+2) * x);
    }

    int i;
    for (i = 0; i < world_size; ++i) {
        subX_size_arr[i] *= (Y_limit+2);
        subX_start_arr[i] *= (Y_limit+2);
    }
    // MPI_Recv((*coordinate)[1], (subX_size)*(Y_limit+2),
    //    MPI_C_BOOL, 0, 0, MPI_COMM_WORLD,
    //    &status);
    // int incoming_size;
    // MPI_Get_count(&status, MPI_C_BOOL, &incoming_size);
    // if (incoming_size!=(subX_size)*(Y_limit+2))
    // {
    //     printf("incoming size for process %d is %d, where should be %d\n", world_rank, incoming_size, (subX_size)*(Y_limit+2));
    //     MPI_Abort(MPI_COMM_WORLD, 1);
    // }
    printf("scatter start for %d\n", world_rank);
    MPI_Scatterv(longCoordinate, subX_size_arr, subX_start_arr,
        MPI_C_BOOL, (*coordinate)[1], subX_size_arr[world_rank],
        MPI_C_BOOL, 0, MPI_COMM_WORLD);
    printf("scatter end for %d\n", world_rank);
    *nextCoordinate = (bool **) malloc((subX_size+2)*sizeof(bool*));
    (*nextCoordinate)[0] = (bool *) calloc ((subX_size+2)*(Y_limit+2), sizeof(bool));
    for(x = 0; x < subX_size+2; x++){
        (*nextCoordinate)[x] = (**nextCoordinate + (Y_limit+2) * x);
    }
}

void cycleOfLife(int subX_size, int Y_limit, bool **coordinate, bool **nextCoordinate) {
    int x, y;
    for(x=1;x<=subX_size;++x){
        for(y=1;y<=Y_limit;++y){
            int count = coordinate[x-1][y-1]+
                        coordinate[x-1][y]+
                        coordinate[x-1][y+1]+
                        coordinate[x][y-1]+
                        coordinate[x][y+1]+
                        coordinate[x+1][y-1]+
                        coordinate[x+1][y]+
                        coordinate[x+1][y+1];
            
            if (coordinate[x][y]){
                if (count!=2&&count!=3){
                    nextCoordinate[x][y]=false;
                }
                else{
                    nextCoordinate[x][y]=true;
                }
            }
            else{
                if (count==3){
                    nextCoordinate[x][y]=true;
                }
                else{
                    nextCoordinate[x][y]=false;
                }
            }

        }
    }
    

}

void copyBound(int subX_size, int world_rank, int world_size, int Y_limit, bool **coordinate) {

    MPI_Status status;
    int up_rank  = world_rank - 1;
    int down_rank = world_rank + 1;

    enum TAGS {
        TOUP,
        TODOWN
    };

    // Some MPIs deadlock if a single process tries to communicate
    // with itself
    if (world_size != 1) {
        // copy sides to neighboring processes
        if (up_rank < 0) {
            MPI_Recv(coordinate[subX_size+1], Y_limit+2, MPI_C_BOOL, down_rank, TOUP, MPI_COMM_WORLD, &status);
            MPI_Send(coordinate[subX_size], Y_limit+2, MPI_C_BOOL, down_rank, TODOWN, MPI_COMM_WORLD);
        } else if (down_rank >= world_size) {
            MPI_Send(coordinate[1], Y_limit+2, MPI_C_BOOL, up_rank, TOUP, MPI_COMM_WORLD);
            MPI_Recv(coordinate[0], Y_limit+2, MPI_C_BOOL, up_rank, TODOWN, MPI_COMM_WORLD, &status);
        } else {
            MPI_Sendrecv(coordinate[1], Y_limit+2, MPI_C_BOOL, up_rank, TOUP,
                coordinate[subX_size+1], Y_limit+2, MPI_C_BOOL, down_rank, TOUP,
                MPI_COMM_WORLD, &status);
            
            MPI_Sendrecv(coordinate[subX_size], Y_limit+2, MPI_C_BOOL, down_rank, TODOWN, 
                coordinate[0], Y_limit+2, MPI_C_BOOL, up_rank, TODOWN, 
                MPI_COMM_WORLD, &status);
        }
    }

}

int main(int argc, char** argv) {
    bool **coordinate;
    bool **nextCoordinate;
    bool **totalCoordinate;

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    if (argc!=5) {
        MPI_Abort(MPI_COMM_WORLD, 1);
        return -1;
    }

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();

    int iteration;
    sscanf(argv[2], "%d", &iteration);

    int X_limit;
    sscanf(argv[3], "%d", &X_limit);

    int Y_limit;
    sscanf(argv[4], "%d", &Y_limit);

    
    if (world_size == 1){

        coordinate = (bool **) malloc((X_limit+2)*sizeof(bool*));
        coordinate[0] = (bool *) calloc ((X_limit+2)*(Y_limit+2), sizeof(bool));
        int i, x, y;
        for(x = 0; x < X_limit+2; x++){
            coordinate[x] = (*coordinate + (Y_limit+2) * x);
        }

        
        nextCoordinate = (bool **) malloc((X_limit+2)*sizeof(bool*));
        nextCoordinate[0] = (bool *) calloc ((X_limit+2)*(Y_limit+2), sizeof(bool));
        for(x = 0; x < X_limit+2; x++){
            nextCoordinate[x] = (*nextCoordinate + (Y_limit+2) * x);
        }

        FILE *fp;
        fp = fopen (argv[1], "r");
        int row;
        int col;
        while(fscanf( fp,"%d %d", &row, &col )==2){
            coordinate[row+1][col+1] = true;
            //nextCoordinate[row+1][col+1] = true;
        }
        fclose(fp);

        bool **temp;

        for(i=0;i<iteration;++i){
            for(x=1;x<=X_limit;++x){
                for(y=1;y<=Y_limit;++y){
                    int count = coordinate[x-1][y-1]+
                                coordinate[x-1][y]+
                                coordinate[x-1][y+1]+
                                coordinate[x][y-1]+
                                coordinate[x][y+1]+
                                coordinate[x+1][y-1]+
                                coordinate[x+1][y]+
                                coordinate[x+1][y+1];
                    
                    if (coordinate[x][y]){
                        if (count!=2&&count!=3){
                            nextCoordinate[x][y]=false;
                        }
                        else{
                            nextCoordinate[x][y]=true;
                        }
                    }
                    else{
                        if (count==3){
                            nextCoordinate[x][y]=true;
                        }
                        else{
                            nextCoordinate[x][y]=false;
                        }
                    }

                }
            }
            temp = nextCoordinate; nextCoordinate = coordinate; coordinate = temp;
        }


        for(x=1;x<=X_limit;++x){
            for(y=1;y<=Y_limit;++y){
                if (coordinate[x][y])
                {
                    printf("%d %d\n", x-1, y-1);
                }
            }
        }

        

    } else {
        int subX_start;
        int subX_size;
        bool **temp;
        int* subX_start_arr;
        int* subX_size_arr;
        
        decompose_domain(X_limit, world_rank, world_size, &subX_start, &subX_size, &subX_start_arr, &subX_size_arr);
        initialize_boards(argv[1], world_rank, world_size, 
                        X_limit, Y_limit,
                        subX_start, subX_size,
                        subX_start_arr, subX_size_arr,
                        &coordinate, &nextCoordinate, &totalCoordinate);
        int i, x, y;
        for (i = 0; i < iteration; ++i)
        {
            // do send and receive
            // TODO
            copyBound(subX_size, world_rank, world_size, Y_limit, coordinate);

            // update and swap next with current
            cycleOfLife(subX_size, Y_limit, coordinate, nextCoordinate);
            temp = nextCoordinate; nextCoordinate = coordinate; coordinate = temp;

        }
        printf("gather start for %d\n", world_rank);
        MPI_Gatherv(coordinate[1], subX_size_arr[world_rank], MPI_C_BOOL,
            totalCoordinate[0], subX_size_arr, subX_start_arr, MPI_C_BOOL,
            0, MPI_COMM_WORLD);
        printf("gather end for %d\n", world_rank);
        
        // printf("start printing process %d\n", world_rank);
        if (world_rank==0) {
            for(x=0;x<X_limit;++x) {
                for(y=1;y<=Y_limit;++y) {
                    if (totalCoordinate[x][y]) {
                        printf("%d %d\n", x, y-1);
                    }
                }
            }
        }

        free(subX_size_arr);
        free(subX_start_arr);
        

    }


    free(coordinate[0]);
    free(coordinate);
    free(nextCoordinate[0]);
    free(nextCoordinate);
    

    if (world_rank == 0 && world_size!=1)
    {
        free(totalCoordinate[0]);
        free(totalCoordinate);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double end = MPI_Wtime();

    if (world_rank == 0) { /* use time on master node */
        printf("Runtime with %d processes on dataset %s = %f\n", world_size, argv[1], end-start);
    }

    MPI_Finalize();

    return 0;
}