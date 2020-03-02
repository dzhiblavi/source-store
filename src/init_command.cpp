#include <cstddef>
#include <string>

#include "repository.h"

void init_command(size_t argc, char* argv[])
{
    std::string repository_root;
    if (argc != 0)
    {
        repository_root = *argv;
        --argc;
        ++argv;
    }
    else
        repository_root = default_repository_root();

    init_new_repository(repository_root);
}
