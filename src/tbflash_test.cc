#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

int main(int argc, char *argv[]) {
  std::vector<char *> args(argv, argv + argc);
  spdlog::info("Got argv: {}", args);
  std::this_thread::sleep_for(std::chrono::seconds(5));
  return 0;
}
