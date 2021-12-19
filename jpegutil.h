#ifndef JPEGUTIL_H
#define JPEGUTIL_H

#include <stdio.h>
#include <jpeglib.h>

#include <iostream>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <array>

template <typename T>
void print8by8(T *data) {
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      std::cout << data[8*i+j] << " ";
    }
    std::cout << "\n";
  }
}

std::vector<std::byte> loadFile(const std::string &filename);

class Jpeg {
public:
  unsigned int h; // height
  unsigned int w; // width
  size_t size;
  Jpeg();
  ~Jpeg();

  int inSubsamp, inColorspace;

  std::vector<std::byte> buf;
  
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  std::vector<short> dctcoeffs;
  std::vector<unsigned short> qtable;

  void readJpeg(const std::string &filename);

  void loadFromFile(const std::string &filename);

  void loadDctCoeffs();

private:
  FILE *infile = NULL;
};

#endif // JPEGUTIL_H