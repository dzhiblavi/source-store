#ifndef SOURCE_STORE_DWARF_MD5_H
#define SOURCE_STORE_DWARF_MD5_H

#include <string>
#include <vector>

#include "md5.h"

namespace dwarf
{
std::vector<std::pair<std::string, md5>> get_source_files(char const *filename);
}

#endif //SOURCE_STORE_DWARF_MD5_H
