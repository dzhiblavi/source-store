#pragma once

#include <string>
#include <stdexcept>

struct can_not_detect_default_repository_root : std::runtime_error
{
    can_not_detect_default_repository_root();
};

std::string default_repository_root();

void init_new_repository(std::string const& path);
