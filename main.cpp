#include "json.hpp"
#include "program.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <future>
#include <gumbo.h>
#include <ios>
#include <iostream>
#include <regex>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

#define CONFIG_FILE ".psioConfig.json"
namespace fs = std::filesystem;
enum Mode { Create, Exists, Setup, Execution, Clear, Repair };

int setup();
int feed(std ::string input_file);
bool run_with_timeout(const std::string &input_file,
                      const std::string &output_exe, int timeout_seconds);
int compile(std ::string file_name,
            std::string output_exe); // feeds input to program
std::string fetchHTML(const std::string &url);
void parse_HTML(const std::string &html, std::vector<std::string> &inputs,
                std::vector<std::string> &outputs,
                std::vector<std::string> &title);
void searchForTestCases(
    GumboNode *node, std::vector<std::string> &inputs,
    std::vector<std::string> &outputs);     // search for test cases
bool exists(std ::string file_name);        // check if file exists
inline void text_in_red(std::string s);     // cout text in red for errors
inline void text_in_green(std::string s);   // text in green for info
inline void text_in_purple(std::string s);  // text in purple for info
std::string setup_problem(std::string url); // setup problem and json
std ::string get_temp();                    // gets the template file
bool output_to_json(program *problem);
bool is_empty(const std::string &filename); // check if file is empty
bool exists_in_json(const nlohmann::json &problem_name,
                    nlohmann::json &jsonArray);
inline std::string remove_spaces(std::string s);
inline bool is_valid_link(const std::string &link);
Mode chose_from_json(std::string title);
bool find_problem_by_title(const std::string &title,
                           const nlohmann::json &jsonArray,
                           nlohmann::json &result);

std::vector<std::string> collect_inputs_from_file(std::string input_file);
std::fstream config_file(CONFIG_FILE, std::ios::app);
std::pair<int, int> check_output(std::string output_file);
Mode create_problem(std::string);

program *problem = new program;
bool coruppted_json = false;

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
      if (!is_valid_link(problem->url)) { // check if valid link (problem->url):
        text_in_red(problem->url + " is an invalid url\n");
        mode = Clear;
        break;
      } else {
        mode = create_problem(problem->url);
      }
      break;
    }
    case Exists: {
      text_in_green("Entering Exists mode\n");
      std::cout << "please enter the file name" << std::endl;
      std::string title;
      std::cin >> title;
      mode = chose_from_json(title);
      break;
    }
    case Execution: {
      bool check = true;
      text_in_green("Entering Execution mode\n");
      // additional check
      if (!exists(problem->input_file) || !exists(problem->output_file) ||
          !exists(problem->file_name)) {
        text_in_red("One or more files are missing\n");
        mode = Clear;
        break;
      } else {
        if (compile(problem->file_name, "psio.out")) {
          text_in_green("Compiled successfully\n");
          if (run_with_timeout(problem->input_file, "psio.out", 5)) {
            std::pair<int, int> result = check_output(problem->output_file);
            text_in_purple("Passed: " + std::to_string(result.first) +
                           " out of " + std::to_string(result.second) + "\n");
          }

        } else {
          text_in_red("Error compiling\n");
        }
      }

      text_in_purple("Do you want to run again?\n");
      while (check) {
        std::cout << "please enter 0 for retest" << std::endl;
        std::cout << "please enter 1 for create" << std::endl
                  << "please enter 2 for exists" << std::endl;
        int x = -1;

        if (std::cin >> x) {
          switch (x) {
          case 0: {
            check = false;
            break;
          }
          case 1: {
            mode = Create;
            check = false;
            break;
          }
          case 2: {
            mode = Exists;
            check = false;
            break;
          }
          default: {
            text_in_red("please enter a valid number\n");
            break;
          }
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

      break;
    }

    case Clear: { // clear everything about the problem
      std::fstream config_file(CONFIG_FILE,
                               std::ios::app); // for if it's deleted
      config_file.close();
      if (coruppted_json) {
        mode = Repair;
        coruppted_json = false;
        break;
      }
      text_in_red("resetting beacuse of a fault or choosing a new problem\n");
      sleep(5);
      system("clear");
      delete problem;
      problem = new program();

      mode = Setup;
      break;
    }
    case Repair: {
      text_in_green("Repairing\n");
      std::ifstream json_in(CONFIG_FILE);
      std::string json_string = "";
      std::vector<std::string> urls;
      text_in_green("getting urls\n");
      for (std::string line; std::getline(json_in, line);) {
        json_string += line;
        std::regex url_pattern(R"((https?://[^\s"]+))");
        std::smatch match;
        if (std::regex_search(line, match, url_pattern)) {
          urls.push_back(match.str(0)); // Return the first matched URL
        }
      }
      json_in.close();
      std::ofstream json_out(
          CONFIG_FILE); // to replace with repaired and delet old one
      for (const auto url : urls) {
        if (create_problem(url) == Execution) // the problem is repaired
          text_in_green("Repaired " + problem->file_name + "\n");
      }
      text_in_green("Done Repairing\n");
      mode = Clear;
      coruppted_json = false;
      break;
    }
    default: {
      break;
    }
    }
  }
}
bool is_empty(const std::string &filename) { // check if file is empty
  std::ifstream file(filename, std::ifstream::ate);
  if (!file.is_open()) {
    text_in_red("Error opening file: " + filename + "\n");
    file.close();
    return false;
  }
  return file.tellg() == 0;
}

bool exists(std ::string file_name) { return fs::exists(file_name); }
int setup() {
  text_in_purple("psio test runner\n");

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
  text_in_green("compiling\n");
  std::string compile_command =
      "g++ " + file_name + " -o " + output_exe; // compile the file
  if (system(compile_command.c_str()) != 0) {
    text_in_red("Compilation failed\n");
    return 0;
  }
  return 1;
}
int feed(std ::string input_file, std::string output_exe) {
  int fd;
  fpos_t pos;
  fflush(stdout);
  fgetpos(stdout, &pos);
  fd = dup(fileno(stdout));
  freopen("psio.output", "w", stdout);
  // Read the input text file
  auto inputs = collect_inputs_from_file(input_file);
  // Run the compiled executable with input from the text file
  //

  for (auto input_content : inputs) { // feed the input
    std::string run_command = std::string("./") + output_exe;
    FILE *pipe = popen(run_command.c_str(), "w");
    if (!pipe) {
      text_in_red("Failed to run the compiled program\n");
      return -1;
    }

    fwrite(input_content.c_str(), 1, input_content.size(), pipe);
    pclose(pipe);
    std::cout << "\npsio---\n";
    fflush(stdout);
  }
  // Restore stdout to its original destination
  dup2(fd, fileno(stdout));
  close(fd);
  clearerr(stdout);
  fsetpos(stdout, &pos);
  return 1;
}
// Function to run `feed` in a separate thread and enforce a timeout
bool run_with_timeout(const std::string &input_file,
                      const std::string &output_exe, int timeout_seconds) {
  std::packaged_task<int()> task(
      [input_file, output_exe]() { return feed(input_file, output_exe); });
  // asynic function

  std::future<int> result = task.get_future();
  std::thread(std::move(task)).detach();

  if (result.wait_for(std::chrono::seconds(timeout_seconds)) ==
      std::future_status::timeout) {
    text_in_red("Function execution exceeded timeout of : ");
    std::cerr << timeout_seconds;
    text_in_red(" seconds.\n");
    return false;
  }

  int exit_code = result.get();
  return exit_code == 1;
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

// Function to search for the div with class  and extract all text
void search_for_text(GumboNode *node, std::vector<std::string> &results,
                     std::string tag_name) {
  if (node->type != GUMBO_NODE_ELEMENT)
    return;
  GumboAttribute *classAttr;
  if (node->v.element.tag == GUMBO_TAG_DIV &&
      (classAttr = gumbo_get_attribute(&node->v.element.attributes, "class")) &&
      std::string(classAttr->value) == tag_name) {

    // Extract text from this node and all its children
    if (results.size() != 0) {
      results.push_back("psio---");
    }
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
    std::ofstream in(remove_spaces(title[0] + ".In"));
    std::ofstream out(remove_spaces(title[0] + ".Out"));

    for (auto i : inputs) {
      in << i << "\n";
    };
    for (auto o : outputs) {
      out << o << "\n";
    };
    std::string source = get_temp(); // get the template file
    std::string destination =
        remove_spaces(title[0]) + ".cpp"; // creating the source file
    try {
      fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
      text_in_green("Template copied successfully.\n");
    } catch (fs::filesystem_error &e) {
      text_in_red("Error copying template: " + std::string(e.what()) + '\n');
      text_in_red("making an empty file\n");
      // making an empty file
      std::ofstream empty(destination);
      empty.close();
    }
    text_in_green(title[0] + " is setuped successfully.\n");
    return title[0];
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

bool output_to_json(program *problem) { // output problem to json
  nlohmann::json array_of_problems;     // array of problems in the file
  std::ifstream jsonFile(CONFIG_FILE);  // get the problems config file
  nlohmann::json problem_json;          // create json object for the problem
  problem_json["title"] = problem->file_name;
  problem_json["input"] = problem->input_file;
  problem_json["output"] = problem->output_file;
  problem_json["url"] = problem->url;
  if (!is_empty(CONFIG_FILE) && exists(CONFIG_FILE)) { // if there are problems
    if (config_file.is_open()) {
      try {
        array_of_problems = nlohmann::json::parse(jsonFile);
      } catch (nlohmann::json::parse_error &e) {
        text_in_red("Parse error: " + std::string(e.what()) + "\n");
        coruppted_json = true;
        return false;
      }
    } else {
      text_in_red("Error opening file for reading\n");
      return false;
    }
    if (exists_in_json(problem_json, array_of_problems)) {
      text_in_red(
          "Problem already exists in JSON. Not adding. maybe fixing \n");
      return true;
    } else {
      text_in_green("Adding new problem to JSON\n");
      array_of_problems.push_back(problem_json);
      std::ofstream json_out(
          CONFIG_FILE); // overwrite existing json and add updated version
      json_out << array_of_problems.dump(4);
      json_out.close();
      return true;
    }
  } else {
    std::ofstream json_out(CONFIG_FILE);
    if (config_file.is_open()) {
      text_in_green("Creating new json file\n");
      array_of_problems.push_back(problem_json);
      json_out << array_of_problems.dump(4);
      json_out.close();
      return true;
    } else {
      text_in_red("Error opening file for writing\n");
      return false;
    }
  }
}
bool exists_in_json(const nlohmann::json &problem,
                    nlohmann::json &array_of_problems) {
  for (auto &item : array_of_problems) {
    if (item.contains("title") && item["title"] == problem["title"]) {
      item.update(problem); // resetting maybe some thing is missing
      std::ofstream json_out(
          CONFIG_FILE); // overwrite existing json and add updated version
      json_out << array_of_problems.dump(4);
      json_out.close();
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
      text_in_red("Parse error: " + std::string(e.what()) + "\n");
      coruppted_json = true;
      config_file.close();
      text_in_red("please enter create mode to fix\n");
      return Clear;
    }
  } else {
    text_in_red("Error opening config file for reading\n");
    // when there is no file
    return Clear;
  }
  nlohmann::json result;
  if (find_problem_by_title(title, jsonArray, result)) {
    if (result.contains("url") &&
        (!result.contains("input") || !result.contains("output") ||
         !result.contains("title"))) {
      std::cout << "here" << result.contains("url") << std::endl;
      if (is_valid_link(result["url"])) {
        text_in_red("invalid data in json");
        text_in_green("uerl is correct\n");
        setup_problem(result["url"]);
      }
    } else if (!result.contains("url")) {
      std::cout << result << std::endl;
      text_in_red("invalid data and  url in json\n");
      text_in_red("please enter create mode to fix\n");
      return Clear;
    }

    problem->input_file = result["input"];
    problem->output_file = result["output"];
    problem->file_name = result["title"];
    problem->url = result["url"];
    if (!exists(problem->input_file) || !exists(problem->output_file) ||
        !exists(problem->file_name)) {
      text_in_red("your files are missing\n");
      text_in_green("setting up files\n");
      if (setup_problem(problem->url) != "")
        text_in_green("files are ready\n");
      else {
        text_in_red("could not setup problem\n");
        return Clear;
      }
    }
    text_in_green("Problem: " + title + " is ready to be solved" + "\n");
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
std::vector<std::string> collect_inputs_from_file(std::string input_file) {
  std::vector<std::string> inputs;
  std::string input = "";
  std::string temp;
  std::ifstream file(input_file);
  if (!file) {
    text_in_red("Failed to open input file\n");
    return {};
  }
  if (file.is_open()) {
    while (getline(file, temp)) {
      if (temp != "psio---")
        input += "\n" + (temp);
      else {
        inputs.push_back(input);
        input.clear();
      }
    }
    if (input.size() > 0) {
      inputs.push_back(input);
    }
    file.close();
  }
  return inputs;
}
std::pair<int, int> check_output(std::string output_file) {
  std::ifstream testcases_output(output_file), user_output("psio.output");
  std::vector<std::string> testcases_output_lines, user_output_lines;
  int passed = 0, testcases = 0;
  for (std::string line; std::getline(testcases_output, line);) {
    testcases_output_lines.push_back(line);
  }
  testcases_output_lines.push_back(
      "psio---"); // for last line becuse my output is missing it
  testcases_output.close();
  text_in_green("User output\n");
  for (std::string line; std::getline(user_output, line);) {
    user_output_lines.push_back(line);
    std::cout << line << "\n";
  }
  user_output.close();
  for (int i = 0; i < testcases_output_lines.size(); i++) {
    if (testcases_output_lines[i] == "psio---") {
      testcases++;
      continue;
    }
    if (testcases_output_lines[i] == user_output_lines[i]) {
      passed++;
    }
  }
  return (std::pair<int, int>(passed, testcases));
}

Mode create_problem(std::string url) {
  problem->url = url;
  std::string title = setup_problem(problem->url);
  if (title == "") {
    text_in_red("could not setup problem\n");
    return Clear;
  }
  title = remove_spaces(title);
  problem->file_name = title + ".cpp";
  problem->input_file = title + ".In";
  problem->output_file = title + ".Out";
  if (output_to_json(problem)) {
    text_in_green("Files setup complete\n");
    return Execution;
  } else {
    text_in_red("Error setting up problem\n");
    return Clear;
  }
}
inline void text_in_red(std::string s) {
  std::cerr << "\033[1;31m";
  std::cerr << "[ ERROR ] " << s;
  std::cerr << "\033[0m";
}
inline void text_in_green(std::string s) {
  std::cout << "\033[1;32m";
  std::cout << "[ INFO ] " << s;
  std::cout << "\033[0m";
}
inline void text_in_purple(std::string s) {
  std::cout << "\033[1;35m";
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
