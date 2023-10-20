#include "bufio.hpp"
std::unique_ptr<uint8_t[]> bufio::Stacker::get_buf(std::ifstream &stm,
                                                   size_t &size) {
  stm.seekg(0, std::ios::end);
  size = stm.tellg();
  stm.seekg(0, std::ios::beg);
  auto buf = std::make_unique<uint8_t[]>(size);
  stm.read((char *)buf.get(), size);
  return buf;
};

std::unique_ptr<uint8_t[]> bufio::Stacker::get_compress_buf(std::ifstream &stm,
                                                            size_t &size) {
  size_t orig_size;
  auto buf = get_buf(stm, orig_size);
  auto compress_buf = lzma::Compress(buf.get(), orig_size, &size);
  return compress_buf;
}

bufio::Stacker::Stacker(std::string self_path, std::string out_path) {
  spdlog::debug("self_path: {}", self_path);
  auto self_stm = std::ifstream(self_path, std::ios::binary);
  out_stm_ = std::ofstream(out_path, std::ios::binary);
  if (self_stm.fail()) {
    throw Exception("self file not found");
  }
  if (out_stm_.fail()) {
    throw Exception("output file writing failed");
  }
  out_stm_ << self_stm.rdbuf();
  self_stm.close();
};

bufio::Stacker::~Stacker() { out_stm_.close(); }

bufio::Stacker &bufio::Stacker::add_file(const Flag f, std::string path) {
  std::ifstream stm(path, std::ios::binary);
  if (stm.fail()) {
    throw Exception("file not found");
  }

  std::string filename = fs::path(path).filename().string();
  spdlog::debug("add file path: {}", filename);

  size_t filename_size = filename.size();
  size_t buf_size;
  auto buf = get_compress_buf(stm, buf_size);
  spdlog::debug("flag: {}, flag size: {}", f.value, sizeof(Flag));
  spdlog::debug("filename: {}, filename size: {}", filename, filename_size);
  spdlog::debug("file size: {}", buf_size);

  size_t stm_pos = out_stm_.tellp();

  out_stm_.write(reinterpret_cast<const char *>(&f), sizeof(Flag));
  out_stm_.write(reinterpret_cast<char *>(&stm_pos), sizeof(size_t));

  out_stm_.write(reinterpret_cast<char *>(&filename_size), sizeof(size_t));
  out_stm_.write(reinterpret_cast<char *>(&buf_size), sizeof(size_t));

  out_stm_.write(filename.c_str(), filename.size());
  out_stm_.write(reinterpret_cast<char *>(buf.get()), buf_size);

  return *this;
}
