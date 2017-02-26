#!/bin/bash
#SBATCH --ntasks=32
#SBATCH -t 00:00:30
#SBATCH --mem-per-cpu=2048
#SBATCH --share


mpirun life_mpi final.data 500 500 500