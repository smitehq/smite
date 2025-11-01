#ifndef MODULES_LINUX_MODULE_H
#define MODULES_LINUX_MODULE_H

#include <string>
#include <map>
#include <memory>

struct File {
    std::string content;
    std::string perms = "rw-r--r--";
};

struct Dir {
    std::map<std::string, std::unique_ptr<Dir>> subdirs;
    std::map<std::string, std::unique_ptr<File>> files;
};

#endif