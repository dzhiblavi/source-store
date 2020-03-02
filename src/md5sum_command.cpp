#include <cstddef>
#include <stdexcept>
#include <iostream>

#include "file_descriptor.h"
#include "md5.h"

void md5sum_command(size_t argc, char* argv[])
{
    if (argc == 0)
        throw std::runtime_error("filename expected");

    for (size_t i = 0; i != argc; ++i)
    {
        char const* filename = argv[i];

        std::vector<char> text = read_whole_file(filename);
        md5 hash = md5_hash(text.data(), text.size());
        std::cout << hash << ' ' << filename << '\n';
    }
}
