/*
 * MD5 hash computation
 * Copyright (c) 2020 Ivan Sorokin. (GPL3)
 * 
 * This file is part of Source Store.
 *
 * Source Store is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * Source Store is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Source Store.  If not, see <https://www.gnu.org/licenses/>.
 */

/* 
 * Copyright (c) 2017 Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/fast-md5-hash-implementation-in-x86-assembly
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 */

#include "md5.h"
#include "md5_accumulator.h"
#include <cstring>
#include <ostream>

std::ostream& operator<<(std::ostream& os, md5 const& hash)
{
    static char const hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                 '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    for (size_t i = 0; i != 16; ++i)
    {
        os.put(hex[hash.data[i] / 16]);
        os.put(hex[hash.data[i] % 16]);
    }

    return os;
}

#define BLOCK_LEN 64  // In bytes
#define STATE_LEN 4  // In words

extern "C" void md5_compress(md5* state, char const* block);

void md5_accumulate(char const* message, size_t len, md5& hash)
{
#define LENGTH_SIZE 8  // In bytes

    size_t off;
    for (off = 0; len - off >= BLOCK_LEN; off += BLOCK_LEN)
        md5_compress(&hash, message + off);

    char block[BLOCK_LEN] = {};
    size_t rem = len - off;
    memcpy(block, message + off, rem);

    block[rem] = (char)0x80;
    rem++;
    if (BLOCK_LEN - rem < LENGTH_SIZE)
    {
        md5_compress(&hash, block);
        memset(block, 0, sizeof(block));
    }

    block[BLOCK_LEN - LENGTH_SIZE] = static_cast<char>((len & 0x1FU) << 3);
    len >>= 5;
    for (int i = 1; i < LENGTH_SIZE; i++, len >>= 8)
        block[BLOCK_LEN - LENGTH_SIZE + i] = static_cast<char>(len & 0xFFU);
    md5_compress(&hash, block);
}

md5 md5_hash(char const* message, size_t len)
{
    md5_accumulator acc{};
    md5_accumulate(message, len, acc.get_hash());

    return acc.get_hash();
}
