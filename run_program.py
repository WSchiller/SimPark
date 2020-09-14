import os

cmd = "g++ Functional_Decomposition.cpp -o Functional_Decomposition -lm -fopenmp"
os.system(cmd)
cmd ="./Functional_Decomposition"
os.system(cmd)
