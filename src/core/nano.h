#pragma once
#include <vector>
#include <string>

class Nano {
public:
    // Opens an in-memory buffer for editing
    bool open(const std::string& name, const std::string& content);

private:
    std::string filename;
    std::vector<std::string> buffer;

    void render();
    void handle_input(int ch);

    int cursor_row_ = 0;
    int cursor_col_ = 0;
};