#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <iosfwd>

struct md5
{
    union
    {
        struct
        {
            uint32_t a, b, c, d;
        };

        uint8_t data[16];
    };
};

std::ostream& operator<<(std::ostream& os, md5 const& hash);

md5 md5_hash(char const* message, size_t len);
