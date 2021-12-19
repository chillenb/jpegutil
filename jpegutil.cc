#include "jpegutil.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <jpeglib.h>

#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#define DEBUG

const size_t maxFileSize = (size_t)134217728UL;

using std::byte;
using std::ifstream;
using std::string;
using std::vector;



void Jpeg::loadFromFile(const string &filename) {
  buf = loadFile(filename);
};

Jpeg::Jpeg() {
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
}

void Jpeg::readJpeg(const std::string &filename) {
  struct stat fs;
  if (stat(filename.c_str(), &fs) != 0) {
    std::cerr << "Could not stat file " << filename << ": ";
    fprintf(stderr, "%s\n", strerror(errno));
    exit(-1);
  }

  if((infile = fopen(filename.c_str(), "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename.c_str());
    exit(-1);
  }
  jpeg_stdio_src(&cinfo, infile);
  jpeg_read_header(&cinfo, TRUE);
  w = cinfo.image_width;
  h = cinfo.image_height;

}

Jpeg::~Jpeg() {
  jpeg_destroy_decompress(&cinfo);
  if(infile != NULL) {
    fclose(infile);
  }
}

void Jpeg::loadDctCoeffs() {
  jvirt_barray_ptr* coef_arrays = jpeg_read_coefficients(&cinfo);
  jpeg_component_info *compptr;
  compptr = cinfo.comp_info;
  JQUANT_TBL *qtblptr = compptr->quant_table;
  dctcoeffs.resize(compptr->height_in_blocks * compptr->width_in_blocks * DCTSIZE2);

  for(JDIMENSION blk_y = 0; blk_y < compptr->height_in_blocks;
      blk_y += compptr->v_samp_factor) {
    JBLOCKARRAY buffer = (cinfo.mem->access_virt_barray)
      ((j_common_ptr)&cinfo, coef_arrays[0], blk_y,
      (JDIMENSION)compptr->v_samp_factor, TRUE);
    for(int offset_y = 0; offset_y < compptr->v_samp_factor; offset_y++) {
      JBLOCKROW block = buffer[offset_y];
      for(JDIMENSION blk_x = 0; blk_x < compptr->width_in_blocks; blk_x++) {
        JCOEFPTR ptr = block[blk_x];
        short *buf = &(dctcoeffs[(blk_x + blk_y * compptr->width_in_blocks)*DCTSIZE2]);
        for(int k = 0; k < DCTSIZE2; k++) {
            buf[k] = ptr[k];
        }
      }
    }
  }

  qtable.resize(DCTSIZE2);
  for(int k = 0; k < DCTSIZE2; k++) {
    qtable[k] = qtblptr->quantval[k];
  }
  jpeg_finish_decompress(&cinfo);
}

vector<byte> loadFile(const string &filename) {
  FILE *f;
  struct stat fs;
  if (stat(filename.c_str(), &fs) != 0) {
    std::cerr << "Could not stat file " << filename << ": ";
    fprintf(stderr, "%s\n", strerror(errno));
    exit(-1);
  }
  size_t fsize = fs.st_size;

  if (fsize <= maxFileSize && fsize > 0) {
    vector<byte> arr(fsize);
    f = fopen(filename.c_str(), "rb");
    fread(&arr[0], fsize, 1, f);
    fclose(f);
    return arr;
  } else {
    if (fsize > 0)
      fprintf(stderr, "File size too large\n");
    else
      fprintf(stderr, "File empty\n");
    exit(-1);
  }
}

