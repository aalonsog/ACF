#include <iostream>
#include <fstream>
#include <random>
#include <unordered_map>
#include <vector>
#include <cstdio>
#include <ctime>

#include "four-one.hpp"

using namespace std;


void PrintUsage() {
  printf("Usage:\n");
  printf(" ***\n");
  printf(" -c the number of distinct false positives\n");
  printf(" -n one quarter the size of the filter\n");
  printf(" -b one quarter the size of the filter\n");
  printf(" -o the percent occupancy of the filter, expressed as a number between 0 and 100\n");
  printf(" -r the number of runs\n");
  printf(" -h print usage\n");
}

int main(int argc, char** argv) {

  // variables from the paper:
  // ./cestimate.exe -c 256 -n 25 -b 4096 -o 95 -r 100


  // the number of distinct false positives
  unsigned long ul;
  size_t C;
  // the number of queries of these false positives
  size_t N;
  // one quarter the size of the filter
  size_t b;
  // the number of fingerprint bits
  constexpr int f = 15;
  //int f = atoi(argv[4]);
  // the percent occupancy of the filter, expressed as a number between 0 and 100
  double o;
  // the number of runs
  int r;
  
  while (argc > 1 && argv[1][0] == '-') {
    argc--;
    argv++;
    int flag = 0;  // if flag 1 there is an argument after the switch
    int c = 0;
    while ((c = *++argv[0])) {
      switch (c) {
        case 'c':
          flag = 1;
          ul = strtoul (argv[1], NULL, 0);
          C = ul << 10;
          argc--;
          break;
          break;
        case 'n':
          flag = 1;
          N = atoi(argv[1]) * C;
          argc--;
          break;
        case 'b':
          flag = 1;
          b = atoi(argv[1]);
          argc--;
          break;
        case 'o':
          flag = 1;
          o = strtod(argv[1],NULL)/100;
          argc--;
          break;
        case 'r':
          flag = 1;
          r = atoi(argv[1]);
          argc--;
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
  
  cout << "Number of distinct false positives: " << C << "\n";
  cout << "Number of queries of these false positives: " << N << "\n";
  cout << "One quarter the size of the filter: " << b << "\n";
  cout << "Number of fingerprint bits: " << f << "\n";
  cout << "Percent occupancy of the filter: " << o << "\n";
  cout << "Number of runs: " << r << "\n";

  random_device d;
  auto Rand = [&d]() { return (static_cast<uint64_t>(d()) << 32) | d(); };
    // Don't need a hash function when the keys are fully random!
  auto Id = [](uint64_t x) { return x; };
  MS64 hash_makers[4] = {MS64::FromDevice(), MS64::FromDevice(), MS64::FromDevice(),
                         MS64::FromDevice()};
  MS128 fingerprint_maker = MS128::FromDevice();
  auto MagicMod = [](uint64_t x, uint64_t y) {
    return (static_cast<unsigned __int128>(x) * static_cast<unsigned __int128>(y)) >> 64;
  };

  ofstream myfile;

  char name[200];
  sprintf(name, "%d_%d_%d_%d_%d_%d.txt", C, N, b, f, o, r);

  myfile.open(name);

  for (int i = 0; i < r; ++i) {
    // the keys that we use. keys[k] = false if a key is not in the map, true otherwise
    unordered_map<uint64_t, bool> keys;
    for (size_t i = 0; i < C; ++i) {
      keys[Rand()] = false;
    }

    FourOne<uint64_t, f, 1 /* adaptivity bits */, decltype(Id)> filter(Id, b, hash_makers,
                                                                       fingerprint_maker);
    for (size_t i = 0; i < b * 4 * o; ++i) {
      auto key = Rand();
      keys[key] = true;
      // fill the filter
      filter.Insert(key, key);
    }

    vector<uint64_t> false_positives;
    for (const auto& x : keys) {
      if (not x.second) false_positives.push_back(x.first);
    }

    for (size_t i = 0; i < N; ++i) {
      auto index = MagicMod(Rand(), false_positives.size());
      filter.AdaptiveFind(index, index);
    }

    // print the estimate
    cout << 1.0 * filter.AdaptedCount() / filter.Count();
    cout << "\t" << false_positives.size() << endl;

    // save the estimate
    myfile << 1.0 * filter.AdaptedCount() / filter.Count();
    myfile << "\t" << false_positives.size() << endl;

  }
  myfile.close();
}
