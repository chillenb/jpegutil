#include "jpegutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

using std::vector;
using std::byte;
using std::ifstream;
using std::string;

vector<byte> loadFile(const string& filename) {
  FILE * f;
  struct stat fs;
  stat(filename.c_str(), &fs);
  size_t fsize = fs.st_size;
  vector<byte> arr(fsize);

  f = fopen(filename.c_str(), "rb");
  fread(&arr[0], fsize, 1, f);
  fclose(f);

  return arr;
}