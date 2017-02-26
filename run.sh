#!/bin/bash
#SBATCH --ntasks=4
#SBATCH -t 00:00:10
#SBATCH --mem-per-cpu=2048
#SBATCH --exclusive

module unload intel
module load openmpi/gnu

mpirun life_mpi ./life.data.1 100 250 250