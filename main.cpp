#include "json.hpp"
#include "program.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <gumbo.h>
#include <ios>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <vector>

#define CONFIG_FILE ".psioConfig.json"
namespace fs = std::filesystem;
enum Mode { Create, Exists, Setup, Execution, Clear };

int setup();
int feed(std ::string input_file);
int compile(std ::string file_name); // feeds input to program
std::string fetchHTML(const std::string &url);
void parse_HTML(const std::string &html, std::vector<std::string> &inputs,
                std::vector<std::string> &outputs,
                std::vector<std::string> &title);
void searchForTestCases(
    GumboNode *node, std::vector<std::string> &inputs,
    std::vector<std::string> &outputs);     // search for test cases
bool exists(std ::string file_name);        // check if file exists
void check_json_config();                   // check if config file exists
inline void text_in_red(std::string s);     // cout text in red
inline void text_in_green(std::string s);   // text in green for info
std::string setup_problem(std::string url); // setup problem and json
std ::string get_temp();                    // gets the template file
void output_to_json(program *problem);
bool is_empty(const std::string &filename); // check if file is empty
bool exists_in_json(const std::string &problem_name, nlohmann::json &jsonArray);
inline std::string remove_spaces(std::string s);
inline bool is_valid_link(const std::string &link);
Mode chose_from_json(std::string title);
bool find_problem_by_title(const std::string &title,
                           const nlohmann::json &jsonArray,
                           nlohmann::json &result);

std::fstream config_file(CONFIG_FILE, std::ios::app);
program *problem = new program;

int main(int argc, char *argv[]) {
  Mode mode = Setup;
  while (true) {
    switch (mode) {
    case Setup: {
      mode = (Mode)setup();
      break;
    }
    case Create: {
      text_in_green("Entering Create mode\n");
      std::cout << "please enter the problem url" << std::endl;
      std::cin >> problem->url;
      if (!is_valid_link(problem->url)) { // check if valid link (problem->url){
        text_in_red(problem->url + " is an invalid url\n");
        mode = Clear;
        break;
      }
      std::string title = setup_problem(problem->url);
      if (title == "") {
        text_in_red("could not setup problem\n");
        mode = Clear;
        break;
      }
      title = remove_spaces(title);
      problem->file_name = title + ".cpp";
      problem->input_file = title + ".In";
      problem->output_file = title + ".Out";
      output_to_json(problem);
      text_in_green("Files setup complete\n");
      mode = Execution;
      break;
    }
    case Exists: {
      text_in_green("Entering Exists mode\n");
      std::cout << "please enter the file name" << std::endl;
      std::string title;
      mode = chose_from_json(title);
      break;
    }
    case Execution: {
      text_in_green("Entering Execution mode\n");
      return 1;
    }
    case Clear: { // clear everything about the problem
      text_in_red("resetting beacuse of a fault\n");
      delete problem;
      problem = new program();
      mode = Setup;
      break;
    }
    default: {
      break;
    }
    }
  }
}
bool is_empty(const std::string &filename) { // check if file is empty
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    text_in_red("Error opening file: " + filename);
    return false;
  }
  return file.tellg() == 0;
}

bool exists(std ::string file_name) { return fs::exists(file_name); }
int setup() {
  text_in_green("psio test runner\n");

  while (true) {
    std::cout << "please enter the mode you want to run" << std::endl;
    std::cout << "please enter 0 for create" << std::endl
              << "please enter 1 for exists" << std::endl;

    int x = -1;

    if (std::cin >> x) {
      if (x == 0 || x == 1) {
        return x;
      } else {
        text_in_red("please enter a valid number\n");
      }
    } else {
      // Clear the error flag on cin
      std::cin.clear();
      // Ignore the invalid input
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      text_in_red("not a number\n");
      return Mode::Setup; // clear mode
    }
  }
}

inline bool is_valid_link(const std::string &link) {
  return (!(link.size() >= 65) ||
          link.find("http://codeforces.com/problemset/problem") !=
              std::string::npos ||
          link.find("https://codeforces.com/gym/") != std::string::npos ||
          link.find("https://codeforces.com/contest/") != std::string::npos);
}
int compile(std ::string file_name, std::string output_exe) {
  std::string compile_command =
      "g++ " + file_name + " -o " + output_exe; // compile the file
  if (system(compile_command.c_str()) != 0) {
    text_in_red("Compilation failed\n");
    return -1;
  }
  return 1;
}
int feed(std ::string input_file, std::string output_exe) {
  int fd;
  fpos_t pos;
  fflush(stdout);
  fgetpos(stdout, &pos);
  fd = dup(fileno(stdout));
  freopen("out.out", "w", stdout);
  // Read the input text file
  std::ifstream input(input_file);
  if (!input) {
    text_in_red("Failed to open input file\n");
    return -1;
  }
  std::stringstream input_buffer;
  input_buffer << input.rdbuf();
  std::string input_content = input_buffer.str();

  // Run the compiled executable with input from the text file
  std::string run_command = std::string("./") + output_exe;
  FILE *pipe = popen(run_command.c_str(), "w");
  if (!pipe) {
    text_in_red("Failed to run the compiled program\n");
    return -1;
  }

  fwrite(input_content.c_str(), 1, input_content.size(), pipe);
  pclose(pipe);
  // Restore stdout to its original destination
  fflush(stdout);
  dup2(fd, fileno(stdout));
  close(fd);
  clearerr(stdout);
  fsetpos(stdout, &pos);
  return 1;
}

size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                     std::string *s) {
  size_t newLength = size * nmemb;
  size_t oldLength = s->size();
  try {
    s->resize(oldLength + newLength);
  } catch (std::bad_alloc &e) {
    // handle memory problem
    return 0;
  }

  std::copy((char *)contents, (char *)contents + newLength,
            s->begin() + oldLength);
  return size * nmemb;
}

std::string fetchHTML(const std::string &url) {
  CURL *curl;
  CURLcode res;
  std::string htmlContent;

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // setting curl url
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                     WriteCallback); // setting write function
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                     &htmlContent); // setting write data
    res = curl_easy_perform(curl);  // getting response
    curl_easy_cleanup(curl);        // cleaning up
  }
  return htmlContent;
}
void extract_text_from_node(GumboNode *node,
                            std::vector<std::string> &results) {
  if (node->type == GUMBO_NODE_TEXT) {
    results.push_back(node->v.text.text);
  } else if (node->type == GUMBO_NODE_ELEMENT ||
             node->type == GUMBO_NODE_DOCUMENT) {
    GumboAttribute *classAttr =
        gumbo_get_attribute(&node->v.element.attributes, "class");
    if (classAttr && std::string(classAttr->value) == "title") {
      // Skip this node and its children
      if (node->parent) {
        GumboAttribute *parentClassAttr =
            gumbo_get_attribute(&node->parent->v.element.attributes, "class");
        if (parentClassAttr &&
            std::string(parentClassAttr->value) != "header") {
          // Skip this node and its children
          return;
        }
      }
    }
    const GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
      extract_text_from_node(static_cast<GumboNode *>(children->data[i]),
                             results);
    }
  }
}

// Function to search for the div with class "philo" and extract all text
void search_for_text(GumboNode *node, std::vector<std::string> &results,
                     std::string tag_name) {
  if (node->type != GUMBO_NODE_ELEMENT)
    return;
  GumboAttribute *classAttr;
  if (node->v.element.tag == GUMBO_TAG_DIV &&
      (classAttr = gumbo_get_attribute(&node->v.element.attributes, "class")) &&
      std::string(classAttr->value) == tag_name) {

    // Extract text from this node and all its children
    extract_text_from_node(node, results);
    return; // No need to continue searching, assuming only one "philo" div is
            // targeted
  }

  const GumboVector *children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; ++i) {
    search_for_text(static_cast<GumboNode *>(children->data[i]), results,
                    tag_name);
  }
}

void parse_HTML(const std::string &html, std::vector<std::string> &inputs,
                std::vector<std::string> &outputs,
                std::vector<std::string> &title) {
  GumboOutput *output = gumbo_parse(html.c_str());
  search_for_text(output->root, title, "title");
  search_for_text(output->root, inputs, "input");
  search_for_text(output->root, outputs, "output");
}
std::string setup_problem(std::string url) {
  std::vector<std::string> inputs, outputs, title;
  std::string html = fetchHTML(url);
  parse_HTML(html, inputs, outputs, title);
  if (title.size() != 0 && inputs.size() != 0 &&
      outputs.size() !=
          0) { // if we got the inputs and outputs and the title correctly
    std::ofstream in(title[0] + ".In");
    std::ofstream out(title[0] + ".Out");

    for (auto i : inputs) {
      in << i << "\npsio ----\n";
    };
    for (auto o : outputs) {
      out << o << "\npsio ----\n";
    };
    std::string source = get_temp();             // get the template file
    std::string destination = title[0] + ".cpp"; // creating the source file
    try {
      fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
      text_in_green("Template copied successfully.\n");
    } catch (fs::filesystem_error &e) {
      text_in_red("Error copying template: " + std::string(e.what()) + '\n');
    }
    return title[0];
    ;
  }
  return "";
}
std ::string get_temp() {
  std::string global_config =
      std::string(std::getenv("HOME")) +
      "/.config/psio.temp"; // the global config file of psio
  if (!exists(global_config)) {
    std::string temp_file;
    text_in_red("please provide a template file\n");
    std::cin >> temp_file;
    std::fstream temp(global_config, std::ios::app);
    if (temp.is_open()) {
      temp << temp_file;
    }
    temp.close();
    return temp_file;
  } else {
    std::ifstream temp(global_config);
    std::string temp_file;
    if (temp.is_open()) {
      if (std::getline(temp, temp_file)) {
        return temp_file;
      } else {
        text_in_red("Error reading temp_file\n");
      }
      temp.close();
    } else {
      text_in_red("Error opening file.\n");
    }
    return temp_file;
  }
}

void output_to_json(program *problem) { // output problem to json
  nlohmann::json array_of_problems;     // array of problems in the file
  std::ifstream jsonFile(CONFIG_FILE);  // get the problems config file
  nlohmann::json problem_json;          // create json object for the problem
  problem_json["title"] = problem->file_name;
  problem_json["input"] = problem->input_file;
  problem_json["output"] = problem->output_file;
  problem_json["url"] = problem->url;
  if (!is_empty(CONFIG_FILE)) { // if there are problems
    if (config_file.is_open()) {
      array_of_problems = nlohmann::json::parse(jsonFile);
    } else {
      text_in_red("Error opening file for reading\n");
      return;
    }
    if (exists_in_json(problem->file_name, array_of_problems)) {
      text_in_red("Problem already exists in JSON. Not adding.\n");
      // todo add validation
      return;
    } else {
      array_of_problems.push_back(problem_json);
      std::ofstream json_out(
          CONFIG_FILE); // overwrite existing json and add updated version
      json_out << array_of_problems.dump(4);
      json_out.close();
    }
  } else {
    std::ofstream json_out(CONFIG_FILE);
    if (config_file.is_open()) {
      array_of_problems.push_back(problem_json);
      json_out << array_of_problems.dump(4);
      json_out.close();
    } else {
      text_in_red("Error opening file for writing\n");
    }
  }
}
bool exists_in_json(const std::string &problem_name,
                    nlohmann::json &jsonArray) {
  for (const auto &item : jsonArray) {
    if (item.contains("title") && item["title"] == problem_name) {
      return true;
    }
  }
  return false;
}
Mode chose_from_json(std::string title) {
  std::ifstream config_file(CONFIG_FILE);
  nlohmann::json jsonArray;

  if (config_file.is_open()) {
    try {
      config_file >> jsonArray;
      config_file.close();
    } catch (const nlohmann::json::parse_error &e) {
      text_in_red("Parse error: " + std::string(e.what()));
      config_file.close();
      text_in_red("please enter create mode to fix");
      return Clear;
    }
  } else {
    text_in_red("Error opening config file for reading");
    // when there is no file
    return Clear;
  }
  nlohmann::json result;
  if (find_problem_by_title(title, jsonArray, result)) {
    if (result["title"] == "" || result["input"] == "" ||
        result["output"] == "") {
      if (is_valid_link(result["url"])) {
        text_in_red("invalid data in json");
        text_in_green("uerl is correct\n");
        setup_problem(result["url"]);
      } else {
        text_in_red("invalid data and  url in json\n");
        text_in_red("please enter create mode to fix");
        return Clear;
      }
    }

    problem->file_name = result["title"];
    problem->input_file = result["input"];
    problem->output_file = result["output"];
    if (!exists(problem->input_file)||!exists(problem->output_file)||!exists(problem->file_name)) {
	text_in_red("your files are missing\n");
	text_in_green("setting up files\n");
        setup_problem(problem->url);
	text_in_green("files are ready\n");
    }
    text_in_green("Problem: " + title + "is ready to be solved" + "\n");
    return Execution;
  } else {
    text_in_red("Problem not found.\n");
    return Clear;
  }
}
bool find_problem_by_title(const std::string &title,
                           const nlohmann::json &jsonArray,
                           nlohmann::json &result) {
  for (const auto &item : jsonArray) {
    if (item.contains("title") && item["title"] == title) {
      result = item;
      return true;
    }
  }
  return false;
}
inline void text_in_red(std::string s) {
  std::cerr << "\033[1;31m";
  std::cerr << s;
  std::cerr << "\033[0m";
}
inline void text_in_green(std::string s) {
  std::cout << "\033[1;32m";
  std::cout << s;
  std::cout << "\033[0m";
}
inline std::string remove_spaces(std::string s) {
  std::string result;
  for (auto &c : s) {
    if (c != ' ') {
      result.push_back(c);
    }
  }
  return result;
}
