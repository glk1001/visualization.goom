// g++ -std=c++2a -Wall -Weffc++ -Wextra -Wsign-conversion -g -o test_convert test_convert.cpp test_utils.cpp

#include "../libs/utils/sstream_convert.hpp"
#include <string>
#include <iostream>
#include <ostream>
#include <exception>


int main( int argc, char **argv ) { 
    const char* c_strVal = convert_to<const char*>("Hello World with extras");
    std::cout << "c_strVal = '" << c_strVal << "'." << std::endl;

    const std::string strVal = convert_to<std::string>(std::string("Hello World with extras"));
    std::cout << "strVal = '" << strVal << "'." << std::endl;

    const double dblVal = convert_to<double>("13.37");
    std::cout << "dblVal = '" << dblVal << "'." << std::endl;

    const int intVal = convert_to<int>("13");
    std::cout << "intVal = '" << intVal << "'." << std::endl;

    const unsigned long ulongVal = convert_to<unsigned long>("13000000000012");
    std::cout << "ulongVal = '" << ulongVal << "'." << std::endl;

    try {
        const int badVal = convert_to<int>("aada");
        std::cout << "badVal = '" << badVal << "'." << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }


    const std::string c_strStr = convert_from<const char*>("Hello World with extras");
    std::cout << "c_strStr = \"" << c_strStr << "\"." << std::endl;

    const std::string strStr = convert_from<std::string>(std::string("Hello World with extras"));
    std::cout << "strStr = \"" << strStr << "\"." << std::endl;

    const std::string dblStr = convert_from<double>(13.37);
    std::cout << "dblStr = \"" << dblStr << "\"." << std::endl;

    const std::string intStr = convert_from<int>(13);
    std::cout << "intStr = \"" << intStr << "\"." << std::endl;

    const std::string ulongStr = convert_from<unsigned long>(13000000000012);
    std::cout << "ulongStr = \"" << ulongStr << "\"." << std::endl;

    return 0;
}
