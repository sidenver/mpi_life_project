#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mpi.h>

void decompose_domain(int X_limit, int world_rank,
                      int world_size, int* subX_start,
                      int* subX_size) {
  if (world_size > X_limit) {
    // Don't worry about this special case. Assume the domain size
    // is greater than the world size.
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  *subX_start = X_limit / world_size * world_rank;
  *subX_size = X_limit / world_size;
  if (world_rank == world_size - 1) {
    // Give remainder to last process
    *subX_size += X_limit % world_size;
  }
  printf("process %d start with %d with size %d\n", world_rank, *subX_start, *subX_size);
}

void initialize_boards(char* filename, int world_rank, int world_size, 
                        int X_limit, int Y_limit,
                        int subX_start, int subX_size,
                        bool ***coordinate, bool ***nextCoordinate) {
    MPI_Status status;
    if (world_rank == 0)
    {
        bool **totalCoordinate = (bool **) malloc((X_limit)*sizeof(bool*));
        totalCoordinate[0] = (bool *) calloc ((X_limit)*(Y_limit+2), sizeof(bool));
        int i, x;
        for(x = 0; x < X_limit; x++){
            totalCoordinate[x] = (*totalCoordinate + (Y_limit+2) * x);
        }

        *coordinate = (bool **) malloc((subX_size+2)*sizeof(bool*));
        (*coordinate)[0] = (bool *) calloc ((subX_size+2)*(Y_limit+2), sizeof(bool));
        for(x = 0; x < subX_size+2; x++){
            (*coordinate)[x] = (**coordinate + (Y_limit+2) * x);
        }

        FILE *fp;
        fp = fopen (filename, "r");
        int row;
        int col;
        while(fscanf( fp,"%d %d", &row, &col )==2) {
            totalCoordinate[row][col + 1] = true;
            if (row < subX_size) {
                (*coordinate)[row + 1][col + 1] = true;
            }
            //nextCoordinate[row+1][col+1] = true;
        }
        fclose(fp);

        for (i = 1; i < world_size; ++i)
        {
            if (i!=world_size-1){
                MPI_Send(totalCoordinate[i * subX_size], 
                    subX_size*(Y_limit+2), MPI_C_BOOL, i, 0, MPI_COMM_WORLD);
            } else {
                MPI_Send(totalCoordinate[i * subX_size], 
                    (subX_size + X_limit % subX_size)*(Y_limit+2), MPI_C_BOOL, i, 0, MPI_COMM_WORLD);
            }
        }

        free(totalCoordinate[0]);
        free(totalCoordinate);

    } else {
        (*coordinate) = (bool **) malloc((subX_size+2)*sizeof(bool*));
        (*coordinate)[0] = (bool *) calloc ((subX_size+2)*(Y_limit+2), sizeof(bool));
        int x;
        for(x = 0; x < subX_size+2; x++){
            (*coordinate)[x] = (**coordinate + (Y_limit+2) * x);
        }

        MPI_Recv((*coordinate)[1], (subX_size)*(Y_limit+2),
           MPI_C_BOOL, 0, 0, MPI_COMM_WORLD,
           &status);
        int incoming_size;
        MPI_Get_count(&status, MPI_C_BOOL, &incoming_size);
        printf("incoming size for process %d is %d, where should be %d\n", world_rank, incoming_size, (subX_size)*(Y_limit+2));
    }
    
    *nextCoordinate = (bool **) malloc((subX_size+2)*sizeof(bool*));
    (*nextCoordinate)[0] = (bool *) calloc ((subX_size+2)*(Y_limit+2), sizeof(bool));
    int x;
    for(x = 0; x < subX_size+2; x++){
        (*nextCoordinate)[x] = (**nextCoordinate + (Y_limit+2) * x);
    }
}

// void cycleOfLife(int subX_size, int Y_limit,
//                  bool **coordinate, bool **nextCoordinate) {
//     bool **temp;
//     for(int x=1;x<=subX_size;++x){
//         for(int y=1;y<=Y_limit;++y){
//             int count = coordinate[x-1][y-1]+
//                         coordinate[x-1][y]+
//                         coordinate[x-1][y+1]+
//                         coordinate[x][y-1]+
//                         coordinate[x][y+1]+
//                         coordinate[x+1][y-1]+
//                         coordinate[x+1][y]+
//                         coordinate[x+1][y+1];
            
//             if (coordinate[x][y]){
//                 if (count!=2&&count!=3){
//                     nextCoordinate[x][y]=false;
//                 }
//                 else{
//                     nextCoordinate[x][y]=true;
//                 }
//             }
//             else{
//                 if (count==3){
//                     nextCoordinate[x][y]=true;
//                 }
//                 else{
//                     nextCoordinate[x][y]=false;
//                 }
//             }

//         }
//     }
//     temp = nextCoordinate; nextCoordinate = coordinate; coordinate = temp;

// }

// void send_boundary(bool* outgoing, int Y_limit, int target) {
//   // Send the data as an array of MPI_BYTEs to the next process.
//   // The last process sends to process zero.
//   MPI_Send(outgoing,
//            Y_limit+2, 
//            MPI_C_BOOL,
//            target, 0, MPI_COMM_WORLD);

// }

// void receive_boundary(bool* incoming, int Y_limit, int sender) {
//   // Status for new incoming
//   MPI_Status status;

//   // Resize your incoming walker buffer based on how much data is
//   // being received
//   MPI_Recv(incoming, Y_limit+2,
//            MPI_C_BOOL, sender, 0, MPI_COMM_WORLD,
//            &status);

//   int incoming_walkers_size;
//   MPI_Get_count(&status, MPI_C_BOOL, &incoming_walkers_size);
//   if (incoming_walkers_size!=Y_limit+2)
//   {
//       // recieve does not match
//       MPI_Abort(MPI_COMM_WORLD, 1);
//   }
// }


int main(int argc, char** argv) {
    bool **coordinate;
    bool **nextCoordinate;

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
        
        decompose_domain(X_limit, world_rank, world_size, &subX_start, &subX_size);
        initialize_boards(argv[1], world_rank, world_size, 
                        X_limit, Y_limit,
                        subX_start, subX_size,
                        &coordinate, &nextCoordinate);
        int x, y;
        printf("start printing process %d\n", world_rank);
        for(x=1;x<=subX_size;++x){
            for(y=1;y<=Y_limit;++y){
                if (coordinate[x][y])
                {
                    printf("%d %d\n", subX_start+x-1, y-1);
                }
            }
        }

    }


    free(coordinate[0]);
    free(coordinate);
    free(nextCoordinate[0]);
    free(nextCoordinate);
    MPI_Finalize();

    return 0;
}