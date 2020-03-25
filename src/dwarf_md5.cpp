#include "dwarf_md5.h"

#include <fcntl.h>
#include <cassert>
#include <unistd.h>
#include <iostream>

#include "dwarf_debug.h"

namespace
{
void process_cu_die(dwarf::debug& dbg, Dwarf_Die cu_die, std::vector<std::pair<std::string, md5>>& result)
{
    Dwarf_Unsigned lineversion = 0;
    Dwarf_Signed linecount = 0;
    Dwarf_Line *linebuf = nullptr;
    Dwarf_Small  table_count = 0;
    int lres = 0;
    Dwarf_Line_Context line_context = 0;

    lres = dwarf_srclines_b(cu_die, &lineversion,
                            &table_count, &line_context,
                            nullptr);
    if (lres == DW_DLV_OK)
    {
        lres = dwarf_srclines_from_linecontext(line_context,
                                               &linebuf, &linecount, nullptr);
    }
    else
    {
        // TODO: error handling
        assert(false);
    }

    const char* name = 0;
    Dwarf_Signed baseindex = 0;
    Dwarf_Signed file_count = 0;
    Dwarf_Signed endindex = 0;

    lres = dwarf_srclines_files_indexes(line_context,
                                        &baseindex, &file_count, &endindex, nullptr);
    if (lres != DW_DLV_OK)
    {
        // TODO: error handling
        assert(false);
    }

    for (int i = baseindex; i < endindex; ++i)
    {
        Dwarf_Unsigned dirindex = 0;
        Dwarf_Unsigned modtime = 0;
        Dwarf_Unsigned flength = 0;
        Dwarf_Form_Data16 *md5data = 0;

        lres = dwarf_srclines_files_data_b(line_context, i,
                                           &name, &dirindex, &modtime, &flength,
                                           &md5data, nullptr);
        if (lres != DW_DLV_OK)
        {
            // TODO: error handling
            assert(false);
        }

        if (md5data == nullptr) {
            // :NOTE: no md5 at all
            // TODO: error handling
            assert(false);
        }

        md5 hash{};
        std::copy(md5data->fd_data, md5data->fd_data + 16, hash.data);

        result.emplace_back(name, hash);
    }
}

std::vector<std::pair<std::string, md5>> read_cu_list(dwarf::debug& dbg)
{
    std::vector<std::pair<std::string, md5>> result;
    Dwarf_Bool is_info = 1;
    dwarf::cu_header cu;

    for (int cu_number = 0; ;++cu_number)
    {
        Dwarf_Die no_die = 0;
        Dwarf_Die cu_die = 0;

        if (dbg.next_cu_header(is_info, &cu, nullptr) == DW_DLV_NO_ENTRY)
        {
            return result;
        }

        dbg.sibling_of(no_die, &cu_die, is_info, nullptr);
        process_cu_die(dbg, cu_die, result);
        dbg.dealloc(cu_die, DW_DLA_DIE);
    }
}
}

namespace dwarf
{
std::vector<std::pair<std::string, md5>> get_source_files(char const *filename)
{
    // TODO: OMG FIX IT!
    int fd = open(filename, O_RDONLY);

    dwarf::debug dbg(fd);
    auto r = read_cu_list(dbg);

    close(fd);
    return r;
}
}

void get_source_files(size_t argc, char* argv[])
{
    if (argc == 0)
        throw std::runtime_error("filename expected");

    for (size_t i = 0; i != argc; ++i)
    {
        auto v = dwarf::get_source_files(argv[i]);

        for (auto const& p : v)
        {
            std::cout << "'" << p.first << "', md5 value: " << p.second << std::endl;
        }
    }
}