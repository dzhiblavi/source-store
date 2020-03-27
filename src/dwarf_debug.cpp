#include <iostream>
#include "dwarf_debug.h"

namespace dwarf
{
int check_for_error(char const* what, char const* func, int res)
{
    if (res == DW_DLV_ERROR)
    {
        DWARF_THROW_ERROR_ORIGIN(what, func);
    }

    return res;
}

debug::native_handle_t debug::native_handle() noexcept
{
    return handle_;
}

debug::debug(int fd, int mode, Dwarf_Error* perror)
{
    check_for_error("dwarf initialization failed", __func__,
                    dwarf_init(fd, mode, nullptr, nullptr, &handle_, perror));
}

debug::debug(debug&& other) noexcept
{
    swap(other);
}

debug& debug::operator=(debug&& other) noexcept
{
    return swap(other);
}

debug& debug::swap(debug& other) noexcept
{
    std::swap(handle_, other.handle_);
    return *this;
}

debug::~debug()
{
    try
    {
        if (handle_)
            finish();
    }
    catch (...)
    {
        std::terminate();
    }
}

void debug::finish(Dwarf_Error* perror)
{
    dwarf_finish(handle_, perror);
}

void debug::dealloc(void* space, Dwarf_Unsigned type) noexcept
{
    dwarf_dealloc(handle_, space, type);
}

int debug::next_cu_header(Dwarf_Bool is_info, cu_header* cu, Dwarf_Error* perror)
{
    return check_for_error("next_cu_header failed", __func__,
                           dwarf_next_cu_header_d(native_handle(), is_info
                                   , &cu->cu_header_length, &cu->version_stamp, &cu->abbrev_offset
                                   , &cu->address_size, &cu->length_size, &cu->extension_size
                                   , &cu->type_signature, &cu->typeoffset, &cu->next_cu_header_offset
                                   , &cu->header_cu_type, perror));
}

int debug::sibling_of(Dwarf_Die r, Dwarf_Die* res, bool is_info, Dwarf_Error* perror)
{
    return check_for_error("sibling_of failed", __func__, dwarf_siblingof_b(native_handle()
            , r, is_info, res, perror));
}
} // namespace dwarf
