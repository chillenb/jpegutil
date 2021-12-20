#include <iostream>
#include <string>

#include "jpegutil.h"
#include "tables.h"
#include "mrcodec.h"

int main(int argc, char **argv) {
  if (argc <= 2)
    return 0;

  Jpeg myjpeg(qtable_mms, weight_table_mms);

  myjpeg.loadFromFile(std::string(argv[2]));
  myjpeg.initCompress(std::string(argv[1]));


  /*
  myjpeg.readJpeg(std::string(argv[1]));
  std::cout << "Image is " << myjpeg.w << " by " << myjpeg.h << "\n";
  std::cout << "Image has " << myjpeg.cinfo.num_components << " components\n";
  myjpeg.loadDctCoeffs();
  print8by8<>(&(myjpeg.qtable[0]));
  */
  return 0;
  
}
