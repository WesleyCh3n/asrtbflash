#include "command.hpp"

int main(int argc, char *argv[]) {
  spdlog::set_level(spdlog::level::debug);
  auto child = process::Command::create("cmd")
                   .current_dir("../")
                   .arg("/c") //
                   .args({"dir", "a"})
                   .build()
                   ->spawn();
  auto output = process::wait_with_output(std::move(child.process_),
                                          std::move(child.pipes_));
  spdlog::info("status: {}", output.status);
  spdlog::info("stdout: {}", output.stdout_str);
  spdlog::info("stderr: {}", output.stderr_str);

  output = process::Command::create("cmd")
               .current_dir("../")
               .arg("/c") //
               .args({"dir"})
               .build()
               ->output();
  spdlog::info("status: {}", output.status);
  spdlog::info("stdout: {}", output.stdout_str);
  spdlog::info("stderr: {}", output.stderr_str);
  return 0;
}
