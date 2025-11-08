#include "nano.h"
#include <ncurses/ncurses.h>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstddef>
#include <functional>

bool Nano::open(const std::string& name, const std::string& content, std::function<void(const std::string&)> save_callback) {
    filename = name;
    buffer.clear();
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) buffer.push_back(line);
    if (buffer.empty()) buffer.push_back("");

    // Initialize ncurses
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(1);

    if (has_colors()) start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);  // buffer
    init_pair(2, COLOR_BLACK, COLOR_WHITE);  // footer
    init_pair(3, COLOR_BLACK, COLOR_WHITE);   // header

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // Persistent windows
    WINDOW* header_win = newwin(1, max_x, 0, 0);
    WINDOW* text_win   = newwin(max_y - 2, max_x, 1, 0);
    WINDOW* footer_win = newwin(1, max_x, max_y - 1, 0);

    wbkgd(text_win, COLOR_PAIR(1));
    keypad(text_win, TRUE);

    // Draw header/footer once
    wattron(header_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(header_win, 0, 0, " GNU nano - editing %s ", filename.c_str());
    wattroff(header_win, COLOR_PAIR(3) | A_BOLD);
    wrefresh(header_win);

    wattron(footer_win, COLOR_PAIR(2) | A_REVERSE);
    mvwprintw(footer_win, 0, 0, "[^O] Save  [^X] Exit");
    wattroff(footer_win, COLOR_PAIR(2) | A_REVERSE);
    wrefresh(footer_win);

    cursor_row_ = 0;
    cursor_col_ = 0;
    bool running = true;

    while (running) {
        // Draw buffer (update only buffer window)
        werase(text_win);
        int text_height = max_y - 2;
        for (int i = 0; i < (int)buffer.size() && i < text_height; ++i) {
            mvwprintw(text_win, i, 0, "%s", buffer[i].c_str());
        }

        wmove(text_win, cursor_row_, cursor_col_);

        // Use overlay instead of wrefresh to avoid affecting parent screen
        overlay(text_win, stdscr);
        doupdate();

        int ch = wgetch(text_win);
        switch (ch) {
            case 24: // Ctrl+X
                running = false;
                break;

            case 15: { // Ctrl+O
                std::string out;
                for (auto& l : buffer) out += l + "\n";

                // call the callback if provided
                if (save_callback) save_callback(out);

                mvwprintw(footer_win, 0, 0, "File saved: %s", filename.c_str());
                wclrtoeol(footer_win);
                wrefresh(footer_win);

                // Wait briefly so user sees message
                napms(500);

                // redraw static footer
                mvwprintw(footer_win, 0, 0, "[^O] Save  [^X] Exit");
                wclrtoeol(footer_win);
                wrefresh(footer_win);
                break;
            }

            case KEY_UP:
                if (cursor_row_ > 0) cursor_row_--;
                if (cursor_col_ > (int)buffer[cursor_row_].size())
                    cursor_col_ = buffer[cursor_row_].size();
                break;

            case KEY_DOWN:
                if (cursor_row_ < (int)buffer.size() - 1) cursor_row_++;
                if (cursor_col_ > (int)buffer[cursor_row_].size())
                    cursor_col_ = buffer[cursor_row_].size();
                break;

            case KEY_LEFT:
                if (cursor_col_ > 0) cursor_col_--;
                else if (cursor_row_ > 0) {
                    cursor_row_--;
                    cursor_col_ = buffer[cursor_row_].size();
                }
                break;

            case KEY_RIGHT:
                if (cursor_col_ < (int)buffer[cursor_row_].size()) cursor_col_++;
                else if (cursor_row_ < (int)buffer.size() - 1) {
                    cursor_row_++;
                    cursor_col_ = 0;
                }
                break;

            case 10: // Enter
                buffer.insert(buffer.begin() + cursor_row_ + 1,
                              buffer[cursor_row_].substr(cursor_col_));
                buffer[cursor_row_] = buffer[cursor_row_].substr(0, cursor_col_);
                cursor_row_++;
                cursor_col_ = 0;
                break;

            case KEY_BACKSPACE:
            case 127:
                if (cursor_col_ > 0) {
                    buffer[cursor_row_].erase(cursor_col_ - 1, 1);
                    cursor_col_--;
                } else if (cursor_row_ > 0) {
                    cursor_col_ = buffer[cursor_row_ - 1].size();
                    buffer[cursor_row_ - 1] += buffer[cursor_row_];
                    buffer.erase(buffer.begin() + cursor_row_);
                    cursor_row_--;
                }
                break;

            default:
                if (ch >= 32 && ch < 127) {
                    buffer[cursor_row_].insert(cursor_col_, 1, (char)ch);
                    cursor_col_++;
                }
                break;
        }
    }

    delwin(text_win);
    delwin(header_win);
    delwin(footer_win);
    endwin();
    return true;
}
