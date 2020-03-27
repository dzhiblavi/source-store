#ifndef DWARF_TEST_DEBUG_H
#define DWARF_TEST_DEBUG_H

#include <stdexcept>
#include <functional>

#include <libdwarf.h>

#define DWARF_THROW_ERROR(reason) throw dwarf::error(std::string("DWARF ERROR: ") \
        + reason + " in function: " + __func__ + "(...)")

#define DWARF_THROW_ERROR_ORIGIN(reason, origin) throw dwarf::error(std::string("DWARF ERROR: ") \
        + reason + " in function: " + origin + "(...)")

namespace dwarf
{
struct error : std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

struct cu_header
{
    Dwarf_Unsigned cu_header_length = 0;
    Dwarf_Half     version_stamp = 0;
    Dwarf_Off      abbrev_offset = 0;
    Dwarf_Half     address_size = 0;
    Dwarf_Half     length_size = 0;
    Dwarf_Half     extension_size = 0;
    Dwarf_Sig8     type_signature;
    Dwarf_Unsigned typeoffset = 0;
    Dwarf_Unsigned next_cu_header_offset = 0;
    Dwarf_Half     header_cu_type = 0;
};

int check_for_error(char const* what, char const* func, int res);

class debug
{
public:
    typedef Dwarf_Debug native_handle_t;

private:
    native_handle_t handle_ = nullptr;

public:
    debug() noexcept = default;

    explicit debug(int fd, int mode = DW_DLC_READ, Dwarf_Error* perror = nullptr);

    debug(debug const&) = delete;

    debug& operator=(debug const&) = delete;

    debug(debug&& other) noexcept;

    debug& operator=(debug&& other) noexcept;

    ~debug();

    debug& swap(debug& other) noexcept;

    void finish(Dwarf_Error* perror = nullptr);

    void dealloc(void* space, Dwarf_Unsigned type) noexcept;

    int next_cu_header(Dwarf_Bool is_info, cu_header* cu, Dwarf_Error* perror = nullptr);

    int sibling_of(Dwarf_Die r, Dwarf_Die* res, bool is_info, Dwarf_Error* perror = nullptr);

    [[nodiscard]] native_handle_t native_handle() noexcept;
};
} // namespace dwarf

#endif //DWARF_TEST_DEBUG_H