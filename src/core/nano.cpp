#include "nano.h"
#include <ncurses/ncurses.h>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstddef>

bool Nano::open(const std::string& name, const std::string& content) {
    filename = name;
    buffer.clear();
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) buffer.push_back(line);
    if (buffer.empty()) buffer.push_back("");

    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(1);

    int row = 0, col = 0;
    bool running = true;

    while (running) {
        render();
        move(row + 1, col);

        int ch = getch();
        switch (ch) {
            case 24: // Ctrl+X
                running = false;
                break;

            case 15: { // Ctrl+O
                std::string content;
                for (auto& l : buffer) content += l + "\n";
                //save_file(path, content);
                mvprintw(LINES - 2, 0, "File saved: %s", filename.c_str());
                refresh();
                napms(500);
                break;
            }

            case KEY_UP:
                if (row > 0) row--;
                if (col > (int)buffer[row].size()) col = buffer[row].size();
                break;

            case KEY_DOWN:
                if (row < (int)buffer.size() - 1) row++;
                if (col > (int)buffer[row].size()) col = buffer[row].size();
                break;

            case KEY_LEFT:
                if (col > 0) col--;
                else if (row > 0) { row--; col = buffer[row].size(); }
                break;

            case KEY_RIGHT:
                if (col < (int)buffer[row].size()) col++;
                else if (row < (int)buffer.size() - 1) { row++; col = 0; }
                break;

            case 10: // Enter
                buffer.insert(buffer.begin() + row + 1, buffer[row].substr(col));
                buffer[row] = buffer[row].substr(0, col);
                row++; col = 0;
                break;

            case KEY_BACKSPACE:
            case 127:
                if (col > 0) {
                    buffer[row].erase(col - 1, 1);
                    col--;
                } else if (row > 0) {
                    col = buffer[row - 1].size();
                    buffer[row - 1] += buffer[row];
                    buffer.erase(buffer.begin() + row);
                    row--;
                }
                break;

            default:
                if (ch >= 32 && ch < 127) { // printable
                    buffer[row].insert(col, 1, (char)ch);
                    col++;
                }
                break;
        }
    }

    endwin();
    return true;
}

void Nano::render() {
    clear();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // Initialize color if not done yet
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLACK); // normal text
        init_pair(2, COLOR_BLACK, COLOR_WHITE); // footer
        init_pair(3, COLOR_BLACK, COLOR_CYAN);  // header
    }

    // Draw header (line 0)
    attron(COLOR_PAIR(3) | A_BOLD);
    mvhline(0, 0, ' ', max_x); // fill entire line
    mvprintw(0, 0, " GNU nano - editing %s ", filename.c_str());
    attroff(COLOR_PAIR(3) | A_BOLD);

    // Draw text buffer starting at line 1
    for (int i = 0; i < (int)buffer.size() && i < max_y - 2; ++i) {
        mvprintw(i + 1, 0, "%s", buffer[i].c_str());
    }

    // Draw footer
    attron(COLOR_PAIR(2) | A_REVERSE);
    mvprintw(max_y - 1, 0, "[^O] Save  [^X] Exit");
    attroff(COLOR_PAIR(2) | A_REVERSE);

    refresh();
}