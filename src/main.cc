#include <iostream>
#include <filesystem>
#include <fstream>
#include "remove_dup.h"
#include "argparse/argparse.hpp"
#include <openssl/opensslv.h>
#include <openssl/crypto.h>

void InitializeOpenSSL() {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    // For OpenSSL < 1.1.0, set up thread callbacks
    CRYPTO_set_locking_callback([](int mode, int type, const char*, int) {
        static std::mutex mutexes[CRYPTO_NUM_LOCKS];
        if (mode & CRYPTO_LOCK) {
            mutexes[type].lock();
        } else {
            mutexes[type].unlock();
        }
    });
    CRYPTO_set_id_callback([]() -> unsigned long {
        return static_cast<unsigned long>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    });
#endif
    // For OpenSSL >= 1.1.0, thread safety is built-in, no callbacks needed
}

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

 typedef struct{
    std::string dir_path;
    std::string move_dir;
    int limit;
    int threads;
    int depth;
    bool recursive;
    bool del_duplicates;
    bool verbose;
    bool move;
    bool stat;
}Arguments_t;

#ifdef DEBUG

void print_out_args (Arguments_t &arg) {
    std::cout << "Input directory: " << arg.dir_path << std::endl;
    std::cout << "Find duplicate limit: " << arg.limit << std::endl;
    if (arg.recursive == true) {
        std::cout << "Search recurcively to depth:" << arg.depth << std::endl;
    } else {
        std::cout << "Recursive search not enabled" << std::endl;
    }
    if (arg.stat) std::cout << "Print statistical information" << std::endl;
    if (arg.del_duplicates) std::cout << "Delete found duplicates" << std::endl;
    std::cout << "Use " << arg.threads << " threads" << std::endl;
    if (arg.move_dir != "") std::cout << "Move found duplicates to :" << arg.move_dir << std::endl;
    if (arg.verbose) std::cout << "Print output verbose" << std::endl;
}
#endif

int main(int argc, char* argv[]) {

    argparse::ArgumentParser app("Remove-Duplicate");
    Arguments_t args;

    app.add_argument("--dir")
        .required()
        .help("Path of the directory to search for duplicate files")
        .store_into(args.dir_path);

    app.add_argument("--limit")
        .help("Maximum number of duplicates to find, default is 5000")
        .default_value(int{5000})
        .store_into(args.limit);
    
    app.add_argument("-r","--recursive")
        .help("Enable recursive search")
        .default_value(false)
        .implicit_value(true)
        .store_into(args.recursive);

    app.add_argument("--stat")
        .help("After completion provide stats")
        .default_value(false)
        .implicit_value(true)
        .store_into(args.stat);

    app.add_argument("--depth")
        .help("Specifies max depth incase of recursive search default is 20")
        .default_value(int{20})
        .store_into(args.depth);

    app.add_argument("--del")
        .help("Delete all but one")
        .default_value(false)
        .implicit_value(true)
        .store_into(args.del_duplicates);

    app.add_argument("-t","--threads")
        .help("number of threads to use")
        .default_value(int{1})
        .store_into(args.threads);

    app.add_argument("--verbose")
        .help("List details of current processing")
        .implicit_value(true)
        .default_value(false)
        .store_into(args.verbose);

    app.add_argument("--mvdir")
        .help("Directory to move duplicate files to")
        .default_value("")
        .store_into(args.move_dir);

    try {
        app.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << app;
        return 1;
    }
    #ifdef DEBUG
    print_out_args(args);
    #endif

    std::unique_ptr<FindDup> find_dup = std::make_unique<FindDup>(
        args.limit,
        args.depth,
        args.recursive,
        args.threads,
        args.stat,
        args.verbose
    );
    InitializeOpenSSL();
    FindDup_path search_dir{args.dir_path};
    std::cout << "Searching in directory: "<< args.dir_path << std::endl;
    find_dup->listDup(search_dir);
    if (args.move_dir != ""){
        FindDup_path target_dir{args.move_dir};
        find_dup->moveDup(target_dir);
    }
}
