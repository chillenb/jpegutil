#ifndef JPEGUTIL_H
#define JPEGUTIL_H

#include <stdio.h>
#include <jpeglib.h>
#include <gmp.h>

#include <iostream>
#include <cstddef>
#include <functional>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <array>

#include "tables.h"
#include "mrcodec.h"

template <typename T>
void print8by8(T *data) {
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      std::cout << data[8*i+j] << " ";
    }
    std::cout << "\n";
  }
}

template <typename T, typename K>
void print8by8diff(T *orig, K *comp) {
  double diff = 0.0;
  for(int i = 0; i < 64; i++) {
    diff += std::abs((double) orig[i] - (double) comp[i]);
  }
  std::cout << diff << "\n";
}


std::vector<std::byte> loadFile(const std::string &filename);

class Jpeg {
public:
  unsigned int h = 512; // height
  unsigned int w = 512; // width
  int quality = 50; // quality
  size_t size;

  const int* qtable;
  const int* wtable;
  Jpeg(const int* qtable, const int* wtable) : qtable(qtable), wtable(wtable) {};

  ~Jpeg();

  int inSubsamp, inColorspace;

  std::vector<std::byte> buf;
  
  struct jpeg_decompress_struct cinfo;
  struct jpeg_compress_struct dinfo;
  struct jpeg_error_mgr jerr;
  jvirt_barray_ptr compress_coefarray;

  std::vector<short> dctcoeffs;
  std::vector<unsigned short> read_qtable;

  void initCompress(const std::string &filename);

  //void initDecompress();

  void readJpeg(const std::string &filename);

  void loadFromFile(const std::string &filename);

  void loadDctCoeffs();

private:
  FILE *infile = NULL;
  FILE *outfile = NULL;
};







#endif // JPEGUTIL_H
