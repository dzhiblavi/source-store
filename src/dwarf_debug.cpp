#include "dwarf_debug.h"

namespace
{
int check_for_error(char const* what, char const* func, int res)
{
    if (res == DW_DLV_ERROR)
    {
        DWARF_THROW_ERROR_ORIGIN(what, func);
    }

    return res;
}
}

namespace dwarf
{
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

void debug::traverse_dies(Dwarf_Die root, std::function<void(debug&, Dwarf_Die, Dwarf_Error*)> const& f
        , Dwarf_Error* perror)
{
    Dwarf_Debug dbg = native_handle();
    int res = DW_DLV_ERROR;
    Dwarf_Die cur_die = root;
    Dwarf_Die child = 0;
    Dwarf_Error *errp = 0;

    f(*this, root, perror);

    for (;;)
    {
        Dwarf_Die sib_die = 0;

        // TODO: implement child same way as sibling_of
        res = check_for_error("dwarf_child() failed", __func__, dwarf_child(cur_die, &child, errp));

        if (res == DW_DLV_OK)
        {
            traverse_dies(child, f, perror);
            dwarf_dealloc(dbg, child, DW_DLA_DIE);
            child = 0;
        }

        res = sibling_of(cur_die, &sib_die, 1, perror);

        if (res == DW_DLV_NO_ENTRY)
        {
            break;
        }

        if (cur_die != root)
        {
            dwarf_dealloc(dbg, cur_die, DW_DLA_DIE);
        }

        cur_die = sib_die;
        f(*this, cur_die, perror);
    }
}
} // namespace dwarf
