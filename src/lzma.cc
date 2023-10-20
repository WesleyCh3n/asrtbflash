#include "lzma.h"

#include "LzmaDec.h"
#include "LzmaEnc.h"

using namespace lzma;

static void *_lzmaAlloc(ISzAllocPtr, size_t size) {
  return new unsigned char[size];
}
static void _lzmaFree(ISzAllocPtr, void *addr) {
  if (!addr)
    return;
  delete[] reinterpret_cast<unsigned char *>(addr);
}

static ISzAlloc _allocFuncs = {_lzmaAlloc, _lzmaFree};

std::unique_ptr<unsigned char[]> lzma::Compress(const unsigned char *input,
                                                size_t inputSize,
                                                size_t *outputSize) {
  std::unique_ptr<unsigned char[]> result;

  // set up properties
  CLzmaEncProps props;
  LzmaEncProps_Init(&props);
  if (inputSize >= (1 << 20))
    props.dictSize = 1 << 20; // 1mb dictionary
  else
    props.dictSize = (unsigned int)inputSize; // smaller dictionary = faster!
  props.fb = 40;

  // prepare space for the encoded properties
  size_t propsSize = LZMA_PROPS_SIZE;
  unsigned char propsEncoded[LZMA_PROPS_SIZE];

  // allocate some space for the compression output
  // this is way more than necessary in most cases...
  // but better safe than sorry
  //   (a smarter implementation would use a growing buffer,
  //    but this requires a bunch of fuckery that is out of
  ///   scope for this simple example)
  size_t outputSize64 = inputSize * (size_t)1.5;
  if (outputSize64 < 1024)
    outputSize64 = 1024;
  auto output = std::make_unique<unsigned char[]>(outputSize64);

  int lzmaStatus =
      LzmaEncode(output.get(), &outputSize64, input, inputSize, &props,
                 propsEncoded, &propsSize, 0, NULL, &_allocFuncs, &_allocFuncs);

  *outputSize = outputSize64 + 13;
  if (lzmaStatus == SZ_OK) {
    // tricky: we have to generate the LZMA header
    // 5 bytes properties + 8 byte uncompressed size
    result = std::make_unique<unsigned char[]>(outputSize64 + 13);
    unsigned char *resultData = result.get();

    memcpy(resultData, propsEncoded, 5);
    for (int i = 0; i < 8; i++)
      resultData[5 + i] = (inputSize >> (i * 8)) & 0xFF;
    memcpy(resultData + 13, output.get(), outputSize64);
  }

  return result;
}

std::unique_ptr<unsigned char[]> lzma::Decompress(const unsigned char *input,
                                                  unsigned int inputSize,
                                                  size_t *outputSize) {
  if (inputSize < 13)
    return NULL; // invalid header!

  // extract the size from the header
  UInt64 size = 0;
  for (int i = 0; i < 8; i++)
    size |= (input[5 + i] << (i * 8));

  if (size <= (256 * 1024 * 1024)) {
    auto blob = std::make_unique<unsigned char[]>(size);

    ELzmaStatus lzmaStatus;
    SizeT procOutSize = size, procInSize = inputSize - 13;
    int status =
        LzmaDecode(blob.get(), &procOutSize, &input[13], &procInSize, input, 5,
                   LZMA_FINISH_END, &lzmaStatus, &_allocFuncs);

    if (status == SZ_OK && procOutSize == size) {
      *outputSize = size;
      return blob;
    }
  }

  return NULL;
}
