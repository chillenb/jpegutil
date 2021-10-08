#include "jpegutil.h"
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <sys/stat.h>
#include <turbojpeg.h>
#include <memory>
#include <errno.h>
#include <cstring>
#include <functional>
#include <assert.h>
#define DEBUG

const size_t maxFileSize = (size_t)134217728UL;

using std::vector;
using std::byte;
using std::ifstream;
using std::string;


Jpeg::Jpeg(vector<byte> data) {
  buf = data;
}

Jpeg::~Jpeg() {
  if(_coeffs != NULL) {
    delete _coeffs;
  }
}


Jpeg Jpeg::loadFromFile(const string& filename) {
  Jpeg newjpeg = Jpeg(loadFile(filename));
  tjhandle tjInstance;
  if((tjInstance = tjInitDecompress()) == NULL) {
    std::cerr << "Error: Failed to initialize decompressor" << std::endl;
    std::cerr << tjGetErrorStr2(tjInstance) << std::endl;
    exit(-1);
  }

  if (tjDecompressHeader3(tjInstance, (const unsigned char*) &newjpeg.buf[0], newjpeg.buf.size(), &newjpeg.w, &newjpeg.h,
      &newjpeg.inSubsamp, &newjpeg.inColorspace) < 0) {
      std::cerr << "Error: Failed to load file " << filename << std::endl;
      std::cerr << tjGetErrorStr2(tjInstance) << std::endl;
      exit(-1);
    }
  tjDestroy(tjInstance);
  return newjpeg;
};


int myfunc(float a, short b, tjregion c) {
  return 0;
}

// Initialize the static member.
template <typename Ret, typename... Params>
std::function<Ret(Params...)> Callback<Ret(Params...)>::func;



Jpeg::Dctcoeffs::Dctcoeffs(int width, int height) {
  h = height; w = width;
  coeffdata = vector<short>(h*w*8*8);
  #ifdef DEBUG
  std::cout<<"dctcoeffs constructed"<<std::endl;
  #endif
}


void Jpeg::loadDctCoeffs() {
  tjhandle tjInstance;
  if ((tjInstance = tjInitTransform()) == NULL) {
    std::cerr << "Error: Failed to initialize transformer" << std::endl;
    std::cerr << tjGetErrorStr2(tjInstance) << std::endl;
  }
  int x_blocks = w/8 + static_cast<int>(w % 8 != 0);
  int y_blocks = h/8 + static_cast<int>(h % 8 != 0);

  _coeffs = new Jpeg::Dctcoeffs(x_blocks, y_blocks);
  tjtransform xform;
  memset(&xform, 0, sizeof(tjtransform));
  
  using namespace std::placeholders;

/* 
  stackoverflow template metaprogramming to make
  a c-style callback bound to a Dctcoeffs instance
  needed to allow the callback (a custom filter)
  to extract the DCT coefficients and write them to
  a buffer. Credit: user Snps.
  https://stackoverflow.com/questions/19808054/convert-c-function-pointer-to-c-function-pointer/19809787
 */

  Callback<int(short*, tjregion,
          tjregion, int, int, tjtransform*)>::func
          = std::bind(&Jpeg::Dctcoeffs::dummyFilter, _coeffs, _1,
          _2, _3, _4, _5, _6);

  int (*customFilterCallback)(short*, tjregion,
          tjregion, int, int, tjtransform*) = static_cast<decltype(customFilterCallback)>
          (Callback<int(short*, tjregion, tjregion, int, int, tjtransform*)>::callback);
  xform.customFilter = customFilterCallback;

  unsigned char *dstBuf = NULL;  /* Dynamically allocate the JPEG buffer */
  unsigned long dstSize = 0;


  xform.options |= TJXOPT_TRIM;
  if (tjTransform(tjInstance, (const unsigned char *) &buf[0], buf.size(), 1, &dstBuf, &dstSize,
                  &xform, TJFLAG_ACCURATEDCT) < 0) {
    tjFree(dstBuf);
  }
  tjFree(dstBuf);
  tjDestroy(tjInstance);
}


vector<byte> loadFile(const string& filename) {
  FILE * f;
  struct stat fs;
  if(stat(filename.c_str(), &fs) != 0) {
    std::cerr << "Could not stat file " << filename << ": ";
    fprintf(stderr, "%s\n", strerror(errno));
    exit(-1);
  }
  size_t fsize = fs.st_size;

  if(fsize <= maxFileSize && fsize > 0) {
    vector<byte> arr(fsize);
    f = fopen(filename.c_str(), "rb");
    fread(&arr[0], fsize, 1, f);
    fclose(f);
    return arr;
  } else {
    if(fsize > 0)
      fprintf(stderr, "File size too large\n");
    else
      fprintf(stderr, "File empty\n");
    exit(-1);
  }
}



int Jpeg::Dctcoeffs::dummyFilter(short *coeffs, tjregion arrayRegion,
                tjregion planeRegion, int componentIndex,
                int transformIndex, tjtransform *transform)
{
  int i;
  printf("%d x %d, (%d x %d)\n", arrayRegion.w, arrayRegion.h, arrayRegion.x, arrayRegion.y);
  // printf("%d %d %d\n", coeffs[0], coeffs[64], coeffs[65]);
  size_t offset = 8*arrayRegion.x + w*arrayRegion.y;
  for (i = 0; i < arrayRegion.w * arrayRegion.h; i++) {
    #ifdef DEBUG
    if(i + offset >= coeffdata.size()) {
      std::cerr<< i << ", " << coeffdata.size() <<std::endl;
    }
    assert(i+offset < coeffdata.size());
    #endif
    coeffdata[i+offset] = coeffs[i];
  }
  return 0;
}

