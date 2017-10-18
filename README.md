# Parallel-programing
Using MPI to parallel programing in C

# 1. circuitSatisfiability.c
  Check every posibility of 32-bits input to a circuit and count the total number of output is "1".
  ### Result :
    1179567 solutions were found.
    1 process : 486.84 secs
    2 process : 278.97 secs
    4 process : 165.39 secs
    8 process : 96.16 secs
    16 process : 48.15 secs

# 2. MonteCarlo.c
  Use Monte Carlo method to estimate pi.
  Need to input the total number of tosses, larger toss can get a more reasonable estimate of pi.
  ### Result:
    Total toss :100000000
    The pi_estimate is 3.1414707200000000497652763
