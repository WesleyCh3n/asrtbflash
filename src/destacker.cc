#include "bufio.hpp"

using namespace bufio;

DeStacker::DeStacker(std::string self_path, std::string running_path) {
  spdlog::debug("self_path: {}", self_path);
  auto stm = std::ifstream(self_path, std::ios::binary);
  if (stm.fail()) {
    throw Exception("self file not found");
  }
  self_buf_ = get_buf(stm, self_len_);
  fs::create_directories(running_path);
  running_path_ = fs::absolute(fs::path(running_path));
}
std::unique_ptr<uint8_t[]> DeStacker::get_buf(std::ifstream &stm,
                                              size_t &size) {
  stm.seekg(0, std::ios::end);
  size = stm.tellg();
  stm.seekg(0, std::ios::beg);
  auto buf = std::make_unique<uint8_t[]>(size);
  stm.read((char *)buf.get(), size);
  return buf;
};
bool DeStacker::is_flag(uint8_t *ptr, const Flag &flag,
                        size_t current_position) {
  for (int i = 0; i < FLAG_LEN; i++) {
    if (ptr[i] != flag.value[i]) {
      return false;
    }
  }
  if (current_position != *(size_t *)&ptr[FLAG_LEN]) {
    return false;
  }
  return true;
}
bool DeStacker::get_buf_ptr(const Flag &flag, uint8_t *&out_ptr) {
  for (size_t i = 0; i < self_len_ - sizeof(Flag) - sizeof(size_t); i++) {
    if (is_flag(&self_buf_[i], flag, i)) {
      spdlog::debug("found flag {} at 0x{:X}", flag.value,
                    i + sizeof(Flag) + sizeof(size_t));
      out_ptr = &self_buf_[i + sizeof(Flag) + sizeof(size_t)];
      return true;
    }
  }
  return false;
}
DeStacker::~DeStacker() {
  spdlog::debug("destructor called");
  if (cleanup_) {
    spdlog::debug("cleaning up {}", running_path_.string());
    fs::remove_all(running_path_);
  }
};
DeStacker DeStacker::create(std::string self_path, std::string running_path) {
  return DeStacker(self_path, running_path);
}
void DeStacker::extract_file(const Flag f) {
  uint8_t *ptr = nullptr;
  if (!get_buf_ptr(f, ptr)) {
    throw Exception("not found");
  }
  size_t filename_len = ((size_t *)ptr)[0];
  size_t data_len = ((size_t *)ptr)[1];
  spdlog::debug("filename_len: {}, data_len: {}", filename_len, data_len);

  ptr = ptr + sizeof(size_t) * 2;
  std::string filename = std::string((char *)ptr, filename_len);
  spdlog::debug("filename {}", filename);

  ptr = ptr + filename_len;

  std::ofstream stm(running_path_ / filename, std::ios::binary);
  size_t real_size;
  auto real_buf = lzma::Decompress(ptr, (uint32_t)data_len, &real_size);
  stm.write(reinterpret_cast<char *>(real_buf.get()), real_size);
  stm.close();
}

void DeStacker::set_finish_cleanup() {
  spdlog::debug("set finish cleanup");
  cleanup_ = true;
}
