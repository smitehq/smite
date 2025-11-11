#pragma once
#include <string>
#include <unordered_map>
#include "file.h"

struct Dir {
    std::unordered_map<std::string, std::unique_ptr<Dir>> subdirs;
    std::unordered_map<std::string, std::unique_ptr<File>> files;
};
