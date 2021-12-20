#ifndef MRCODEC_H
#define MRCODEC_H

#include <stdio.h>
#include <array>
#include <vector>
#include <cstddef>
#include <stdint.h>
#include <jpeglib.h>
#include <gmp.h>

typedef uint8_t u8;
typedef uint32_t u32;
using std::vector;
using std::array;

class MixedRadixEncoder {
  public:
  const int *qtable;
  const int *wtable;
  int n_radices;
  u32 num_bytes;
  u32 total_bitsize;
  int bits_per_block;
  size_t next_bit = 0;
  array<int, 64> radices = {0};
  array<int, 64> indices = {0};
  vector<u8> data;
  vector<u8> buffer;
  mpz_t bufint;
  mpz_t bq;
  MixedRadixEncoder();
  MixedRadixEncoder(const int quant_table[], const int weight_table[]);
  ~MixedRadixEncoder();
  void loadData(u8 *buf, u32 nbytes);
  void writeCoeffs(JCOEFPTR ptr);
  void getNextChunk(mpz_t result);
  bool isDone();

};


#endif //MRCODEC_H
