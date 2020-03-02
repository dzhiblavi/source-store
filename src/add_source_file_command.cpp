#include <cstddef>
#include <stdexcept>
#include <sstream>

#include "file_descriptor.h"
#include "md5.h"
#include "repository.h"

void add_source_file_command(size_t argc, char* argv[])
{
    if (argc == 0)
        throw std::runtime_error("filename expected");

    file_descriptor objects_dir = file_descriptor::open(default_repository_root() + "/objects", file_flags::read_only | file_flags::directory | file_flags::close_on_exec);
    for (size_t i = 0; i != argc; ++i)
    {
        char const* filename = argv[i];

        // file on disk can be changed concurrectly
        std::vector<char> text = read_whole_file(filename);
        md5 hash = md5_hash(text.data(), text.size());
        
        std::stringstream ss;
        ss << hash;
        write_whole_file({objects_dir.get_fd(), ss.str()}, text);
    }
}
