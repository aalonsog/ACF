# Introduction

This repository contains code for simulating the Adaptive Cuckoo Filter in two different versions: 

- Root files include the simulator code presented in the 2018 Proceedings of the Twentieth Workshop on Algorithm Engineering and Experiments (ALENEX) (paper is available [here](https://epubs.siam.org/doi/pdf/10.1137/1.9781611975055.4)) and an extension that use fingerprints with different ranges availbale in the paper published at the ACM Journal of Experimental Algorithmics (JEA) (paper is available [here](https://dl.acm.org/doi/pdf/10.1145/3339504)). Original repository containing these files is available [here](https://github.com/pontarelli/ACF).

- Code available under "reverse" directory demonstrates the constructions and replicates the simulations reported in "Cardinality Estimation Adaptive Cuckoo Filters(CE-ACF): Approximate Membership Check and Distinct Query Count for High-Speed Network Monitoring", in IEEE/ACM Transactions on Networking ([in press](https://doi.ieeecomputersociety.org/10.1109/TNET.2023.3302306)).

# Building

The simulator has been developed on Ubuntu 20.04. Other distributions or versions may need different steps.

Run the following commands in the "reverse" directory to build everything:

```
$ make
```

# Running

Use the following command to get help:

```
$ ./cestimate.exe -h


Usage:
 -c the number of distinct negatives queried on the filter
 -n number of repetitions of elements
 -b table size for the filter
 -o the percent occupancy of the filter, expressed as a number between 0 and 100
 -r the number of runs
 -h print usage
```

Use the following command to run de simulator with parameters:

```
$ ./cestimate.exe -c 256 -n 25 -b 4096 -o 95 -r 100
```

Number of fingerprint bits can be modified in the [source code](https://github.com/aalonsog/ACF/blob/cestimate/reverse/cestimate.cpp#L39). You need to compile again the code to apply the change. On the other hand, three different executables with the typical fingerprint values (7, 11 and 15) are provided.
    
