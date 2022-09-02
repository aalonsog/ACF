# Introduction

This repository contains code for simulating the Adaptive Cuckoo Filter. It demonstrates the constructions and replicates the simulations reported in "Make your Cuckoo Filter Adaptive and Get Cardinality Estimation for Free", submitted to IEEE Transactions on Knowledge and Data Engineering.

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
 -c the number of distinct false positives
 -n one quarter the size of the filter
 -b one quarter the size of the filter
 -o the percent occupancy of the filter, expressed as a number between 0 and 100
 -r the number of runs
 -h print usage
```

Use the following command to run de simulator with parameters:

```
$ ./cestimate.exe -c 256 -n 25 -b 4096 -o 95 -r 100
```

Number of fingerprint bits can be modified in the [source code](https://github.com/aalonsog/ACF/blob/cestimate/reverse/cestimate.cpp#L39). You need to compile again the code to apply the change. On the other hand, three diefferent executables with the typical fingerprint values (7, 11 and 15) are provided.
    
