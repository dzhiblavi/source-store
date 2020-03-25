#ifndef SOURCE_STORE_MD5_ACCUMULATOR_H
#define SOURCE_STORE_MD5_ACCUMULATOR_H

#include <cstddef>
#include "md5.h"

class md5_accumulator
{
    md5 hash{};

public:
    md5_accumulator() noexcept;

    void accumulate(char const* message, size_t len) noexcept;
    
    md5& get_hash() noexcept;
    
    md5 const& get_hash() const noexcept;

    void reset() noexcept;
};


#endif //SOURCE_STORE_MD5_ACCUMULATOR_H
