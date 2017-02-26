#!/bin/bash
#SBATCH --ntasks=2
#SBATCH -t 00:00:10
#SBATCH --mem-per-cpu=2048
#SBATCH --exclusive

module unload intel
module load openmpi/gnu

mpirun ping_pong