#include "jpegutil.h"
#include "tables.h"
#include "mrcodec.h"

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



void Jpeg::initCompress(const std::string &filename) {
  dinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&dinfo);
  if ((outfile = fopen(filename.c_str(), "wb")) == NULL) {
    std::cerr << "Can't open " << filename << "\n";
    exit(1);
  }
  dinfo.image_width = w;
  dinfo.image_height = h;
  dinfo.in_color_space = JCS_GRAYSCALE;
  dinfo.input_components = 1;
  jpeg_stdio_dest(&dinfo, outfile);

  jpeg_set_defaults(&dinfo);
  //jpeg_set_quality(&dinfo, quality, TRUE);
  dinfo.num_components = 1;
  dinfo.optimize_coding = TRUE;
  //jpeg_simple_progression(&dinfo);
  //dinfo.arith_code = TRUE;

  dinfo.jpeg_color_space = JCS_GRAYSCALE;

  MixedRadixEncoder enc(qtable, wtable);
  enc.loadData((u8*) &buf[0], buf.size());

  if(dinfo.quant_tbl_ptrs[0] == NULL)
    dinfo.quant_tbl_ptrs[0] = jpeg_alloc_quant_table((j_common_ptr) &dinfo);
  for(int i = 0; i < DCTSIZE2; i++) {
    dinfo.quant_tbl_ptrs[0]->quantval[i] = qtable[i];
  }

  JDIMENSION width_in_blocks = (dinfo.image_width + 7) / 8;
  JDIMENSION height_in_blocks = (dinfo.image_height + 7) / 8;
  std::cout << dinfo.num_components << std::endl;
  jpeg_component_info *compptr = dinfo.comp_info;
  compptr->height_in_blocks = height_in_blocks;
  compptr->width_in_blocks = width_in_blocks;

  compress_coefarray = (dinfo.mem->request_virt_barray)
    ((j_common_ptr) &dinfo, JPOOL_IMAGE, FALSE, 
    width_in_blocks, height_in_blocks, (JDIMENSION) 1);

  dinfo.jpeg_width = dinfo.image_width;
  dinfo.jpeg_height = dinfo.image_height;
  jpeg_write_coefficients(&dinfo, &compress_coefarray);
  JBLOCKARRAY buffer;
  JBLOCKROW block;
  JDIMENSION blk_x, blk_y;
  for(blk_y = 0; blk_y < height_in_blocks; blk_y++) {
    buffer = (*dinfo.mem->access_virt_barray)
      ((j_common_ptr)&dinfo, compress_coefarray, blk_y,
       (JDIMENSION) 1, TRUE);
    block = buffer[0];
    for(blk_x = 0; blk_x < width_in_blocks; blk_x++) {
      JCOEFPTR ptr = block[blk_x];
      enc.writeCoeffs(ptr);
    }
  }
  jpeg_finish_compress(&dinfo);
}


void Jpeg::readJpeg(const std::string &filename) {
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
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
  //jpeg_destroy_decompress(&cinfo);
  if(infile != NULL) {
    fclose(infile);
  }
  if(outfile != NULL) {
    fclose(outfile);
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

  read_qtable.resize(DCTSIZE2);
  for(int k = 0; k < DCTSIZE2; k++) {
    read_qtable[k] = qtblptr->quantval[k];
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

