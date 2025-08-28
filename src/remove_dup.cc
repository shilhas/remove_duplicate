/**
 * Application to find and remove duplicate file from a given directory
 * Arguments:
 * - dir -> directory to search for duplicates
 * - max -> max duplicates to find. Default 5000
 * - r -> search recursive
 * - depth -> max depth for recursive search
 * - list -> list all found duplicate
 * - del -> delete all but one
 * - t -> limit threads to t
 * - verbose -> outputs log to the terminal
 */
#include <openssl/sha.h>
#include <random>
#include <openssl/evp.h>
#include <filesystem>
#include <fstream>
#include "remove_dup.h"
#include <queue>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <thread>
#include <future>
#include <boost/format.hpp>

namespace fs = std::filesystem;

#define FIND_DUP_BUFF_SIZE 2000000u

FindDup::FindDup():max_count(5000), max_depth(100), recursive(false), thread_count(1){}
    
FindDup::FindDup(int max_cnt, int max_depth, bool recurse, int thread_count, bool show_stat, bool verbose):
    max_count(max_cnt), 
    max_depth(max_depth), 
    recursive(recurse), 
    thread_count(thread_count),
    stat(show_stat),
    verbose(verbose),
    scanned_size(0),
    dup_size(0),
    files_parsed(0),
    duplicate_count(0),
    duplicate_abs_count(0){}

FindDup_Result_t FindDup::delDup() {
    return FindDup_Result_t::SUCCESS;
}

static std::string CalculateSha(FindDup_path fpath) {
    // Open file in binary mode
    std::ifstream rf(fpath, std::ios::binary);
    if (!rf.is_open()) {
        std::cerr << "Unable to open file " << fpath << std::endl;
        return "";
    }

    // Initialize OpenSSL context
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        std::cerr << "OpenSSL context creation error!!" << std::endl;
        return "";
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        std::cerr << "OpenSSL INIT error!!" << std::endl;
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    // Buffer for reading file
    constexpr int buf_size = FIND_DUP_BUFF_SIZE; // Ensure FIND_DUP_BUFF_SIZE is defined
    std::vector<char> buf(buf_size);

    // Read file in chunks
    while (rf.good()) {
        rf.read(buf.data(), buf_size);
        std::streamsize bytes_read = rf.gcount();
        if (bytes_read > 0) {
            if (EVP_DigestUpdate(mdctx, buf.data(), bytes_read) != 1) {
                std::cerr << "Error computing SHA256 for " << fpath << std::endl;
                EVP_MD_CTX_free(mdctx);
                return "";
            }
        }
    }

    // Check for read errors
    if (rf.bad()) {
        std::cerr << "Error reading file " << fpath << std::endl;
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    // Finalize hash
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int sha_len = 0;
    if (EVP_DigestFinal_ex(mdctx, hash, &sha_len) != 1) {
        std::cerr << "Error finalizing SHA256 for " << fpath << std::endl;
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    EVP_MD_CTX_free(mdctx);

    // Convert hash to hex string
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < sha_len; ++i) {
        ss << std::setw(2) << static_cast<int>(hash[i]);
    }
    return ss.str();
}

void FindDup::print_stats() {
    /**
     * things to print: scanned_size, dup_size, files_parsed, duplicate_count, duplicate_abs_count, 
     */
    std::cout << "\n/////////////////////////// Stats ///////////////////////////\n";
    std::cout << boost::format("%-24s: %llu\n")  % "Total files parsed " % this->files_parsed;
    std::cout << boost::format("%-24s: %llu bytes\n")  % "Scanned size " % this->scanned_size;
    std::cout << boost::format("%-24s: %llu\n")  % "Duplicate file count " % this->duplicate_count;
    std::cout << boost::format("%-24s: %llu\n")  % "Total Duplicate files " % this->duplicate_abs_count;
    std::cout << boost::format("%-24s: %llu bytes\n")  % "Duplicate size " % this->dup_size;
}

void FindDup::print_dups() {
    bool found = false;
    for (auto fileList: this->duplist) {
        std::vector<FindDup_path> files = this->duplist[fileList.first];
        if (files.size() > 1) {
            // duplicate found, add to stats
            this->duplicate_count++;
            // total duplicates = count - 1; since one is considered original
            this->duplicate_abs_count += files.size() - 1;
            found = true;
            std::cout << "Duplicate for file: " << files[0] << ":\n";
            for (int i = 1; i < files.size(); i++) {
                std::cout << "\t" << files[i]<<std::endl;
                // update stat: increment the found duplicate
                this->dup_size += fs::file_size(files[i]);
            }
            std::cout << "\n";
        }
    }
    if (!found) {
        std::cout << "No Duplicates found\n";
    }
    if (this->stat) {
        this->print_stats();
    }
}

#if 0
static void get_hashes(std::vector<std::pair<FindDup_path,std::future<std::string>>> fu_hashes) {
    for (auto fu_h: fu_hashes) {
        std::string f_hash = fu_h.second.get();
        if (f_hash == "") {
            return FindDup_Result_t::FAILED;
        }
        this->duplist[f_hash].push_back(fu_h.first);
    }
    fu_hashes = {};
}
#endif

FindDup_Result_t FindDup::listDup(FindDup_path& dirPath) {
    // upto specified depth in the target directory search for duplicates
    std::queue<FindDup_path>dir_queue;
    dir_queue.push(dirPath);
    int recurse_depth = FindDup::recursive?FindDup::max_depth:1;
    while(!dir_queue.empty() and recurse_depth) {
        std::queue<FindDup_path> next_dir_queue;
        int cur_queue_size = dir_queue.size();
        for (int i = 0; i < cur_queue_size; ++i) {
            FindDup_path cur_path = dir_queue.front();
            dir_queue.pop();
            std::vector<std::pair<FindDup_path,std::future<std::string>>> fu_hashes;
            for (const auto next: fs::directory_iterator(cur_path, fs::directory_options::skip_permission_denied)) {
                if (fs::is_directory(next)) {
                    next_dir_queue.push(next);
                } else {
                    // it is a file
                    // update stats
                    this->files_parsed++;
                    this->scanned_size += fs::file_size(next);
                    FindDup_path f_path = next.path();
                    if (this->verbose) {
                        std::cout << "Processing:" << f_path << "\n";
                    }
                    
                    std::future<std::string> fu_hash = std::async(CalculateSha, f_path);
                    fu_hashes.push_back(make_pair(f_path,std::move(fu_hash)));
                }
                if (fu_hashes.size() >= this->thread_count) {
                    // get_hashes(std::move(fu_hashes))
                    for (int i = 0; i < fu_hashes.size(); i++) {
                        std::pair<FindDup_path,std::future<std::string>> fu_h = std::move(fu_hashes[i]);
                        std::string f_hash = fu_h.second.get();
                        if (f_hash == "") {
                            return FindDup_Result_t::FAILED;
                        }
                        this->duplist[f_hash].push_back(fu_h.first);
                    }
                    fu_hashes.clear();
                }
            }
            if (!fu_hashes.empty()) {
                // get_hashes(std::move(fu_hashes))
                for (int i = 0; i < fu_hashes.size(); i++) {
                    std::pair<FindDup_path,std::future<std::string>> fu_h = std::move(fu_hashes[i]);
                    std::string f_hash = fu_h.second.get();
                    if (f_hash == "") {
                        return FindDup_Result_t::FAILED;
                    }
                    this->duplist[f_hash].push_back(fu_h.first);
                }
                fu_hashes.clear();
            }
        }
        dir_queue = std::move(next_dir_queue);
        recurse_depth--;
    }

    // List each found duplicates
    this->print_dups();
    return FindDup_Result_t::SUCCESS;
}

FindDup_Result_t FindDup::listOptions() {
    return FindDup_Result_t::SUCCESS;
}

FindDup_Result_t FindDup::get_filename_ext(std::string& fn, std::pair<std::string,std::string>& res){
    int i = fn.size()-1;
    for (; i >= 0; i--) {
        if ('.' == fn[i]) {
            break;
        }
    }
    if (i < 0) {
        return FindDup_Result_t::FAILED;
    }
    res.first = fn.substr(0,i);
    res.second = fn.substr(i+1);
    return FindDup_Result_t::SUCCESS;
}

FindDup_Result_t FindDup::moveDup(FindDup_path& targetDir) {
    if (!fs::is_directory(targetDir)) {
        if (!fs::exists(targetDir)) {
            std::cout << "Target directory doesn't exist creating new directory\n";
            fs::create_directory(targetDir);
        } else {
            std::cerr << "File with directory name exists!\n";
            return FindDup_Result_t::FAILED;
        }
    }
    for (auto dupList: this->duplist) {
        for (int i = 1; i < dupList.second.size(); i++) {
            std::string fn = dupList.second[i].filename().c_str();
            if (fs::exists(targetDir / fn)) {
                std::random_device rd; 
                std::mt19937 gen(rd()); 
                std::uniform_int_distribution<> distr(458970, 2000000000);
                std::pair<std::string,std::string>fn_ext;
                if (FindDup_Result_t::FAILED == this->get_filename_ext(fn, fn_ext)) {
                    return FindDup_Result_t::FAILED;
                }
                std::stringstream ss;
                ss << fn_ext.first << (distr(gen)) <<"." << fn_ext.second;
                fn = ss.str();
                std::cout << "File moved with new name " << fn << std::endl;
            }
            fs::rename(dupList.second[i], targetDir/fn);
        }
    }
    return FindDup_Result_t::SUCCESS;
}

FindDup_Result_t FindDup::set_recurse_depth(int depth){
    this->max_depth = depth;
    if (depth > 1)  this->recursive = true;
    return FindDup_Result_t::SUCCESS;
}
FindDup_Result_t FindDup::set_thread_count(uint32_t t_count) {
    t_count = std::min(t_count, std::thread::hardware_concurrency());
    this->thread_count = t_count;
    return FindDup_Result_t::SUCCESS;
}

FindDup_Result_t FindDup::set_max_count(int max_count) {
    this->max_count = max_count;
    return FindDup_Result_t::SUCCESS;
}

