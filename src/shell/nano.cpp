#include "nano.h"
#include <ncurses/ncurses.h>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstddef>
#include <functional>
#include <memory>
#include <iostream>

// RAII wrapper for ncurses windows
class NCursesWindow {
public:
    NCursesWindow(int height, int width, int y, int x) 
        : win_(newwin(height, width, y, x)) {
        if (!win_) {
            throw std::runtime_error("Failed to create ncurses window");
        }
    }

    ~NCursesWindow() {
        if (win_) {
            delwin(win_);
        }
    }

    // Delete copy operations
    NCursesWindow(const NCursesWindow&) = delete;
    NCursesWindow& operator=(const NCursesWindow&) = delete;

    // Allow move operations
    NCursesWindow(NCursesWindow&& other) noexcept : win_(other.win_) {
        other.win_ = nullptr;
    }

    NCursesWindow& operator=(NCursesWindow&& other) noexcept {
        if (this != &other) {
            if (win_) delwin(win_);
            win_ = other.win_;
            other.win_ = nullptr;
        }
        return *this;
    }

    operator WINDOW*() { return win_; }
    WINDOW* get() { return win_; }

private:
    WINDOW* win_;
};

// RAII wrapper for ncurses initialization
class NCursesSession {
public:
    NCursesSession() {
        initscr();
        raw();
        keypad(stdscr, TRUE);
        noecho();
        curs_set(1);

        if (has_colors()) {
            start_color();
            init_pair(1, COLOR_WHITE, COLOR_BLACK);  // buffer
            init_pair(2, COLOR_BLACK, COLOR_WHITE);  // footer
            init_pair(3, COLOR_BLACK, COLOR_WHITE);  // header
        }
    }

    ~NCursesSession() {
        endwin();
    }

    // Delete copy and move
    NCursesSession(const NCursesSession&) = delete;
    NCursesSession& operator=(const NCursesSession&) = delete;
    NCursesSession(NCursesSession&&) = delete;
    NCursesSession& operator=(NCursesSession&&) = delete;
};

bool Nano::open(const std::string& name, const std::string& content, 
                std::function<void(const std::string&)> save_callback) {
    
    // Validate input
    if (name.empty()) {
        std::cerr << "Error: Empty filename\n";
        return false;
    }

    // Limit content size (5MB max)
    const size_t MAX_CONTENT_SIZE = 5 * 1024 * 1024;
    if (content.size() > MAX_CONTENT_SIZE) {
        std::cerr << "Error: Content too large (max 5MB)\n";
        return false;
    }

    filename_ = name;
    buffer_.clear();
    
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        buffer_.push_back(line);
    }
    if (buffer_.empty()) {
        buffer_.push_back("");
    }

    try {
        // RAII ensures ncurses cleanup on any exception
        NCursesSession session;

        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);

        // Create windows with RAII - automatic cleanup
        NCursesWindow header_win(1, max_x, 0, 0);
        NCursesWindow text_win(max_y - 2, max_x, 1, 0);
        NCursesWindow footer_win(1, max_x, max_y - 1, 0);

        wbkgd(text_win.get(), COLOR_PAIR(1));
        keypad(text_win.get(), TRUE);

        // Draw header
        wattron(header_win.get(), COLOR_PAIR(3) | A_BOLD);
        mvwprintw(header_win.get(), 0, 0, " GNU nano - editing %s ", filename_.c_str());
        wattroff(header_win.get(), COLOR_PAIR(3) | A_BOLD);
        wrefresh(header_win.get());

        // Draw footer
        auto draw_footer = [&](const std::string& msg = "") {
            werase(footer_win.get());
            wattron(footer_win.get(), COLOR_PAIR(2) | A_REVERSE);
            if (msg.empty()) {
                mvwprintw(footer_win.get(), 0, 0, "[^O] Save  [^X] Exit");
            } else {
                mvwprintw(footer_win.get(), 0, 0, "%s", msg.c_str());
            }
            wattroff(footer_win.get(), COLOR_PAIR(2) | A_REVERSE);
            wrefresh(footer_win.get());
        };

        draw_footer();

        cursor_row_ = 0;
        cursor_col_ = 0;
        bool running = true;

        while (running) {
            // Draw buffer
            werase(text_win.get());
            int text_height = max_y - 2;
            size_t visible_lines = std::min(buffer_.size(), static_cast<size_t>(text_height));
            
            for (size_t i = 0; i < visible_lines; ++i) {
                mvwprintw(text_win.get(), i, 0, "%s", buffer_[i].c_str());
            }

            // Bounds check cursor
            if (cursor_row_ >= static_cast<int>(buffer_.size())) {
                cursor_row_ = static_cast<int>(buffer_.size()) - 1;
            }
            if (cursor_row_ < 0) cursor_row_ = 0;
            
            if (cursor_col_ > static_cast<int>(buffer_[cursor_row_].size())) {
                cursor_col_ = static_cast<int>(buffer_[cursor_row_].size());
            }
            if (cursor_col_ < 0) cursor_col_ = 0;

            wmove(text_win.get(), cursor_row_, cursor_col_);
            wrefresh(text_win.get());

            int ch = wgetch(text_win.get());
            
            switch (ch) {
                case 24: // Ctrl+X
                    running = false;
                    break;

                case 15: { // Ctrl+O
                    std::ostringstream oss;
                    for (const auto& l : buffer_) {
                        oss << l << "\n";
                    }

                    if (save_callback) {
                        save_callback(oss.str());
                    }

                    draw_footer("File saved: " + filename_);
                    napms(800);
                    draw_footer();
                    break;
                }

                case KEY_UP:
                    if (cursor_row_ > 0) {
                        cursor_row_--;
                        cursor_col_ = std::min(cursor_col_, 
                                              static_cast<int>(buffer_[cursor_row_].size()));
                    }
                    break;

                case KEY_DOWN:
                    if (cursor_row_ < static_cast<int>(buffer_.size()) - 1) {
                        cursor_row_++;
                        cursor_col_ = std::min(cursor_col_, 
                                              static_cast<int>(buffer_[cursor_row_].size()));
                    }
                    break;

                case KEY_LEFT:
                    if (cursor_col_ > 0) {
                        cursor_col_--;
                    } else if (cursor_row_ > 0) {
                        cursor_row_--;
                        cursor_col_ = static_cast<int>(buffer_[cursor_row_].size());
                    }
                    break;

                case KEY_RIGHT:
                    if (cursor_col_ < static_cast<int>(buffer_[cursor_row_].size())) {
                        cursor_col_++;
                    } else if (cursor_row_ < static_cast<int>(buffer_.size()) - 1) {
                        cursor_row_++;
                        cursor_col_ = 0;
                    }
                    break;

                case 10: // Enter
                case KEY_ENTER:
                    buffer_.insert(buffer_.begin() + cursor_row_ + 1,
                                  buffer_[cursor_row_].substr(cursor_col_));
                    buffer_[cursor_row_] = buffer_[cursor_row_].substr(0, cursor_col_);
                    cursor_row_++;
                    cursor_col_ = 0;
                    break;

                case KEY_BACKSPACE:
                case 127:
                case 8:
                    if (cursor_col_ > 0) {
                        buffer_[cursor_row_].erase(cursor_col_ - 1, 1);
                        cursor_col_--;
                    } else if (cursor_row_ > 0) {
                        cursor_col_ = static_cast<int>(buffer_[cursor_row_ - 1].size());
                        buffer_[cursor_row_ - 1] += buffer_[cursor_row_];
                        buffer_.erase(buffer_.begin() + cursor_row_);
                        cursor_row_--;
                    }
                    break;

                default:
                    if (ch >= 32 && ch < 127) {
                        buffer_[cursor_row_].insert(cursor_col_, 1, static_cast<char>(ch));
                        cursor_col_++;
                    }
                    break;
            }
        }

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error in nano editor: " << e.what() << "\n";
        return false;
    }
}