#ifndef JPEGUTIL_H
#define JPEGUTIL_H

#include <stdio.h>
#include <jpeglib.h>


#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

std::vector<std::byte> loadFile(const std::string &filename);

class Jpeg {
public:
  int h; // height
  int w; // width
  size_t size;
  Jpeg();
  ~Jpeg();

  int inSubsamp, inColorspace;

  std::vector<std::byte> buf;
  
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;

  void readJpeg(const std::string &filename);

  void loadFromFile(const std::string &filename);

  void loadDctCoeffs();

private:
  FILE *infile = NULL;
};

#endif // JPEGUTIL_H