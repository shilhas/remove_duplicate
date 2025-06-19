#include "remove_dup.h"


int main(int argc, char* argv[]) {
    std::string dir_path = "/home/shilhas/test/test_files";//fs::current_path();
    FindDup_path targetDir{"/home/shilhas/test/moved"};
    
    FindDup find_dup;
    
    FindDup_path search_dir{dir_path};
    find_dup.listDup(search_dir);

    find_dup.moveDup(targetDir);
}