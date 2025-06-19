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

namespace fs = std::filesystem;

FindDup::FindDup():max_count(5000), max_depth(100), recursive(false), thread_count(1){}
    
FindDup::FindDup(int max_cnt, int max_depth, bool recurse, int thread_count):
    max_count(max_cnt), max_depth(max_depth), recursive(recurse), thread_count(thread_count){}

FindDup_Result_t FindDup::delDup() {
    return FindDup_Result_t::SUCCESS;
}

std::string FindDup::CalculateSha(FindDup_path& fpath) {
    std::ifstream rf(fpath, std::ios::binary);
    if(!rf.is_open()) {
        // Unable to open file
        std::cerr << "Unable to open file " << fpath << std::endl;
        return "";
    }
    EVP_MD_CTX *mdctx;

    if((mdctx = EVP_MD_CTX_new()) == NULL) {
        std::cerr << "OpenSSL context creation error!!" << std::endl;
        return "";
    }

    if(1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL)){
        std::cerr << "OpenSSL INIT error!!" << std::endl;
        return "";
    }


    // Buffer
    const int buf_size = 4096;
    std::vector<char>buf(buf_size);

    while (rf.read(buf.data(), buf_size)) {
        if (1 != EVP_DigestUpdate(mdctx, buf.data(), rf.gcount())) {
            std::cerr << "Error computing SHA256!!\n" ;
        }
    }
    if (1 != EVP_DigestUpdate(mdctx, buf.data(), rf.gcount())) {
        std::cerr << "Error computing SHA256!!\n" ;
    }
    unsigned int sha_len = 32;

    unsigned char hash[sha_len];
    EVP_DigestFinal_ex(mdctx, hash, &sha_len);
    EVP_MD_CTX_free(mdctx);
    std::ostringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

void FindDup::print_dups() {
    bool found = false;
    for (auto fileList: this->duplist) {
        std::vector<FindDup_path> files = this->duplist[fileList.first];
        if (files.size() > 1) {
            found = true;
            std::cout << "Duplicate for file: " << files[0] << ":\n";
            for (int i = 1; i < files.size(); i++) {
                std::cout << "\t" << files[i]<<std::endl;
            }
            std::cout << "\n";
        }
    }
    if (!found) {
        std::cout << "No Duplicates found\n";
    }
}

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
            for (const auto next: fs::directory_iterator(cur_path)) {
                if (fs::is_directory(next)) {
                    next_dir_queue.push(next);
                } else {
                    // it is a file
                    FindDup_path f_path = next.path();
                    std::string f_hash = this->CalculateSha(f_path);
                    if (f_hash == "") {
                        return FindDup_Result_t::FAILED;
                    }
                    this->duplist[f_hash].push_back(f_path);
                }
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


