This repo contains code that demonstrates the constructions and replicates the simulations reported in "On the Privacy of Adaptive Cuckoo Filters: Analysis and Protection", submitted to IEEE Transactions on Information Forensics and Security.


To build, run "make".

By default the code is configured to test with Preprocessing Reduction ("PR").
To change this, change the value of the bool "protect" in main.cpp.

To run, use one of the following two templates:

```
./main.exe f_p
```

where `f_p` is between 4 and 16 (inclusive), indicating the number of bits in the fingerprints.

or use

```
cat wordlist | ./main.exe f_p dict invr
```

where `wordlist` is a text file containing the lexicon with words separated by whitespace, `f_p` is as above, `dict` is just the string "dict", and `invr` is 32-r, where r (less than 32) is the number of bits preserved by the preprocessing reduction.

The output is a CSV file that looks like

```
recovered24,positives24,fpp24,recovered411,positives411,fpp411,recovered412,positives412,fpp412,recovered413,positives413,fpp412,fill_count
20598,43670,1.42918,20606,43677,1.06726,20609,43670,1.06047,20609,43670,1.06,31131
```

`recovered` indicates the number of keys successfully inverted, `positives` indicates the number of keys that are *persistent* false positives, `fpp` indicates the false positive probability (as a percentage, so times 100), and `fill_count` is the number of keys originally inserted.
The suffix `24` indicates an adaptive cuckoo filter ("ACF") with buckets of size 4 and with 2 buckets possible per key.
The suffix `41x` indicates an ACF with buckets of size 1 and 4 buckets possible per key and x fingerprint function selection bits in each slot.
