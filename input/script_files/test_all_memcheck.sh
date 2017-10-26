#!/bin/bash
#PBS -A rck-371-aa
#PBS -l walltime=00:30:00
#PBS -l nodes=1:ppn=12
#PBS -q hb
#PBS -o outputfile
#PBS -e errorfile
#PBS -V
#PBS -N pz

# The executable to use
EXECUTABLE=@CMAKE_BINARY_DIR@/bin/test_all

# Specify the test case to run (Not used (but still required) when DTEST is enabled in the Makefile)
TESTCASE="SupersonicVortex"

# Specify the number of processor to run on (this should have correspondence with 'nodes' above)
N_PROCS="1"

# Specify the name of the file where output should be placed (leave empty to use stdout)
LOGFILE=""
#LOGFILE=" > logfile"

# Specify the path to the mpi executable (mpiexec)
MPI_DIR=""

# Specify whether the code should be run using valgrind and choose your options
USE_VALGRIND="1"

if [ "$USE_VALGRIND" = "1" ]; then
  VALGRIND_OPTS="valgrind \
                   --track-origins=yes \
                   --leak-check=yes \
                   --num-callers=20 \
                   --suppressions=../external/valgrind/valgrind.supp \
                "
#                   --gen-suppressions=all \
else
  VALGRIND_OPTS=""
fi



# ADDITIONAL OPTIONS
alias BEGINCOMMENT="if [ ]; then"
alias ENDCOMMENT="fi"

# Various valgrind options
BEGINCOMMENT
	valgrind --track-origins=yes
	         --leak-check=yes
	         --leak-check=full
	         --show-reachable=yes
	         -v # Gives additional information
	         --gen-suppressions=all # Generator suppression information
ENDCOMMENT



# DO NOT MODIFY ANYTHING BELOW THIS LINE

#clear
${MPI_DIR}mpiexec -n ${N_PROCS} ${VALGRIND_OPTS} ${EXECUTABLE} ${TESTCASE} ${LOGFILE}
#${VALGRIND_OPTS} ${MPI_DIR}mpiexec -n ${N_PROCS} ${EXECUTABLE} ${TESTCASE} ${LOGFILE}
