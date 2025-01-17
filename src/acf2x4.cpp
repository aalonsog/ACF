#include <limits.h>
#include <string.h>
#include <time.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <random>  // http://en.cppreference.com/w/cpp/numeric/random
#include <vector>

#include "HTmap.hpp"
#include "utils.h"

bool quiet = false;
// int verbose=0; // define the debug level
int seed;

// d=2, b=4 (second variant)
int num_way = 2;     //# of ways (hash functions)
int num_cells = 4;   //# of slots in a rows
int ht_size = 1024;  //# of rows
int f = 9;           //# of fingerprint bits

int max_loop = 2;      // num of trials
int load_factor = 95;  // load factor
int AS = 32;
int A = 0;
// int npf=1000;
int npf = 10;

int64_t tot_access = 0;
int64_t tot_FF_FP = 0;

map<int64_t, int> S_map;
map<int64_t, int> A_map;
vector<int64_t> A_ar;

int fingerprint(int64_t key, int index, int f) { return hashg(key, 20 + index, 1 << f); }

int myrandom(int i) { return std::rand() % i; }

int run() {
  time_t starttime = time(NULL);
  int line = 0;

  // seed=1456047300;
  srand(seed);

  // Seed with a real random value, if available
  std::random_device rd;

  std::mt19937 gen(seed);
  std::uniform_int_distribution<> dis(1, INT_MAX);

  printf("***********************\n\n");
  printf("***:Summary: \n");
  printf("seed: %d\n", seed);

  // create the table;
  HTmap<int64_t, int> cuckoo(num_way, num_cells, ht_size, 1000);
  printf("\n***Cuckoo table \n");
  printf("***:way: %d\n", num_way);
  printf("***:num_cells: %d\n", num_cells);
  printf("***:Total table size: %d\n", cuckoo.get_size());
  printf("***:---------------------------\n");

  int*** FF = new int**[num_way];
  for (int i = 0; i < num_way; i++) {
    FF[i] = new int*[num_cells];
    for (int ii = 0; ii < num_cells; ii++) {
      FF[i][ii] = new int[ht_size];
    }
  }

  printf("***:ACF:\n");
  printf("***:fingerprint bits: %d\n", f);
  printf("***:Buckets: %d\n", num_way * num_cells * ht_size);
  printf("***:Total size (bits): %d\n", f * num_way * num_cells * ht_size);
  printf("***:---------------------------\n");

  setbuf(stdout, NULL);

  // main loop
  //
  int num_fails = 0;
  int64_t tot_i = (load_factor * cuckoo.get_size()) / 100;
  int64_t num_swap = 0;
  int64_t num_iter = 0;
  int64_t max_FF_FP = 0;
  int64_t min_FF_FP = INT_MAX;
  int64_t tot_count = 0;
  for (int loop = 0; loop < max_loop; loop++) {
    int64_t sample_FF_FP = 0;
    cuckoo.clear();
    S_map.clear();
    A_map.clear();
    A_ar.clear();
    bool fail_insert = false;

    // clear FF;
    for (int i = 0; i < num_way; i++)
      for (int ii = 0; ii < num_cells; ii++)
        for (int iii = 0; iii < ht_size; iii++) {
          FF[i][ii][iii] = -1;
        }

    for (int64_t i = 0; i < tot_i; i++) {
      // int64_t key= rand();
      // unsigned int key= (rand()*2^16)+rand();
      // jbapple: generate a random true positive and insert it into S_map.
      unsigned int key = (unsigned int)dis(gen);
      if (S_map.count(key) > 0) {
        i--;
        continue;
      }

      S_map[key] = line++;
      verprintf("insert key: %u \n", key);
      if ((i % 1000) == 0) {
        if (!quiet) fprintf(stderr, "loop: %d item: %lu\r", loop, i);
      }

      // insert in cuckoo HTmap
      if (!cuckoo.insert(key, line)) {
        verprintf(" Table full (key: %u)\n", key);
        num_fails++;
        fail_insert = true;
        break;
      }
    }
    if (fail_insert) {
      loop--;
      continue;
    }

    if (!quiet) fprintf(stderr, "\n");
    printf("End insertion\n");
    printf("---------------------------\n");
    printf("items= %d\n", cuckoo.get_nitem());
    printf("load(%d)= %f \n", loop, cuckoo.get_nitem() / (0.0 + cuckoo.get_size()));
    cuckoo.stat();

    // insert in ACF
    for (auto x : S_map) {
      auto res = cuckoo.fullquery(x.first);
      FF[std::get<1>(res)][std::get<2>(res)][std::get<3>(res)] =
          fingerprint(x.first, std::get<2>(res), f);
      // verprintf("item %ld is in FF[%d][%d][%d] with fingerprint
      // %d\n",key,std::get<1>(res),std::get<2>(res),std::get<3>(res),fingerprint(key));
    }
    printf("Inserted in ACF\n");

    // consistency check !!!
    /*for (auto x: S_map) {
        bool flagFF = false;
        for (int i = 0; i < num_way; i++) {
            int p = hashg(x.first, i, ht_size);
            for (int ii = 0; ii < num_cells; ii++) {
                if (fingerprint(x.first, ii, f) == FF[i][ii][p]) {
                    flagFF = true;
            }
        }
        if (!flagFF) {
            printf("Consistency ERROR 1 \n");
            exit(1);
        }
    }
    printf("1st Consistency passed\n");
    */
    // create A set
    for (int64_t i = 0; i < A; i++) {
      // jbapple: generate a random key and insert it into A_ar and A_map if and only if
      // it is a unique non-member of S_map.
      unsigned int key = (unsigned int)dis(gen);
      if ((A_map.count(key) > 0) || (S_map.count(key) > 0)) {
        i--;
        continue;
      }

      // insert in A_map and in A_ar npf times
      A_map[key] = line++;
      A_ar.push_back(key);

      verprintf("insert key: %u \n", key);
      if ((i % 1000) == 0) {
        if (!quiet) fprintf(stderr, "loop: %d. Create the A set: %lu\r", loop, i);
      }
    }
    if (!quiet) fprintf(stderr, "\n");

    // scramble  the A array
    // printf("A set of %lu elements created \n",A_ar.size());
    // random_shuffle(A_ar.begin(), A_ar.end(),myrandom);
    // printf("A set of %lu elements scrambled \n",A_ar.size());

    int64_t count = 0;
    int64_t ar_size = A_ar.size();
    num_iter = npf * ar_size;

    // test A set
    for (int64_t iter = 0; iter < num_iter; iter++) {
      int64_t key = A_ar[rand() % ar_size];
      // for (auto key: A_ar) {
      // ACF query
      count++;
      tot_count++;
      // jbapple: flagFF is true if this key, which is really absent, is a false positive
      bool flagFF = false;
      int false_i = -1;
      int false_ii = -1;
      for (int i = 0; i < num_way; i++) {
        int p = hashg(key, i, ht_size);
        for (int ii = 0; ii < num_cells; ii++) {
          if (fingerprint(key, ii, f) == FF[i][ii][p]) {
            flagFF = true;
            false_i = i;
            false_ii = ii;
          }
        }
      }
      if ((count % 1000) == 0) {
        if (!quiet)
          fprintf(stderr, "loop: %d. check the %lu element of A set\r", loop, count);
      }
      if (flagFF) {
        tot_FF_FP++;
        sample_FF_FP++;
      }

      // jbapple: adaptivity is here
      // SWAP
      if (flagFF) {
        num_swap++;
        int p = hashg(key, false_i, ht_size);
        int64_t key1 = cuckoo.get_key(false_i, false_ii, p);
        int value1 = cuckoo.query(key1);
        int jj = false_ii;
        while (jj == false_ii) jj = std::rand() % num_cells;
        int64_t key2 = cuckoo.get_key(false_i, jj, p);
        int value2 = cuckoo.query(key2);
        if (!cuckoo.remove(key1)) {  // false_ii is free
          printf("false_ii is free\n");
        }
        if (!cuckoo.remove(key2))  // jj is free
        {
          FF[false_i][false_ii][p] = -1;
        } else {
          cuckoo.direct_insert(key2, value2, false_i, false_ii);
          FF[false_i][false_ii][p] = fingerprint(key2, false_ii, f);
        }
        cuckoo.direct_insert(key1, value1, false_i, jj);
        FF[false_i][jj][p] = fingerprint(key1, jj, f);
      }
    }
    // consistency check after moving items!!!
    /*
    for (auto x: S_map) {
        bool flagFF = false;
        for (int i = 0; i < num_way; i++) {
            int p = hashg(x.first, i, ht_size);
            for (int ii = 0; ii < num_cells; ii++) {
                if (fingerprint(x.first, ii, f) == FF[i][ii][p]) {
                    flagFF = true;
                }
        }
        if (!(cuckoo.count(x.first)>0)) {
            printf("Consistency ERROR 3 \n");
            exit(1);
        }
        if (!flagFF) {
            printf("Consistency ERROR 2 (key: %ld)\n",x.first);
            exit(1);
        }
    }

    if (!quiet) fprintf(stderr, "\n");
    printf("2nd Consistency passed\n");
    */
    printf("ACF FP: %lu : %.6f \n", sample_FF_FP, sample_FF_FP / (count + 0.0));
    if (sample_FF_FP < min_FF_FP) min_FF_FP = sample_FF_FP;
    if (sample_FF_FP > max_FF_FP) max_FF_FP = sample_FF_FP;
  }  // end main loop

  printf("---------------------------\n");
  printf("---------------------------\n");
  printf("stat:ACF FP min/ave/max %lu %lu %lu \n", min_FF_FP, tot_FF_FP / max_loop,
         max_FF_FP);
  printf("stat:ACF FPR(%lu) : %.6f \n", tot_FF_FP, tot_FF_FP / (tot_count + 0.0));
  printf("stat:num SWAP : %ld \n", num_swap);

  cout << "results: " << f << ", " << ht_size << ", " << 8 << ", " << 0 << ", " << A
       << ", " << max_loop << ", " << npf << ", " << load_factor << ", " << tot_FF_FP
       << ", " << tot_count << endl;

  printf("\n");
  simtime(&starttime);
  return 0;
}

void PrintUsage() {
  printf("usage:\n");
  printf(" ***\n");
  printf(" -m tsize: Table size\n");
  printf(" -f f_bits: number of fingerprint bits\n");
  printf(" -n num_packets: number of packets for each flow \n");
  printf(" -a as_ratio: set the A/S ratio \n");
  printf(" -S seed: select random seed (for debug)\n");
  printf(" -L load_factor : set the ACF load factor \n");
  printf(" -v : verbose \n");
  printf(" -h print usage\n");
  printf(" -v verbose enabled\n");
}

void init(int argc, char* argv[]) {
  printf("\n===========================================\n");
  printf("Simulator for the Adaptive Cuckoo Filter with 4x1 tables\n");
  printf("Run %s -h for usage\n", argv[0]);
  printf("===========================================\n\n");

  // code_version();
  print_hostname();
  print_command_line(argc, argv);  // print the command line with the option
  seed = time(NULL);
  // Check for switches
  while (argc > 1 && argv[1][0] == '-') {
    argc--;
    argv++;
    int flag = 0;  // if flag 1 there is an argument after the switch
    int c = 0;
    while ((c = *++argv[0])) {
      switch (c) {
        case 'q':
          printf("\nQuiet enabled\n");
          quiet = true;
          break;
        case 'a':
          flag = 1;
          AS = atoi(argv[1]);
          argc--;
          break;
          break;
        case 'm':
          flag = 1;
          ht_size = atoi(argv[1]);
          argc--;
          break;
        case 'f':
          flag = 1;
          f = atoi(argv[1]);
          argc--;
          break;
        case 'S':
          flag = 1;
          seed = atoi(argv[1]);
          argc--;
          break;
        case 'n':
          flag = 1;
          npf = atoi(argv[1]);
          argc--;
          break;
        case 'L':
          flag = 1;
          load_factor = atoi(argv[1]);
          argc--;
          break;
        case 'v':
          printf("\nVerbose enabled\n");
          verbose += 1;
          break;
        case 'h':
          PrintUsage();
          exit(1);
          break;
        default:
          printf("Illegal option %c\n", c);
          PrintUsage();
          exit(1);
          break;
      }
    }
    argv = argv + flag;
  }
  A = ht_size * num_way * num_cells * AS;
  // Print general parameters
  printf("general parameters: \n");
  max_loop = 250 * (1 << ((f - 8) / 2)) / AS;
  printf("seed: %d\n", seed);
  printf("way: %d\n", num_way);
  printf("num_cells: %d\n", num_cells);
  printf("Table size: %d\n", ht_size);
  printf("A size: %d\n", A);
  printf("iterations: %d\n", max_loop);
  printf("AS ratio: %d\n", AS);
  printf("npf: %d\n", npf);
  printf("---------------------------\n");
}

int main(int argc, char** argv) {
  init(argc, argv);
  return run();
}
