#include <iostream>
#include <string>

#include "jpegutil.h"

int main(int argc, char **argv) {
  if (argc <= 1)
    return 0;

  Jpeg myjpeg = Jpeg::loadFromFile(std::string(argv[1]));
  std::cout << "Image is " << myjpeg.w << " by " << myjpeg.h << std::endl;
  myjpeg.loadDctCoeffs();
  std::cout << myjpeg._coeffs->coeffdata[0] << std::endl;
  return 0;
}