#include "md5_accumulator.h"

md5_accumulator::md5_accumulator() noexcept
{
    reset();
}

void md5_accumulator::accumulate(const char *message, size_t len) noexcept
{
    md5_accumulate(message, len, get_hash());
}

md5& md5_accumulator::get_hash() noexcept
{
    return hash;
}

md5 const& md5_accumulator::get_hash() const noexcept
{
    return hash;
}

void md5_accumulator::reset() noexcept
{
    hash.a = UINT32_C(0x67452301);
    hash.b = UINT32_C(0xEFCDAB89);
    hash.c = UINT32_C(0x98BADCFE);
    hash.d = UINT32_C(0x10325476);
}
