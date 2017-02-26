#!/bin/bash
#SBATCH --ntasks=2
#SBATCH -t 00:00:30
#SBATCH --mem-per-cpu=2048
#SBATCH --exclusive


mpirun life_mpi ./life.data.1 100 250 250