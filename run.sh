#!/bin/bash
#SBATCH --ntasks=40
#SBATCH -t 00:01:00
#SBATCH --mem-per-cpu=2048
#SBATCH --exclusive

module unload intel
module load openmpi/gnu

mpirun mpi_hello