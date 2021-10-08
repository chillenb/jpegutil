#include <turbojpeg.h>

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

  int inSubsamp, inColorspace;

  std::vector<std::byte> buf;

  Jpeg(std::vector<std::byte> data);
  ~Jpeg();

  static Jpeg loadFromFile(const std::string &filename);

  void loadDctCoeffs();

  class Dctcoeffs {
  public:
    Dctcoeffs(int width, int height);
    int h; // height in 8x8 blocks
    int w; // width in 8x8 blocks
    std::vector<short> coeffdata;
    int dummyFilter(short *coeffs, tjregion arrayRegion, tjregion planeRegion,
                    int componentIndex, int transformIndex,
                    tjtransform *transform);
  };
  Dctcoeffs *_coeffs;
};

template <typename T> struct Callback;

template <typename Ret, typename... Params> struct Callback<Ret(Params...)> {
  template <typename... Args> static Ret callback(Args... args) {
    return func(args...);
  }
  static std::function<Ret(Params...)> func;
};
