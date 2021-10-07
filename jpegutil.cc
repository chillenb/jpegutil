#include "jpegutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <turbojpeg.h>

const size_t maxFileSize = (size_t)134217728UL;

using std::vector;
using std::byte;
using std::ifstream;
using std::string;


Jpeg::Jpeg(vector<byte> data) {
  buf = data;
  _coeffs = NULL;
}

Jpeg Jpeg::loadFromFile(const string& filename) {
  Jpeg newjpeg = Jpeg(loadFile(filename));
  return newjpeg;
};

class Jpeg::Dctcoeffs {
    public:
      int h; // height in 8x8 blocks
      int w; // width in 8x8 blocks
      vector<short> coeffdata;
};

Jpeg::~Jpeg() {
  if(_coeffs != NULL) {
    delete _coeffs;
  }
}

vector<byte> loadFile(const string& filename) {
  FILE * f;
  struct stat fs;
  stat(filename.c_str(), &fs);
  size_t fsize = fs.st_size;

  if(fsize <= maxFileSize && fsize > 0) {
    vector<byte> arr(fsize);
    f = fopen(filename.c_str(), "rb");
    fread(&arr[0], fsize, 1, f);
    fclose(f);
    return arr;
  } else {
    fprintf(stderr, "File size too large\n");
    exit(0);
  }
}