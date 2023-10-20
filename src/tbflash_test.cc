#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

int main(int argc, char *argv[]) {
  std::vector<char *> args(argv, argv + argc);
  spdlog::info("Got argv: {}", args);
  return 0;
}
