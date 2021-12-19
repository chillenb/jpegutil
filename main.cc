#include <iostream>
#include <string>

#include "jpegutil.h"

int main(int argc, char **argv) {
  if (argc <= 1)
    return 0;

  Jpeg myjpeg;
  myjpeg.readJpeg(std::string(argv[1]));
  std::cout << "Image is " << myjpeg.w << " by " << myjpeg.h << "\n";
  std::cout << "Image has " << myjpeg.cinfo.num_components << " components\n";
  myjpeg.loadDctCoeffs();
  print8by8<>(&(myjpeg.qtable[0]));
  return 0;
}
