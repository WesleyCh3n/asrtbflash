#include "smbios.h"

#include "bufio.hpp"
#include "command.hpp"

#include <algorithm>
#include <iostream>

#include <cxxopts.hpp>

extern "C" {
#define MAX_STR_LEN 100

#pragma pack(push, 1)
struct header {
  uint8_t sign;
  uint8_t pos;
};
struct ModelName {
  uint8_t len;
  char str[MAX_STR_LEN];
};
struct ModelNames {
  uint8_t len;
  ModelName str[8];
};
struct TbflashInfo {
  uint8_t len;
  char flash_args[MAX_STR_LEN];
};
#pragma pack(pop)
}

namespace global {
std::string model_name;
std::string output_path;
std::string tool_path;
std::string firmware_path;
std::string device_id;
std::string vender_id;
} // namespace global

void parse_dev_args(int argc, char *argv[]) {
  cxxopts::Options options(argv[0], "ASRock thunderbolt flash tool");
  // clang-format off
  options.add_options()
    ("model", "motherboard model name", cxxopts::value<std::string>())
    ("output", "packed output file path", cxxopts::value<std::string>())
    ("t,tool", "thunderbolt flash tool path", cxxopts::value<std::string>())
    ("f,firmware", "thunderbolt firmware path", cxxopts::value<std::string>())
    ("d,device", "thunderbolt device id", cxxopts::value<std::string>()->default_value("0x1137"))
    ("v,vender", "thunderbolt vender id", cxxopts::value<std::string>()->default_value("0x8086"))
    ("h,help", "help message");
  // clang-format on
  options.parse_positional({"model", "output"});
  options.positional_help("<model name> <packed output file path>");
  options.show_positional_help();
  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    exit(0);
  }
  global::model_name = result["model"].as<std::string>();
  global::output_path = result["output"].as<std::string>();
  global::tool_path = result["tool"].as<std::string>();
  global::firmware_path = result["firmware"].as<std::string>();
  global::device_id = result["device"].as<std::string>();
  global::vender_id = result["vender"].as<std::string>();
}

int main(int argc, char *argv[]) {
#ifdef _DEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::debug); // TODO
#endif

  if (fs::path(argv[0]).stem().string() == "asrtbflash") {
    spdlog::debug("developer mode");
    try {
      parse_dev_args(argc, argv);
    } catch (const cxxopts::exceptions::exception &e) {
      std::cout << e.what() << std::endl;
      spdlog::error(e.what());
      exit(1);
    }

    ModelName model_name;
    assert(global::model_name.size() <= MAX_STR_LEN);
    model_name.len = static_cast<uint8_t>(global::model_name.size());
    global::model_name.copy(model_name.str, global::model_name.size());

    std::string args = fmt::format(
        "{} -d {} -v {}", fs::path(global::firmware_path).filename().string(),
        global::device_id, global::vender_id);
    assert(args.size() <= MAX_STR_LEN);
    spdlog::debug("args: {}, args size: {}", args, args.size());
    TbflashInfo tbflash_info;
    tbflash_info.len = static_cast<uint8_t>(args.size());
    memset(tbflash_info.flash_args, 0, MAX_STR_LEN);
    args.copy(tbflash_info.flash_args, args.size());

    try {
      bufio::Stacker::create(argv[0], global::output_path)
          .add_file(bufio::Flag{0xF0, 0xF5, 0xF3, 0xF0}, global::tool_path)
          .add_file(bufio::Flag{0xF1, 0xF0, 0xF2, 0xF4}, global::firmware_path)
          .add_buf(bufio::Flag{0xA0, 0xA5, 0xA3, 0xA0}, model_name)
          .add_buf(bufio::Flag{0xA1, 0xA0, 0xA2, 0xA4}, tbflash_info);
    } catch (const bufio::Exception &e) {
      spdlog::error(e.what());
      exit(1);
    }
  } else {
    spdlog::debug("Client mode");
    spdlog::info("Initilizing...");
    ModelName model_name;
    TbflashInfo tbflash_args;
    std::string work_dir = (fs::temp_directory_path() / "asrtbflash").string();
    spdlog::debug("extract path: {}", work_dir);
    auto destacker = bufio::DeStacker::create(argv[0], work_dir);
    // destacker.set_finish_cleanup();

    // 1. check model name valid
    destacker.extract_buf(bufio::Flag{0xA0, 0xA5, 0xA3, 0xA0}, model_name);
    const SMBIOS &SmBios = SMBIOS::getInstance();
    const wchar_t *board_product_name = SmBios.BoardProductName();
    char model_name_str[MAX_STR_LEN];
    sprintf_s(model_name_str, "%ws", board_product_name);
    auto mb_name = std::string(model_name_str);
    auto find_name = std::string(model_name.str, model_name.len);
    std::transform(mb_name.begin(), mb_name.end(), mb_name.begin(), ::toupper);
    std::transform(find_name.begin(), find_name.end(), find_name.begin(),
                   ::toupper);
    spdlog::debug("mb_name: {}, find_name: {}", mb_name, find_name);
    if (mb_name != find_name) {
      spdlog::error("Couldn't flash on the model {}", mb_name);
      exit(1);
    }

    // 2. extract all files
    destacker.extract_file(bufio::Flag{0xF0, 0xF5, 0xF3, 0xF0}); // tbflash
    destacker.extract_file(bufio::Flag{0xF1, 0xF0, 0xF2, 0xF4}); // firmware
    // 3. extract tbflash args
    destacker.extract_buf(bufio::Flag{0xA1, 0xA0, 0xA2, 0xA4}, tbflash_args);

    auto args = std::string(tbflash_args.flash_args, tbflash_args.len);
    spdlog::debug("model name: {}",
                  std::string(model_name.str, model_name.len));
    spdlog::debug("tblash args: {}, size: {}", args, args.size());
    try {
      spdlog::info("Start flashing...");
      auto output = process::Command::create("cmd")
                        .arg("/c")
                        .arg("tbflash.exe")
                        .arg(args)
                        .current_dir(work_dir)
                        .build()
                        ->output();
      if (output.status != 0) {
        spdlog::error("tbflash failed");
        spdlog::error("{}", output.stderr_str);
        exit(1);
      }
    } catch (const std::exception &e) {
      spdlog::error(e.what());
      exit(1);
    }
    spdlog::info("Finish");
  }
  return 0;
}
