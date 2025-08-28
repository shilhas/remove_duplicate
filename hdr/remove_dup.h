#pragma once

#include <unordered_map>
#include <vector>
#include <string>

using FindDup_path = std::filesystem::path;
using PathMap = std::unordered_map<std::string,std::vector<FindDup_path>>;

typedef enum {
    FAILED = 0,
    SUCCESS,
    UNKNOWN_ERROR
} FindDup_Result_t;

typedef long long unsigned scanned_size_t;

class FindDup{
    scanned_size_t scanned_size; // total size of all the files scanned
    scanned_size_t dup_size; // size of all the duplicates found 
    PathMap duplist; // data structure to hold all the scanned files and their duplicates
    int max_count; // limit number of duplicates found to this number
    int max_depth; // how deep should the application go in recursive search
    int thread_count; // number of threads to use to scan  duplicates
    int files_parsed; // count of total number of files parsed during the application execution
    int duplicate_count; // No. of files with duplicates
    int duplicate_abs_count; // Absolute number of duplicate files, i.e. if there is a single file with 2 duplicates 
                             //then the 'duplicate_abs_count' is 2 and 'duplicate_count' is 1
    bool recursive; // should the application search recursively for duplicate files
    bool verbose;   // Application prints out information related to the application execution
    bool stat; // Show statistical information
public:
    FindDup();
    
    FindDup(int max_cnt, int max_depth, bool recurse, int thread_count, bool show_stat, bool verbose);

    FindDup_Result_t listDup(FindDup_path& dirPath);
    FindDup_Result_t delDup();
    FindDup_Result_t listOptions();
    FindDup_Result_t moveDup(FindDup_path& targetDir);
    FindDup_Result_t set_recurse_depth(int depth);
    FindDup_Result_t set_thread_count(uint32_t t_count);
    FindDup_Result_t set_max_count(int max_count);
    FindDup_Result_t set_recursive(bool recurse);

private:
    // static std::string CalculateSha(FindDup_path& fpath);
    void print_dups();
    void print_stats();
    FindDup_Result_t get_filename_ext(std::string& fn, 
                                               std::pair<std::string,std::string>& res);
};