#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

int feed(std ::string input_file);
int compile(std ::string file_name);

static std::string output_exe = "output_program";
int main(int argc, char *argv[]) {
  if (argc != 3) { // get the source and it's input
    std::cerr << "Usage: " << argv[0] << " <cpp_source_file> <input_text_file>"
              << std::endl;
    return 1;
  }

  const char *cpp_file = argv[1];
  const char *input_file = argv[2];

  compile(cpp_file);
  feed(input_file);
  return 0;
}

int compile(std ::string file_name) {

  std::string compile_command = "g++ " + file_name + " -o " + output_exe;
  if (system(compile_command.c_str()) != 0) {
    std::cerr << "Compilation failed" << std::endl;
    return -1;
  }
  return 1;
}
int feed(std ::string input_file) {
  // Read the input text file
  std::ifstream input(input_file);
  if (!input) {
    std::cerr << "Failed to open input file" << std::endl;
    return -1;
  }
  std::stringstream input_buffer;
  input_buffer << input.rdbuf();
  std::string input_content = input_buffer.str();

  // Run the compiled executable with input from the text file
  std::string run_command = std::string("./") + output_exe;
  FILE *pipe = popen(run_command.c_str(), "w");
  if (!pipe) {
    std::cerr << "Failed to run the compiled program" << std::endl;
    return -1;
  }

  fwrite(input_content.c_str(), 1, input_content.size(), pipe);
  pclose(pipe);
  return 1;
}
