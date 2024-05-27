#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>
#include <fstream>
#include <gumbo.h>
#include <iostream>
#include <sstream>
#include <vector>

int feed(std ::string input_file);
int compile(std ::string file_name);
std::string fetchHTML(const std::string &url);
void parseHTML(const std::string &html, std::vector<std::string> &inputs,
               std::vector<std::string> &outputs);
void searchForTestCases(GumboNode *node, std::vector<std::string> &inputs,
                        std::vector<std::string> &outputs);

static std::string output_exe = "output_program";
int main(int argc, char *argv[]) {
  if (argc != 3) { // get the source and it's input
    std::cerr << "Usage: " << argv[0] << " <cpp_source_file> <input_text_file>"
              << std::endl;
    return 1;
  }

  const char *cpp_file = argv[1];
  const char *input_file = argv[2];

  // compile(cpp_file);
  // feed(input_file);
  std ::string html =
      fetchHTML("https://codeforces.com/problemset/problem/1970/F3").c_str();
  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  parseHTML(html, inputs, inputs);

  for (int i = 0; i < inputs.size(); i++) {
    std::cout << inputs[i] << "in" << std::endl;
  }
  for (int i = 0; i < outputs.size(); i++) {
    std::cout << outputs[i] << "out" << std::endl;
  }
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
void extract_text_from_node(GumboNode* node, std::vector<std::string>& results) {
    if (node->type == GUMBO_NODE_TEXT ) {
        results.push_back(node->v.text.text);
    } else if (node->type == GUMBO_NODE_ELEMENT || node->type == GUMBO_NODE_DOCUMENT) {
	GumboAttribute* classAttr = gumbo_get_attribute(&node->v.element.attributes, "class");
        if (classAttr && std::string(classAttr->value) == "title") {
            // Skip this node and its children
            return;
        }
        const GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            extract_text_from_node(static_cast<GumboNode*>(children->data[i]), results);
        }
    }
}

// Function to search for the div with class "philo" and extract all text
void search_for_text(GumboNode* node, std::vector<std::string>& results , std:: string tag_name) {
    if (node->type != GUMBO_NODE_ELEMENT) return;
    GumboAttribute* classAttr;
    if (node->v.element.tag == GUMBO_TAG_DIV &&
        (classAttr = gumbo_get_attribute(&node->v.element.attributes, "class")) &&
        std::string(classAttr->value) == tag_name) {

        // Extract text from this node and all its children
        extract_text_from_node(node, results);
        return;  // No need to continue searching, assuming only one "philo" div is targeted
    }

    const GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        search_for_text(static_cast<GumboNode*>(children->data[i]), results, tag_name);
    }
}

void parseHTML(const std::string &html, std::vector<std::string> &inputs,
               std::vector<std::string> &outputs) {
  GumboOutput *output = gumbo_parse(html.c_str());
  std::vector<std::string> results;
  search_for_text(output->root, results, "input");
  search_for_text(output->root, results, "output");
  for (auto x : results) {
    std::cout << x<<"\n";
  }
}
