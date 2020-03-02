// Compile with
//
// g++ -std=c++2a -Wall -Weffc++ -Wextra -Wsign-conversion test_date.cpp -o test_date

#include <ctime>
#include <iostream>
#include <locale>
 
int main()
{
    std::time_t t = std::time(nullptr);
    char mbstr[100];
    if (std::strftime(mbstr, sizeof(mbstr), "%A %c", std::localtime(&t))) {
        std::cout << mbstr << '\n';
    }
    if (std::strftime(mbstr, sizeof(mbstr), "%F_%T", std::localtime(&t))) {
        std::cout << mbstr << '\n';
    }
    if (std::strftime(mbstr, sizeof(mbstr), "%F_%H-%M-%S", std::localtime(&t))) {
        std::cout << mbstr << '\n';
    }
}
