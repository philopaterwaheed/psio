# psio
an automation c++ program for case testing from CodeForces questions  

### dependencies
- curl : is  a general purpose library for sending HTTP requests in C++
    - `https://terminalroot.com/using-curl-with-cpp/`   a good tutorial
- gumbo : is a C++ library for parsing HTML in C++ 
    - `https://www.scrapingbee.com/blog/web-scraping-c++/`   a good tutorial
- nlohmann::json : is a header-only C++ JSON library

### learned 
- how to use curl in c++
- how to use gumbo in c++
- more about pipes 
- how to redirect output from stdout to file and return it back 
### usage 
##### compile
```
g++ main.cpp -o psio -lcurl -lgumbo 
```
