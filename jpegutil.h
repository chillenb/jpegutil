#include <vector>
#include <cstddef>
#include <string>

std::vector<std::byte> loadFile(const std::string& filename);

class Jpeg {
  public:
    int h; //height
    int w; //width
    std::vector<std::byte> buf;

    Jpeg(std::vector<std::byte> data);
    ~Jpeg();

    static Jpeg loadFromFile(const std::string& filename);



  class Dctcoeffs;
  Dctcoeffs * _coeffs;

};

