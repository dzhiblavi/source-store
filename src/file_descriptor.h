#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <vector>
#include <memory>

struct file_descriptor;
struct directory_stream;
enum class unlink_flags : int;

enum class file_flags : int
{
    read_only     = O_RDONLY,
    read_write    = O_RDWR,
    write_only    = O_WRONLY,

    append        = O_APPEND,
    async         = O_ASYNC,
    close_on_exec = O_CLOEXEC,
    create        = O_CREAT,
    directory     = O_DIRECTORY,
    dsync         = O_DSYNC,
    excl          = O_EXCL,
    noatime       = O_NOATIME,
    noctty        = O_NOCTTY,
    nofollow      = O_NOFOLLOW,
    nonblock      = O_NONBLOCK,
    path          = O_PATH,
    sync          = O_SYNC,
    temporary     = O_TMPFILE,
    truncate      = O_TRUNC
};

file_flags operator|(file_flags a, file_flags b);
file_flags& operator|=(file_flags& a, file_flags b);

enum class file_mode : int
{
    none           = 0,

    owner_read     = S_IRUSR,
    owner_write    = S_IWUSR,
    owner_execute  = S_IXUSR,
    owner_all      = owner_read | owner_write | owner_execute,

    group_read     = S_IRGRP,
    group_write    = S_IWGRP,
    group_execute  = S_IXGRP,
    group_all      = group_read | group_write | group_execute,

    others_read     = S_IROTH,
    others_write    = S_IWOTH,
    others_execute  = S_IXOTH,
    others_all      = others_read | others_write | others_execute,

    set_user_id     = S_ISUID,
    set_group_id    = S_ISGID,
    sticky          = S_ISVTX,

    file_default      = owner_read | owner_write | group_read | group_write | others_read | others_write,
    directory_default = owner_all | group_all | others_all,
};

file_mode operator|(file_mode a, file_mode b);
file_mode& operator|=(file_mode& a, file_mode b);

enum class stat_flags : int
{
    none             = 0,

    empty_path       = AT_EMPTY_PATH,
    no_automount     = AT_NO_AUTOMOUNT,
    symlink_nofollow = AT_SYMLINK_NOFOLLOW,
};

enum class seek_origin : int
{
    file_start       = SEEK_SET,
    current_position = SEEK_CUR,
    file_end         = SEEK_END,
};

struct file_location
{
    file_location(int basedir, char const* filename);
    file_location(directory_stream const& basedir, char const* filename);
    file_location(char const* filename);

    file_location(int basedir, std::string const& filename);
    file_location(directory_stream const& basedir, std::string const& filename);
    file_location(std::string const& filename);

private:
    int basedir;
    char const* filename;

    friend struct file_descriptor;
    friend void mkdir(file_location location, file_mode mode);
    friend bool mkdir_if_not_exists(file_location location, file_mode mode);
    friend void chmod(file_location location, file_mode mode);
    friend struct stat64 stat(file_location location, stat_flags flags);
    friend void unlink(file_location location, unlink_flags flags);
};

struct nonblock_result
{
    size_t bytes() const;
    bool is_success() const;
    bool is_eof() const;
    bool is_wouldblock() const;

private:
    explicit nonblock_result(ssize_t value);

private:
    ssize_t value;

    friend struct file_descriptor;
};

struct file_descriptor
{
    file_descriptor();
    file_descriptor(file_descriptor&&) noexcept;
    file_descriptor& operator=(file_descriptor&&) noexcept;
    ~file_descriptor();

    explicit operator bool() const;

    void close();
    int get_fd() const;
    int release();

    nonblock_result read_nonblock(void* data, size_t size);
    size_t read_some(void* data, size_t size);
    void read(void* data, size_t size);

    size_t write_some(void const* data, size_t size);
    void write(void const* data, size_t size);

    int64_t seek(int64_t offset, seek_origin whence = seek_origin::file_start);
    int64_t tell() const;

    void set_close_on_exec(bool value);
    void set_nonblock(bool value);
    
    bool is_nonblock() const;

    struct stat64 stat() const;

    static file_descriptor attach(int fd) noexcept;
    static file_descriptor open(file_location location, file_flags flags, file_mode mode = file_mode::file_default);
    static file_descriptor open_if_exists(file_location location, file_flags flags, file_mode mode = file_mode::file_default);

private:
    int file;
    static constexpr int const INVALID_VALUE = -1;
};

struct directory_stream
{
    struct dirent
    {
        ino64_t        d_ino;
        off64_t        d_off;
        unsigned short d_reclen;
        unsigned char  d_type;
        char           d_name[];
    };

    directory_stream();
    directory_stream(file_descriptor fd);
    directory_stream(directory_stream&&) noexcept;
    directory_stream& operator=(directory_stream&&) noexcept;
    ~directory_stream();

    explicit operator bool() const;

    void close();
    
    dirent const* next();
    int get_fd() const;

private:
    static constexpr size_t BUF_SIZE = 32 * 1024;

    file_descriptor fd;
    std::unique_ptr<char[]> buf;
    void* current;
    void* end;

    friend struct sorted_directory_stream;
};

struct sorted_directory_stream
{
    using dirent = directory_stream::dirent;

    sorted_directory_stream();
    sorted_directory_stream(file_descriptor fd);
    sorted_directory_stream(sorted_directory_stream&&) noexcept;
    sorted_directory_stream& operator=(sorted_directory_stream&&) noexcept;
    ~sorted_directory_stream();

    explicit operator bool() const;

    void close();

    dirent const* next();
    int get_fd() const;

private:
    static constexpr size_t BUF_SIZE = directory_stream::BUF_SIZE;

    file_descriptor fd;
    std::vector<std::unique_ptr<char[]>> bufs;
    std::vector<dirent const*> ents;
    size_t current;
};

void mkdir(file_location, file_mode mode = file_mode::directory_default);
bool mkdir_if_not_exists(file_location, file_mode mode = file_mode::directory_default);

void chmod(file_location, file_mode mode = file_mode::directory_default);

struct stat64 stat(file_location location, stat_flags flags);

struct pipe_fds
{
    file_descriptor read_end;
    file_descriptor write_end;
};

enum class pipe_flags : int
{
    none          = 0,

    close_on_exec = O_CLOEXEC,
    direct        = O_DIRECT,
    nonblock      = O_NONBLOCK
};

pipe_flags operator|(pipe_flags a, pipe_flags b);
pipe_flags& operator|=(pipe_flags& a, pipe_flags b);

pipe_fds make_pipe(pipe_flags flags);

enum class dup_flags : int
{
    none          = 0,
    close_on_exec = O_CLOEXEC
};

void dup(int source, int target, dup_flags flags);
size_t poll_fds(pollfd* fds, size_t nfds, int timeout);
size_t poll_fds(pollfd* fds, size_t nfds);

void change_directory(char const* path);
void change_directory(std::string const& path);

enum class unlink_flags : int
{
    none      = 0,
    directory = AT_REMOVEDIR,
};

void unlink(file_location location, unlink_flags flags = unlink_flags::none);

std::vector<char> read_whole_file(file_location location);
std::unique_ptr<std::vector<char>> read_whole_file_if_exists(file_location location);
void write_whole_file(file_location location, std::vector<char> const& data);
