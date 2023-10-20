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
  ModelName models[10];
};
struct TbflashInfo {
  uint8_t len;
  char flash_args[MAX_STR_LEN];
};
#pragma pack(pop)
}

namespace global {
std::vector<std::string> model_names;
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
    ("output", "packed output file path", cxxopts::value<std::string>())
    ("m,model", "motherboard model name", cxxopts::value<std::vector<std::string>>())
    ("t,tool", "thunderbolt flash tool path", cxxopts::value<std::string>())
    ("f,firmware", "thunderbolt firmware path", cxxopts::value<std::string>())
    ("d,device", "thunderbolt device id", cxxopts::value<std::string>()->default_value("0x1137"))
    ("v,vender", "thunderbolt vender id", cxxopts::value<std::string>()->default_value("0x8086"))
    ("h,help", "help message");
  // clang-format on
  options.parse_positional({"output"});
  options.positional_help("<packed output file path>");
  options.show_positional_help();
  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    exit(0);
  }
  global::model_names = result["model"].as<std::vector<std::string>>();
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
#endif

  if (fs::path(argv[0]).stem().string() == "asrtbflash") {
    spdlog::set_level(spdlog::level::debug); // TODO
    spdlog::debug("Developer mode");
    try {
      parse_dev_args(argc, argv);
    } catch (const cxxopts::exceptions::exception &e) {
      std::cout << e.what() << std::endl;
      spdlog::error(e.what());
      exit(1);
    }

    ModelNames model_names;
    memset(&model_names, 0, sizeof(model_names));
    model_names.len = static_cast<uint8_t>(global::model_names.size());
    for (int i = 0; i < global::model_names.size(); i++) {
      model_names.models[i].len =
          static_cast<uint8_t>(global::model_names[i].size());
      global::model_names[i].copy(model_names.models[i].str,
                                  global::model_names[i].size());
    }

    std::string args = fmt::format(
        "{} -d {} -v {}", fs::path(global::firmware_path).filename().string(),
        global::device_id, global::vender_id);
    assert(args.size() <= MAX_STR_LEN);
    TbflashInfo tbflash_info;
    tbflash_info.len = static_cast<uint8_t>(args.size());
    memset(tbflash_info.flash_args, 0, MAX_STR_LEN);
    args.copy(tbflash_info.flash_args, args.size());

    try {
      bufio::Stacker::create(argv[0], global::output_path)
          .add_file(bufio::Flag{0xF0, 0xF5, 0xF3, 0xF0}, global::tool_path)
          .add_file(bufio::Flag{0xF1, 0xF0, 0xF2, 0xF4}, global::firmware_path)
          .add_buf(bufio::Flag{0xA0, 0xA5, 0xA3, 0xA0}, model_names)
          .add_buf(bufio::Flag{0xA1, 0xA0, 0xA2, 0xA4}, tbflash_info);
      spdlog::debug("Tbflash args: {}, args size: {}", args, args.size());
      spdlog::debug("ModelNames: {}", global::model_names);
    } catch (const bufio::Exception &e) {
      spdlog::error(e.what());
      exit(1);
    }
  } else {
    cxxopts::Options options("tbflash", "tbflash");
    options.add_options()("D,debug", "Debug",
                          cxxopts::value<bool>()->default_value("false"));
    auto result = options.parse(argc, argv);
    if (result.count("debug")) {
      spdlog::set_level(spdlog::level::debug);
    } else {
      spdlog::set_level(spdlog::level::info);
    }

    spdlog::debug("Client mode");
    spdlog::info("Initilizing...");
    ModelNames model_names;
    TbflashInfo tbflash_args;
    std::string work_dir = (fs::temp_directory_path() / "asrtbflash").string();
    auto destacker = bufio::DeStacker::create(argv[0], work_dir);
    // destacker.set_finish_cleanup();

    // 1. check model name valid
    destacker.extract_buf(bufio::Flag{0xA0, 0xA5, 0xA3, 0xA0}, model_names);
    const SMBIOS &SmBios = SMBIOS::getInstance();
    const wchar_t *board_product_name = SmBios.BoardProductName();
    char model_name_str[MAX_STR_LEN];
    sprintf_s(model_name_str, "%ws", board_product_name);
    auto mb_name = std::string(model_name_str);
    std::transform(mb_name.begin(), mb_name.end(), mb_name.begin(), ::toupper);

    bool is_found = false;
    for (int i = 0; i < model_names.len; i++) {
      auto find_name =
          std::string(model_names.models[i].str, model_names.models[i].len);
      std::transform(find_name.begin(), find_name.end(), find_name.begin(),
                     ::toupper);
      spdlog::debug("Available model: {}", find_name);

      if (mb_name == find_name) {
        is_found = true;
        spdlog::info("Requirement passed...");
        break;
      }
    }
    if (!is_found) {
      spdlog::error("Couldn't flash on the model {}", mb_name);
      fmt::println("Enter to exit...");
      std::cin.get();
      exit(1);
    }

    // 2. extract all files
    destacker.extract_file(bufio::Flag{0xF0, 0xF5, 0xF3, 0xF0}); // tbflash
    destacker.extract_file(bufio::Flag{0xF1, 0xF0, 0xF2, 0xF4}); // firmware
    // 3. extract tbflash args
    destacker.extract_buf(bufio::Flag{0xA1, 0xA0, 0xA2, 0xA4}, tbflash_args);

    auto args = std::string(tbflash_args.flash_args, tbflash_args.len);
    spdlog::debug("Flashing args: {}", args);
    try {
      spdlog::info("Start flashing...");
      std::atomic_bool done(false);
      std::atomic_bool success(true);
      std::thread t([&]() {
        auto output = process::Command::create("cmd")
                          .arg("/c")
                          .arg("tbflash.exe")
                          .arg(args)
                          .current_dir(work_dir)
                          .build()
                          ->output();
        if (output.status != 0) {
          success.store(false);
          spdlog::error("tbflash failed");
          spdlog::error("{}", output.stderr_str);
          done.store(true);
        }
        done.store(true);
      });
      const char c[4] = {'\\', '|', '/', '-'};
      int i = 0;
      while (!done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        fmt::print("Please wait...{}\r", c[i]);
        i = (i + 1) % 4;
      }
      t.join();
      success.load() ? spdlog::info("Success") : spdlog::error("Failed");
      fmt::println("Enter to exit...");
      std::cin.get();
    } catch (const std::exception &e) {
      spdlog::error(e.what());
      fmt::println("Enter to exit...");
      std::cin.get();
      exit(1);
    }
  }
  return 0;
}
