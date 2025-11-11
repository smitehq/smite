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
        
        int text_height = max_y - 2;  // Reserve space for header and footer

        // Create windows with RAII - automatic cleanup
        NCursesWindow header_win(1, max_x, 0, 0);
        NCursesWindow text_win(text_height, max_x, 1, 0);
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
        int top_line = 0;  // First visible line in the viewport
        bool running = true;

        while (running) {
            // Bounds check cursor (logical position in buffer)
            if (cursor_row_ >= static_cast<int>(buffer_.size())) {
                cursor_row_ = static_cast<int>(buffer_.size()) - 1;
            }
            if (cursor_row_ < 0) cursor_row_ = 0;
            
            if (cursor_col_ > static_cast<int>(buffer_[cursor_row_].size())) {
                cursor_col_ = static_cast<int>(buffer_[cursor_row_].size());
            }
            if (cursor_col_ < 0) cursor_col_ = 0;

            // Adjust viewport to keep cursor visible
            // If cursor moved above the viewport, scroll up
            if (cursor_row_ < top_line) {
                top_line = cursor_row_;
            }
            
            // If cursor moved below the viewport, scroll down
            if (cursor_row_ >= top_line + text_height) {
                top_line = cursor_row_ - text_height + 1;
            }
            
            // Clamp top_line to valid range
            if (top_line < 0) {
                top_line = 0;
            }
            
            // Only clamp max if we have more lines than viewport
            if (static_cast<int>(buffer_.size()) > text_height) {
                int max_top_line = static_cast<int>(buffer_.size()) - text_height;
                if (top_line > max_top_line) {
                    top_line = max_top_line;
                }
            }

            // Draw visible portion of buffer
            werase(text_win.get());
            for (int i = 0; i < text_height; ++i) {
                int buffer_line = top_line + i;
                if (buffer_line >= static_cast<int>(buffer_.size())) break;
                
                // Safely print line (handle long lines)
                const std::string& line_text = buffer_[buffer_line];
                if (line_text.size() > static_cast<size_t>(max_x)) {
                    // Truncate very long lines to prevent buffer overflow
                    std::string truncated = line_text.substr(0, max_x - 1);
                    mvwprintw(text_win.get(), i, 0, "%s", truncated.c_str());
                } else {
                    mvwprintw(text_win.get(), i, 0, "%s", line_text.c_str());
                }
            }

            // Calculate screen cursor position (relative to viewport)
            int screen_row = cursor_row_ - top_line;
            int screen_col = cursor_col_;
            
            // Ensure cursor is within screen bounds
            if (screen_row >= 0 && screen_row < text_height) {
                wmove(text_win.get(), screen_row, screen_col);
            }
            
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

                case KEY_HOME:  // Home key - go to start of line
                case 1:         // Ctrl+A
                    cursor_col_ = 0;
                    break;

                case KEY_END:   // End key - go to end of line
                case 5:         // Ctrl+E
                    cursor_col_ = static_cast<int>(buffer_[cursor_row_].size());
                    break;

                case KEY_PPAGE: // Page Up
                    cursor_row_ -= text_height;
                    if (cursor_row_ < 0) cursor_row_ = 0;
                    cursor_col_ = std::min(cursor_col_, 
                                          static_cast<int>(buffer_[cursor_row_].size()));
                    break;

                case KEY_NPAGE: // Page Down
                    cursor_row_ += text_height;
                    if (cursor_row_ >= static_cast<int>(buffer_.size())) {
                        cursor_row_ = static_cast<int>(buffer_.size()) - 1;
                    }
                    cursor_col_ = std::min(cursor_col_, 
                                          static_cast<int>(buffer_[cursor_row_].size()));
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

                case KEY_DC:    // Delete key
                case 4:         // Ctrl+D
                    if (cursor_col_ < static_cast<int>(buffer_[cursor_row_].size())) {
                        buffer_[cursor_row_].erase(cursor_col_, 1);
                    } else if (cursor_row_ < static_cast<int>(buffer_.size()) - 1) {
                        buffer_[cursor_row_] += buffer_[cursor_row_ + 1];
                        buffer_.erase(buffer_.begin() + cursor_row_ + 1);
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