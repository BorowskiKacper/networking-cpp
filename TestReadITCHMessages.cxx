#include <iostream>
#include <fstream>
#include <string>

int main()
{

    std::ifstream file("itchmessages/12302019.NASDAQ_ITCH50");
    std::string line;

    for (size_t i = 0; i < 10; i++)
    {
        std::getline(file, line);
        std::cout << line << '\n';
    }

    return EXIT_SUCCESS;
}