#include "repository.h"
#include <cstdlib>
#include "file_descriptor.h"

can_not_detect_default_repository_root::can_not_detect_default_repository_root()
    : runtime_error("can not detect default repository root: XDG_CACHE_HOME and HOME environment variables are not set")
{}

std::string default_repository_root()
{
    if (char const* cache_dir = getenv("XDG_CACHE_HOME"))
        return std::string(cache_dir) + "/source-store";

    if (char const* home_dir = getenv("HOME"))
        return std::string(home_dir) + "/.cache/source-store";

    throw can_not_detect_default_repository_root();
}

void init_new_repository(std::string const& repository_root)
{
    mkdir(repository_root);
    try
    {
        file_descriptor root = file_descriptor::open(repository_root, file_flags::read_only | file_flags::close_on_exec | file_flags::directory);
        mkdir({root.get_fd(), "objects"});
    }
    catch (...)
    {
        unlink(repository_root);
        throw;
    }
}
