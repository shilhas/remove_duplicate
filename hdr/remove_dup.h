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

class FindDup{
    PathMap duplist;
    int max_count;
    int max_depth;
    int thread_count;
    bool recursive;
    bool show_dup;
    bool verbose;
public:
    FindDup();
    
    FindDup(int max_cnt, int max_depth, bool recurse, int thread_count, bool show_dup, bool verbose);

    FindDup_Result_t listDup(FindDup_path& dirPath);
    FindDup_Result_t delDup();
    FindDup_Result_t listOptions();
    FindDup_Result_t moveDup(FindDup_path& targetDir);
    FindDup_Result_t set_recurse_depth(int depth);
    FindDup_Result_t set_thread_count(uint32_t t_count);
    FindDup_Result_t set_max_count(int max_count);
    FindDup_Result_t set_recursive(bool recurse);

private:
    std::string CalculateSha(FindDup_path& fpath);
    void print_dups();
    FindDup_Result_t get_filename_ext(std::string& fn, 
                                               std::pair<std::string,std::string>& res);
};