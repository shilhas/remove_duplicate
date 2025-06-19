#include <iostream>
#include <filesystem>
#include <fstream>
#include "remove_dup.h"
#include "argparse/argparse.hpp"

/**
 * Application to find and remove duplicate file from a given directory
 * Arguments:
 * - dir -> directory to search for duplicates
 * - limit -> max limit of duplicates to find. Default 5000
 * - r -> search recursive
 * - depth -> max depth for recursive search
 * - list -> list all found duplicate
 * - del -> delete all but one
 * - t -> limit threads to t
 * - verbose -> outputs log to the terminal
 */

#ifdef DEBUG

void print_out_args(argparse::ArgumentParser &app) {
    std::cout << "Input directory: " << app["--dir"] << std::endl;
    std::cout << "Find duplicate limit: " << app.get<int>("--limit") << std::endl;
    if (app["-r"] == true) {
        std::cout << "Search recurcively to depth:" << app["--depth"] << std::endl;
    } else {
        std::cout << "Recursive search not enabled" << std::endl;
    }
    if (app.is_used("--list")) std::cout << "List found duplicates" << std::endl;
    if (app.is_used("--del")) std::cout << "Delete found duplicates" << std::endl;
    std::cout << "Use " << app["-t"] << " threads" << std::endl;
    if (app.is_used("--mvdir")) std::cout << "Move found duplicates to :" << app["--mvdir"] << std::endl;
    if (app["-v"]==true) std::cout << "Print output verbose" << std::endl;
}
#endif

int main(int argc, char* argv[]) {

    argparse::ArgumentParser app("Remove-Duplicate");
    std::string dir_path;
    app.add_argument("--dir")
        .required()
        .help("Path of the directory to search for duplicate files")
        .store_into(dir_path);

    app.add_argument("--limit")
        .help("Maximum number of duplicates to find, default is 5000")
        .default_value(int{5000});
    
    app.add_argument("-r","--recursive")
        .help("Enable recursive search")
        .default_value(false);

    app.add_argument("--depth")
        .help("Specifies max depth incase of recursive search default is 20")
        .default_value(int{20});
    
    app.add_argument("-l","--list")
        .help("List all found duplicates")
        .default_value(false);

    app.add_argument("--del")
        .help("Delete all but one")
        .default_value(false);

    app.add_argument("-t","--threads")
        .help("number of threads to use")
        .default_value(int{1});

    app.add_argument("-v","--verbose")
        .help("List details of current processing")
        .default_value(false);

    std::string targetDir;
    app.add_argument("--mvdir")
        .help("Directory to move duplicate files to")
        .store_into(targetDir);

    try {
        app.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << app;
        return 1;
    }
    #ifdef DEBUG
    print_out_args(app);
    #endif

    std::unique_ptr<FindDup> find_dup = std::make_unique<FindDup>(
        // max_cnt=app["--limit"], 
        // max_depth=app["--depth"], 
        // recurse = app["-r"]?true:false, 
        // thread_count = app["-t"]
    );

    FindDup_path search_dir{dir_path};
    find_dup->listDup(search_dir);
    FindDup_path target_dir{targetDir};
    find_dup->moveDup(target_dir);
}