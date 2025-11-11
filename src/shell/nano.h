#ifndef NANO_H
#define NANO_H

#include <vector>
#include <string>
#include <functional>

class Nano {
public:
    // Opens an in-memory buffer for editing
    // Returns true if successfully opened and edited
    // save_callback is called when user presses Ctrl+O
    bool open(const std::string& name, 
              const std::string& content, 
              std::function<void(const std::string&)> save_callback = nullptr);

private:
    std::string filename_;
    std::vector<std::string> buffer_;
    int cursor_row_ = 0;
    int cursor_col_ = 0;
};

#endif // NANO_H