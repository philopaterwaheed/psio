# psio
an automation c++ program for case testing from CodeForces questions  

### dependencies
- curl : is  a general purpose library for sending HTTP requests in C++
    - `https://terminalroot.com/using-curl-with-cpp/`   a good tutorial
- gumbo : is a C++ library for parsing HTML in C++ 
    - `https://www.scrapingbee.com/blog/web-scraping-c++/`   a good tutorial
- nlohmann::json : is a header-only C++ JSON library
### use case 
psio is a program that takes Codeforces problems links and automats 
- creating a source code for solving from a template file 
- compiling / running your source code
- inputing provided test cases from Codeforces and checks them against the provided expected output provided by Codeforces 
### learned 
- how to use curl in c++
- how to use gumbo in c++
- more about pipes 
- how to redirect output from stdout to file and return it back 
- how to use nlohmann::json in c++
- how to do async functions 
- more knowledge about threads
### usage 
##### compile
```
g++ main.cpp -o psio -lcurl -lgumbo -lpthread
```

#### running
```
sudo cp ./psio /usr/bin
```
 #### starting
if running the program for the first time it asks you where your template file is ; please provide an existing template place .
the program stores the config file at 
```
~/.config/paio.config
```
change the first line if you want to change the 
template file .
the program has two options 
1. creating a new problem by providing a Codeforces; the program copies the template file as the problem file named as problem_name.cpp
2. choosing a problem created by the program
you just enter the problem_name.cpp to the program

<br> <br>
either way the if no issues in anything the program starts compiling the problem and fedding it to the compiled code 
and tells you what was the output and how many it has passed
<br>
<br>
to alter input and ouput you can change the 
`problem_name.In` and `problem_name.Out` files but please follow the file convention I set

### screenshots
![Screenshot from 2024-06-14 11-59-17](https://github.com/philopaterwaheed/psio/assets/61416026/16dc7cd3-36f5-4a9b-9c14-e3f52a452308)
