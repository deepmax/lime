#include "parser.h"
#include "vm.h"
#include <stdint.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>


void print_help_compiler()
{
    fprintf(stderr, "Usage: lime --c [--stdin] [--dasm <file>] [--exec|--gen <file>] [<file.lm>]\n");
    fprintf(stderr, "  --stdin    Read code from stdin instead of a file\n");
    fprintf(stderr, "  --dasm     Write disassembly to file\n");
    fprintf(stderr, "  --exec     Compile and execute\n");
    fprintf(stderr, "  --gen      Generate bytecode to file\n");
}

void print_help_executor()
{
    fprintf(stderr, "Usage: lime --x [file.lmx]\n");
    fprintf(stderr, "  Loads and executes bytecode file\n");
}

void print_help()
{
    print_help_compiler();
    print_help_executor();
}

int main_compiler(int argc, char *argv[])
{
    int opt;
    int use_stdin = 0;
    int exec_flag = 0;
    int gen_flag = 0;
    char* dasm_filename = NULL;
    char* output_filename = NULL;

    static struct option long_options[] = {
        {"stdin", no_argument, 0, 's'},
        {"dasm", required_argument, 0, 'd'},
        {"exec", no_argument, 0, 'e'},
        {"gen", required_argument, 0, 'g'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "", long_options, NULL)) != -1)
    {
        switch (opt)
        {
        case 's':
            use_stdin = 1;
            break;
        case 'd':
            dasm_filename = optarg;
            break;
        case 'e':
            exec_flag = 1;
            break;
        case 'g':
            gen_flag = 1;
            output_filename = optarg;
            break;
        default:
            print_help_compiler();
            return 1;
        }
    }

    if (exec_flag && gen_flag)
    {
        fprintf(stderr, "Error: Cannot specify both --exec and --gen\n");
        return 1;
    }

    if (use_stdin)
    {
        if (optind < argc)
        {
            fprintf(stderr, "Error: Cannot specify both --stdin and a filename\n");
            return 1;
        }
        parser_load_stdin();
    }
    else if (optind < argc)
    {
        parser_load_file(argv[optind]);
    }
    else
    {
        print_help_compiler();
        return 1;
    }

    vm_init();

    parser_parse();
    parser_free();

    if (dasm_filename)
    {
        vm_dasm(dasm_filename);
    }

    if (gen_flag)
    {
        if (!output_filename)
        {
            fprintf(stderr, "Error: --gen requires an output filename\n");
            return 1;
        }
        vm_save(output_filename);
    }

    if (exec_flag)
    {
        vm_exec();
    }

    return 0;
}

int main_executor(int argc, char *argv[])
{
    char* bytecode_file = NULL;

    if (optind < argc)
    {
        bytecode_file = argv[optind];
    }
    else
    {
        print_help_executor();
        return 1;
    }

    vm_init();
    vm_load(bytecode_file);
    vm_exec();

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_help();
        return 1;
    }

    // Check for mode: --c (compiler) or --x (executor)
    if (strcmp(argv[1], "--c") == 0)
    {
        // Compiler mode: skip --c and process remaining arguments
        return main_compiler(argc - 1, argv + 1);
    }
    else if (strcmp(argv[1], "--x") == 0)
    {
        // Executor mode: skip --x and process remaining arguments
        optind = 2; // Set optind to skip --x
        return main_executor(argc, argv);
    }
    else
    {
        print_help();
        return 1;
    }
}
