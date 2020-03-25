#include <cstring>
#include <iostream>

void add_source_file_command(size_t argc, char* argv[]);
void init_command(size_t argc, char* argv[]);
void md5sum_command(size_t argc, char* argv[]);
void get_source_files(size_t argc, char* argv[]);

int main(int argc, char* argv[])
{
    try
    {
        --argc;
        ++argv;
    
        if (argc == 0)
        {
            std::cerr << "subcommand expected\n";
            return 1;
        }
        
        if (!strcmp(*argv, "add_source_file"))
        {
            --argc;
            ++argv;
            add_source_file_command(argc, argv);
        }
        else if (!strcmp(*argv, "init"))
        {
            --argc;
            ++argv;
            init_command(argc, argv);
        }
        else if (!strcmp(*argv, "md5sum"))
        {
            --argc;
            ++argv;
            md5sum_command(argc, argv);
        }
        else if (!strcmp(*argv, "list_source_files"))
        {
            --argc;
            ++argv;
            get_source_files(argc, argv);
        }
        else
        {
            std::cerr << "unknown subcommand\n";
        }
    }
    catch (std::exception const& e)
    {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}
