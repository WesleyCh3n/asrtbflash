#include <memory>

namespace lzma {
std::unique_ptr<unsigned char[]> Compress(const unsigned char *input,
                                          size_t inputSize, size_t *outputSize);

std::unique_ptr<unsigned char[]> Decompress(const unsigned char *input,
                                            unsigned int inputSize,
                                            size_t *outputSize);
} // namespace lzma
