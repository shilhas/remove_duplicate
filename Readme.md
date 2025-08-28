### Remove Duplicate files

Application is designed to parse input folder for duplicate files. Found duplicate files a listed and if requested can be deleted or moved to separate folder for you to look at the found duplicates and decide if you want to keep a redundant copy or delete it manually. Program uses SHA-256 to track files for duplicate.

## Build the application from source

Create a build directory by running command `mkdir build && cd build`

From build directory initialize cmake build `cmake -B . -S .. -G Ninja`

To build project run `ninja`

## Example usage:
```
./Remove_Duplicate --dir ~/Downloads/ -r --verbose --stat
```

* --dir -> Specifies the directory t search, in this example application searches for in `/$user_home/Downloads` directory for duplicate files
* -r -> Search the directory recursively
* --verbose -> Be verbose about what the application is doing.
* --stat -> provide stats related to processed data after processing is done.

for more info related to usage run command `./Remove_Duplicate --help`


## Statistical info generated include

- Number of files scanned
- file count with duplicates
- Absolute count of duplicates found
- total scanned data size
- time needed to scan
- total size of duplicate files