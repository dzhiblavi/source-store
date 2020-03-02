#include "file_descriptor.h"
#include <cassert>
#include <cstring>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <algorithm>
#include <iostream>

namespace
{
    char const* error_enum_name(int err)
    {
        switch (err)
        {
        case ENOENT:
            return "ENOENT";
        case EBADF:
            return "EBADF";
        case EAGAIN:
            return "EAGAIN";
        case EACCES:
            return "EACCES";
        case EEXIST:
            return "EEXIST";
        case EISDIR:
            return "EISDIR";
        case EINVAL:
            return "EINVAL";
        case EMFILE:
            return "EMFILE";
        case EPIPE:
            return "EPIPE";
        case ENOTEMPTY:
            return "ENOTEMPTY";
        case ENOTSOCK:
            return "ENOTSOCK";
        case EADDRINUSE:
            return "EADDRINUSE";
        case EADDRNOTAVAIL:
            return "EADDRNOTAVAIL";
        case ECONNRESET:
            return "ECONNRESET";
        case ECONNREFUSED:
            return "ECONNREFUSED";
        }

        return "<unknown error>";
    }

    void print_error(std::ostream& out, int err, char const* action)
    {
        out << action << " failed, error: " << error_enum_name(err);
        char tmp[2048];
        char const* err_msg = strerror_r(err, tmp, sizeof tmp);
        out << " (" << err << ", " << err_msg << ")";
    }

    void throw_error [[noreturn]] (int err, char const* action)
    {
        std::stringstream ss;
        print_error(ss, err, action);
        throw std::runtime_error(ss.str());
    }
}

file_flags operator|(file_flags a, file_flags b)
{
    return static_cast<file_flags>(static_cast<int>(a) | static_cast<int>(b));
}

file_flags& operator|=(file_flags& a, file_flags b)
{
    a = a | b;
    return a;
}

file_mode operator|(file_mode a, file_mode b)
{
    return static_cast<file_mode>(static_cast<int>(a) | static_cast<int>(b));
}

file_mode& operator|=(file_mode& a, file_mode b)
{
    a = a | b;
    return a;
}

size_t nonblock_result::bytes() const
{
    assert(is_success() || is_eof());
    return static_cast<size_t>(value);
}

bool nonblock_result::is_success() const
{
    return value > 0;
}

bool nonblock_result::is_eof() const
{
    return value == 0;
}

bool nonblock_result::is_wouldblock() const
{
    return value < 0;
}

nonblock_result::nonblock_result(ssize_t value)
    : value(value)
{}

file_location::file_location(int basedir, char const* filename)
    : basedir(basedir)
    , filename(filename)
{}

file_location::file_location(directory_stream const& basedir, char const* filename)
    : basedir(basedir.get_fd())
    , filename(filename)
{}

file_location::file_location(char const* filename)
    : basedir(AT_FDCWD)
    , filename(filename)
{}

file_location::file_location(int basedir, std::string const& filename)
    : basedir(basedir)
    , filename(filename.c_str())
{}

file_location::file_location(directory_stream const& basedir, std::string const& filename)
    : basedir(basedir.get_fd())
    , filename(filename.c_str())
{}

file_location::file_location(std::string const& filename)
    : basedir(AT_FDCWD)
    , filename(filename.c_str())
{}

file_descriptor::file_descriptor()
    : file(INVALID_VALUE)
{}

file_descriptor::file_descriptor(file_descriptor&& other) noexcept
    : file(other.file)
{
    other.file = INVALID_VALUE;
}

file_descriptor& file_descriptor::operator=(file_descriptor&& rhs) noexcept
{
    close();
    file = rhs.file;
    rhs.file = INVALID_VALUE;
    return *this;
}

file_descriptor::~file_descriptor()
{
    close();
}

file_descriptor::operator bool() const
{
    return file != INVALID_VALUE;
}

void file_descriptor::close()
{
    if (file != INVALID_VALUE)
    {
        int r = ::close(file);
        if (r != 0)
        {
            assert(r == -1);
            int err = errno;
            switch (err)
            {
            case EINTR:
            case EIO:
            case ENOSPC:
            case EDQUOT:
                break;
            default:
                print_error(std::cerr, err, "close");
                std::cerr << std::endl;
                std::abort();
            }
        }
        file = INVALID_VALUE;
    }
}

int file_descriptor::get_fd() const
{
    return file;
}

int file_descriptor::release()
{
    int result = file;
    file = INVALID_VALUE;
    return result;
}

nonblock_result file_descriptor::read_nonblock(void *data, size_t size)
{
    ssize_t bytes_read = ::read(file, data, size);
    if (bytes_read < 0)
    {
        assert(bytes_read == -1);
        int err = errno;
        if (err == EAGAIN || err == EWOULDBLOCK)
            return nonblock_result(-1);

        throw_error(err, "read");
    }

    return nonblock_result(bytes_read);
}

size_t file_descriptor::read_some(void* data, size_t size)
{
    assert(!is_nonblock());

    ssize_t bytes_read = ::read(file, data, size);
    if (bytes_read < 0)
    {
        assert(bytes_read == -1);
        throw_error(errno, "read");
    }

    return static_cast<size_t>(bytes_read);
}

void file_descriptor::read(void* data, size_t size)
{
    size_t bytes_read = read_some(data, size);
    if (bytes_read != size)
    {
        std::stringstream ss;
        ss << "incomplete read, requested: " << size << ", written: " << bytes_read;
        throw std::runtime_error(ss.str());
    }
}

size_t file_descriptor::write_some(void const* data, size_t size)
{
    ssize_t bytes_written = ::write(file, data, size);
    if (bytes_written < 0)
    {
        assert(bytes_written == -1);
        throw_error(errno, "write");
    }

    return static_cast<size_t>(bytes_written);
}

void file_descriptor::write(void const* data, size_t size)
{
    size_t bytes_written = write_some(data, size);
    if (bytes_written != size)
    {
        std::stringstream ss;
        ss << "incomplete write, requested: " << size << ", written: " << bytes_written;
        throw std::runtime_error(ss.str());
    }
}

int64_t file_descriptor::seek(int64_t offset, seek_origin whence)
{
    errno = 0;
    int64_t result = ::lseek64(file, offset, static_cast<int>(whence));
    if (result < 0)
    {
        int err = errno;
        if (err != 0)
            throw_error(err, "seek");
    }

    return result;
}

int64_t file_descriptor::tell() const
{
    return const_cast<file_descriptor&>(*this).seek(0, seek_origin::current_position);
}

void file_descriptor::set_close_on_exec(bool value)
{
    int r1 = fcntl(file, F_GETFD);
    if (r1 < 0)
    {
        assert(r1 == -1);
        throw_error(errno, "fcntl(F_GETFD)");
    }

    if (value)
        r1 |= FD_CLOEXEC;
    else
        r1 &= ~FD_CLOEXEC;

    int r2 = fcntl(file, F_SETFD, r1);
    if (r2 < 0)
    {
        assert(r2 == -1);
        throw_error(errno, "fcntl(F_SETFD)");
    }
}

void file_descriptor::set_nonblock(bool value)
{
    int r1 = fcntl(file, F_GETFL);
    if (r1 < 0)
    {
        assert(r1 == -1);
        throw_error(errno, "fcntl(F_GETFL)");
    }

    if (value)
        r1 |= O_NONBLOCK;
    else
        r1 &= ~O_NONBLOCK;

    int r2 = fcntl(file, F_SETFL, r1);
    if (r2 < 0)
    {
        assert(r2 == -1);
        throw_error(errno, "fcntl(F_SETFL)");
    }
}

bool file_descriptor::is_nonblock() const
{
    int r1 = fcntl(file, F_GETFL);
    if (r1 < 0)
    {
        assert(r1 == -1);
        throw_error(errno, "fcntl(F_GETFL)");
    }
    
    return r1 & O_NONBLOCK;
}

struct stat64 file_descriptor::stat() const
{
    struct stat64 result;

    int r = ::fstat64(file, &result);
    if (r != 0)
        throw_error(errno, "fstat");

    return result;
}

file_descriptor file_descriptor::attach(int fd) noexcept
{
    file_descriptor result;
    result.file = fd;
    return result;
}

file_descriptor file_descriptor::open(file_location location, file_flags flags, file_mode mode)
{
    file_descriptor fd;

    fd.file = ::openat(location.basedir, location.filename, static_cast<int>(flags), static_cast<int>(mode));

    if (fd.file == INVALID_VALUE)
        throw_error(errno, "open");

    return fd;
}

file_descriptor file_descriptor::open_if_exists(file_location location, file_flags flags, file_mode mode)
{
    file_descriptor fd;

    fd.file = ::openat(location.basedir, location.filename, static_cast<int>(flags), static_cast<int>(mode));

    if (fd.file == INVALID_VALUE)
    {
        int err = errno;
        if (err != ENOENT)
            throw_error(err, "open");
    }

    return fd;
}

directory_stream::directory_stream()
{}

directory_stream::directory_stream(file_descriptor fd)
    : fd(std::move(fd))
    , buf(new char[BUF_SIZE])
    , current(buf.get())
    , end(buf.get())
{}

directory_stream::directory_stream(directory_stream&& other) noexcept
    : fd(std::move(fd))
    , buf(std::move(buf))
    , current(other.current)
    , end(other.end)
{
    other.current = nullptr;
    other.end = nullptr;
}

directory_stream& directory_stream::operator=(directory_stream&& other) noexcept
{
    if (this != &other)
    {
        fd = std::move(fd);
        buf = std::move(buf);
        current = other.current;
        end = other.end;

        other.current = nullptr;
        other.end = nullptr;
    }

    return *this;
}

directory_stream::~directory_stream()
{}

directory_stream::operator bool() const
{
    return buf != nullptr;
}

void directory_stream::close()
{
    fd.close();
    buf.reset();
}

directory_stream::dirent const* directory_stream::next()
{
    if (current == end)
    {
        ssize_t r = syscall(SYS_getdents64, fd.get_fd(), buf.get(), BUF_SIZE);
        if (r < 0)
        {
            int err = errno;

            if (err != 0)
                throw_error(err, "readdir");
        }

        if (r == 0)
            return nullptr;

        current = buf.get();
        end = buf.get() + r;
    }

    assert(current < end);
    dirent const* ent = static_cast<dirent const*>(current);
    current = (char*)current + ent->d_reclen;

    return ent;
}

int directory_stream::get_fd() const
{
    return fd.get_fd();
}

sorted_directory_stream::sorted_directory_stream()
{}

sorted_directory_stream::sorted_directory_stream(file_descriptor fd)
    : fd(std::move(fd))
    , current(0)
{
    for (;;)
    {
        std::unique_ptr<char[]> buf(new char[BUF_SIZE]);
        ssize_t r = syscall(SYS_getdents64, this->fd.get_fd(), buf.get(), BUF_SIZE);
        if (r < 0)
        {
            int err = errno;

            if (err != 0)
                throw_error(err, "readdir");
        }

        if (r == 0)
            break;

        dirent const* p = reinterpret_cast<dirent const*>(buf.get());
        void const* end = buf.get() + r;

        bufs.push_back(std::move(buf));

        while (p != end)
        {
            ents.push_back(p);
            p = reinterpret_cast<dirent const*>((char*)p + p->d_reclen);
        }
    }
    
    std::sort(ents.begin(), ents.end(), [](dirent const* a, dirent const* b)
    {
        return strcmp(a->d_name, b->d_name) < 0;
    });
}

sorted_directory_stream::sorted_directory_stream(sorted_directory_stream&& other) noexcept
    : fd(std::move(other.fd))
    , bufs(std::move(other.bufs))
    , ents(std::move(other.ents))
    , current(0)
{
    other.current = 0;
}

sorted_directory_stream& sorted_directory_stream::operator=(sorted_directory_stream&& other) noexcept
{
    if (this != &other)
    {
        fd = std::move(other.fd);
        bufs = std::move(other.bufs);
        ents = std::move(other.ents);
        current = other.current;
        other.current = 0;
    }

    return *this;
}

sorted_directory_stream::~sorted_directory_stream()
{}

sorted_directory_stream::operator bool() const
{
    return static_cast<bool>(fd);
}

void sorted_directory_stream::close()
{
    fd.close();
    bufs.clear();
    ents.clear();
    current = 0;
}

sorted_directory_stream::dirent const* sorted_directory_stream::next()
{
    if (current == ents.size())
        return nullptr;

    dirent const* result = ents[current];
    ++current;
    return result;
}

int sorted_directory_stream::get_fd() const
{
    return fd.get_fd();
}

void mkdir(file_location location, file_mode mode)
{
    int r = ::mkdirat(location.basedir, location.filename, static_cast<mode_t>(mode));
    if (r < 0)
    {
        int err = errno;
        assert(r == -1);
        throw_error(err, "mkdirat");
    }
}

bool mkdir_if_not_exists(file_location location, file_mode mode)
{
    int r = ::mkdirat(location.basedir, location.filename, static_cast<mode_t>(mode));
    if (r < 0)
    {
        int err = errno;
        if (err == EEXIST)
            return false;

        assert(r == -1);
        throw_error(err, "mkdirat");
    }

    return true;
}

void chmod(file_location location, file_mode mode)
{
    int r = fchmodat(location.basedir, location.filename, static_cast<mode_t>(mode), 0);
    if (r < 0)
    {
        int err = errno;
        assert(r == -1);
        throw_error(err, "chmodat");
    }
}

struct stat64 stat(file_location location, stat_flags flags)
{
    struct stat64 result;

    int r = fstatat64(location.basedir, location.filename, &result, static_cast<int>(flags));
    if (r != 0)
        throw_error(errno, "fstatat64");

    return result;
}

pipe_flags operator|(pipe_flags a, pipe_flags b)
{
    return static_cast<pipe_flags>(static_cast<int>(a) | static_cast<int>(b));
}

pipe_flags& operator|=(pipe_flags& a, pipe_flags b)
{
    a = a | b;
    return a;
}

pipe_fds make_pipe(pipe_flags flags)
{
    int fds[2];
    int r = pipe2(fds, static_cast<int>(flags));
    if (r < 0)
    {
        assert(r == -1);
        throw_error(errno, "pipe2");
    }

    pipe_fds result;
    result.read_end = file_descriptor::attach(fds[0]);
    result.write_end = file_descriptor::attach(fds[1]);
    return result;
}

void dup(int source, int target, dup_flags flags)
{
    int r = ::dup3(source, target, static_cast<int>(flags));
    if (r < 0)
    {
        assert(r == -1);
        throw_error(errno, "dup3");
    }
}

size_t poll_fds(pollfd* fds, size_t nfds, int timeout)
{
    for (;;)
    {
        int r = ::poll(fds, nfds, timeout);
        if (r < 0)
        {
            assert(r == -1);
            int err = errno;
            if (err == EAGAIN || err == EINTR)
                continue;
            throw_error(err, "poll");
        }
        
        return static_cast<size_t>(r);
    }
}

size_t poll_fds(pollfd* fds, size_t nfds)
{
    size_t n = poll_fds(fds, nfds, -1);
    assert(n != 0);
    return n;
}

void change_directory(char const* path)
{
    int r = chdir(path);
    if (r < 0)
    {
        assert(r == -1);
        throw_error(errno, "chdir");
    }
}

void change_directory(std::string const& path)
{
    change_directory(path.c_str());
}

void unlink(file_location location, unlink_flags flags)
{
    int r = unlinkat(location.basedir, location.filename, static_cast<int>(flags));
    if (r < 0)
    {
        int err = errno;
        throw_error(err, "unlink");
    }

    assert(r == 0);
}

std::vector<char> read_whole_file(file_location location)
{
    std::vector<char> buf;

    file_descriptor fd = file_descriptor::open(location, file_flags::read_only | file_flags::close_on_exec);
    auto size = fd.stat().st_size;
    if (size >= buf.max_size())
        throw std::runtime_error("file is too large");

    buf.resize(static_cast<size_t>(size));
    size_t bytes_read = fd.read_some(buf.data(), buf.size());

    // shrink vector if file was truncated after we stat'ed it and before we read it
    // these two lines are redundant in 99.9% of cases and are very cheap
    buf.resize(bytes_read);
    buf.shrink_to_fit();

    return buf;
}

std::unique_ptr<std::vector<char> > read_whole_file_if_exists(file_location location)
{
    std::vector<char> buf;

    file_descriptor fd = file_descriptor::open_if_exists(location, file_flags::read_only | file_flags::close_on_exec);
    if (!fd)
        return nullptr;

    auto size = fd.stat().st_size;
    if (size >= buf.max_size())
        throw std::runtime_error("file is too large");

    buf.resize(static_cast<size_t>(size));
    size_t bytes_read = fd.read_some(buf.data(), buf.size());

    // shrink vector if file was truncated after we stat'ed it and before we read it
    // these two lines are redundant in 99.9% of cases and are very cheap
    buf.resize(bytes_read);
    buf.shrink_to_fit();

    return std::unique_ptr<std::vector<char>>(new std::vector<char>(std::move(buf)));
}

void write_whole_file(file_location location, std::vector<char> const& data)
{
    file_descriptor fd = file_descriptor::open(location, file_flags::write_only | file_flags::create | file_flags::truncate | file_flags::close_on_exec);
    fd.write(data.data(), data.size());
}
