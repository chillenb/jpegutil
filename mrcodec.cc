#include "mrcodec.h"
#include "tables.h"
#include <jpeglib.h>
#include <stdint.h>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <gmp.h>

MixedRadixEncoder::MixedRadixEncoder(const int quant_table[], const int weight_table[]) {
    qtable = quant_table;
    wtable = weight_table;
    int ii = 0;
    double szbits = 0;
    for(int i = 0; i < 64; i++) {
      if(wtable[i] != 0) {
        indices[ii] = i;
        radices[ii] = wtable[i];
        szbits += log2(2*wtable[i] + 1);
        ii++;
      }
    }
    n_radices = ii;
    bits_per_block = floor(szbits);
    buffer.resize(bits_per_block / 8 + 1);
    std::cerr << bits_per_block << std::endl;
    mpz_init2(bufint, bits_per_block+1);
    mpz_init2(bq, bits_per_block+1);
}

MixedRadixEncoder::~MixedRadixEncoder() {
  mpz_clear(bufint);
  mpz_clear(bq);
}

void MixedRadixEncoder::loadData(u8 *buf, u32 nbytes) {
  num_bytes = nbytes;
  total_bitsize = 8*(num_bytes + 4);
  data.resize(4 + num_bytes);
  // write size in little endian format
  data[0] = nbytes & 0xFF;
  data[1] = (nbytes >> 8) & 0xFF;
  data[2] = (nbytes >> 16) & 0xFF;
  data[3] = (nbytes >> 24) & 0xFF;
  std::memcpy(&data[4], buf, num_bytes);
}

void MixedRadixEncoder::getNextChunk(mpz_t result) {
  std::fill(buffer.begin(), buffer.end(), 0);
  for(size_t btidx = 0; btidx < bits_per_block && btidx + next_bit < total_bitsize; btidx++) {
    size_t dt_index = btidx + next_bit;
    u8 bit = (data[(dt_index)/8] & (1 << (dt_index % 8))) ? 1 : 0;
    buffer[btidx / 8] |= (bit << (btidx % 8));
    /*
    if(bit)
      std::cout << "1";
    else
      std::cout << "0";
    */
  }
  //std::cout << "\n";
  //std::cerr << "Getting chunk at bit " << next_bit << std::endl;
  mpz_import(result, buffer.size(), -1, sizeof(u8), -1, 0, &buffer[0]);
  next_bit += bits_per_block;
}

bool MixedRadixEncoder::isDone() {
  if(next_bit >= total_bitsize) {
    return true;
  } else {
    return false;
  }
}

void MixedRadixEncoder::writeCoeffs(JCOEFPTR ptr) {
  std::fill(ptr, ptr+DCTSIZE2, 0);
  if(!isDone()) {
    getNextChunk(bufint);
    for(int n = 0; n < n_radices; n++) {
      unsigned long int bn = radices[n] * 2 + 1;
      unsigned long int xn = mpz_tdiv_q_ui(bq, bufint, bn);
      ptr[indices[n]] = (int) xn - (int) radices[n];
      if(!isDone())
      //std::cout << "(" << indices[n] << ", " << (int) radices[n] << ", " << (int) xn - (int) radices[n] << ")\n";
      mpz_set(bufint, bq);
    }
  } else {
    mpz_set_ui(bufint, 0);
  }

}

