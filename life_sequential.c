#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main(int argc, char** argv) {
    if (argc!=5)
        return -1;

    int iteration;
    sscanf(argv[2], "%d", &iteration);

    int X_limit;
    sscanf(argv[3], "%d", &X_limit);

    int Y_limit;
    sscanf(argv[4], "%d", &Y_limit);

    bool **coordinate;
    coordinate = (bool **) malloc((X_limit+2)*sizeof(bool*));
    coordinate[0] = (bool *) calloc ((X_limit+2)*(Y_limit+2), sizeof(bool));
    for(int x = 0; x < X_limit+2; x++){
        coordinate[x] = (*coordinate + (Y_limit+2) * x);
    }

    bool **nextCoordinate;
    nextCoordinate = (bool **) malloc((X_limit+2)*sizeof(bool*));
    nextCoordinate[0] = (bool *) calloc ((X_limit+2)*(Y_limit+2), sizeof(bool));
    for(int x = 0; x < X_limit+2; x++){
        nextCoordinate[x] = (*nextCoordinate + (Y_limit+2) * x);
    }

    bool **temp;

    FILE *fp;
    int row;
    int col;
    fp = fopen (argv[1], "r");
    while(fscanf( fp,"%d %d", &row, &col )==2){
        coordinate[row+1][col+1] = true;
        //nextCoordinate[row+1][col+1] = true;
    }
    fclose(fp);

    for(int i=0;i<iteration;++i){
        for(int x=1;x<=X_limit;++x){
            for(int y=1;y<=Y_limit;++y){
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


    for(int x=1;x<=X_limit;++x){
        for(int y=1;y<=Y_limit;++y){
            if (coordinate[x][y])
            {
                printf("%d %d\n", x-1, y-1);
            }
        }
    }

    free(coordinate[0]);
    free(coordinate);
    free(nextCoordinate[0]);
    free(nextCoordinate);

    return 0;
}