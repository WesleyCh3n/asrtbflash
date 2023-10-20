#pragma once
#include <exception>
#include <filesystem>
#include <fstream> // ifstream
#include <string>

#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

#include "lzma.h"

namespace fs = std::filesystem;

namespace bufio {

extern "C" {
#define FLAG_LEN 4
#pragma pack(push, 1)
struct Flag {
  uint8_t value[FLAG_LEN];
};
#pragma pack(pop)
}

class Exception : std::exception {
  std::string msg_;

public:
  Exception(const std::string &msg) : msg_(msg) {}
  char const *what() const noexcept override { return msg_.c_str(); }
};

class Stacker {
  std::ofstream out_stm_;
  std::unique_ptr<uint8_t[]> get_buf(std::ifstream &stm, size_t &size);

  std::unique_ptr<uint8_t[]> get_compress_buf(std::ifstream &stm, size_t &size);

public:
  Stacker() = delete;
  Stacker(std::string self_path, std::string out_path);
  ~Stacker();

  static Stacker create(std::string self_path, std::string out_path) {
    return Stacker(self_path, out_path);
  };

  /**
   * @param flag
   * @param header
   * @param path
   * @return
   *
   * [flag][flag position]
   * [filename size][data size]
   * [filename][data]
   */
  Stacker &add_file(const Flag f, std::string path);

  /**
   * @param flag
   * @param header
   * @return
   *
   * [flag][flag position]
   * [data]
   */
  template <typename T> Stacker &add_buf(const Flag f, const T &data) {
    size_t stm_pos = out_stm_.tellp();
    out_stm_.write(reinterpret_cast<const char *>(&f), sizeof(Flag));
    out_stm_.write(reinterpret_cast<char *>(&stm_pos), sizeof(size_t));
    out_stm_.write(reinterpret_cast<const char *>(&data), sizeof(T));
    return *this;
  }
};

class DeStacker {
  std::unique_ptr<uint8_t[]> self_buf_;
  size_t self_len_;
  fs::path running_path_;
  bool cleanup_ = false;
  DeStacker() = delete;
  DeStacker(std::string self_path, std::string running_path);

  std::unique_ptr<uint8_t[]> get_buf(std::ifstream &stm, size_t &size);
  bool is_flag(uint8_t *ptr, const Flag &flag, size_t current_position);

  bool get_buf_ptr(const Flag &flag, uint8_t *&out_ptr);

public:
  ~DeStacker();
  DeStacker(DeStacker &other) = delete; // copy constructor

  static DeStacker create(std::string self_path, std::string running_path);

  void extract_file(const Flag f);

  template <typename T> void extract_buf(const Flag f, T &header) {
    uint8_t *ptr = nullptr;
    if (!get_buf_ptr(f, ptr)) {
      throw Exception("not found");
    }
    header = *(T *)ptr;
  };

  void set_finish_cleanup();
};

} // namespace bufio
