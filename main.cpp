#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>

int main(int argc, char* argv[]) {
    if (argc != 3) { // get the source and it's input 
        std::cerr << "Usage: " << argv[0] << " <cpp_source_file> <input_text_file>" << std::endl;
        return 1;
    }

    const char* cpp_file = argv[1];
    const char* input_file = argv[2];
    const char* output_exe = "output_program";

    std::string compile_command = "g++ " + std::string(cpp_file) + " -o " + output_exe;
    if (system(compile_command.c_str()) != 0) {
        std::cerr << "Compilation failed" << std::endl;
        return 1;
    }

    // Read the input text file
    std::ifstream input(input_file);
    if (!input) {
        std::cerr << "Failed to open input file" << std::endl;
        return 1;
    }
    std::stringstream input_buffer;
    input_buffer << input.rdbuf();
    std::string input_content = input_buffer.str();

    // Run the compiled executable with input from the text file
    std::string run_command = std::string("./") + output_exe;
    FILE* pipe = popen(run_command.c_str(), "w");
    if (!pipe) {
        std::cerr << "Failed to run the compiled program" << std::endl;
        return 1;
    }

    fwrite(input_content.c_str(), 1, input_content.size(), pipe);
    pclose(pipe);

    // Capture and print the output of the compiled program
    pipe = popen(run_command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to run the compiled program" << std::endl;
        return 1;
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::cout << buffer;
    }
    pclose(pipe);

    return 0;
}

