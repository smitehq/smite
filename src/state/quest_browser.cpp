#include "quest_browser.h"
#include "core/router.h"
#include <ncurses/ncurses.h>
#include <algorithm>
#include <sstream>

QuestBrowser::QuestBrowser(const QuestManager& quest_mgr, Router& router)
    : quest_mgr_(quest_mgr), router_(router) {}

std::string QuestBrowser::difficulty_to_string(QuestDifficulty diff) const {
    switch (diff) {
        case QuestDifficulty::BEGINNER: return "Beginner";
        case QuestDifficulty::INTERMEDIATE: return "Intermediate";
        case QuestDifficulty::ADVANCED: return "Advanced";
        case QuestDifficulty::EXPERT: return "Expert";
        default: return "Unknown";
    }
}

int QuestBrowser::get_difficulty_count(const ModuleInfo& mod, QuestDifficulty diff) const {
    auto it = mod.quests_by_difficulty.find(diff);
    return (it != mod.quests_by_difficulty.end()) ? it->second.size() : 0;
}

std::vector<QuestBrowser::ModuleInfo> QuestBrowser::gather_module_info() {
    std::vector<ModuleInfo> modules;

    for (const auto& mod : router_.get_modules()) {
        auto quests = quest_mgr_.get_quests_for_module(mod->name());
        if (quests.empty()) continue;

        ModuleInfo info;
        info.name = mod->name();
        info.total_quests = quests.size();

        // Group quests by difficulty (store by value, not pointer!)
        for (const auto& q : quests) {
            info.quests_by_difficulty[q.difficulty].push_back(q);
        }

        modules.push_back(info);
    }

    return modules;
}

std::string QuestBrowser::launch() {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);  // Hide cursor

    // Gather quest data
    auto modules = gather_module_info();
    if (modules.empty()) {
        endwin();
        return "";
    }

    // Get screen dimensions
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // Create separate windows for header, content, and footer
    WINDOW* header_win = newwin(3, max_x, 0, 0);
    WINDOW* content_win = newwin(max_y - 5, max_x, 3, 0);
    WINDOW* footer_win = newwin(2, max_x, max_y - 2, 0);

    if (!header_win || !content_win || !footer_win) {
        endwin();
        return "";
    }

    keypad(content_win, TRUE);

    // Draw header ONCE (fixed width ASCII for stability)
    wattron(header_win, A_BOLD);
    mvwprintw(header_win, 0, 0, "================================================================================");
    mvwprintw(header_win, 1, 0, "                              QUEST BROWSER                                     ");
    mvwprintw(header_win, 2, 0, "================================================================================");
    wattroff(header_win, A_BOLD);
    wrefresh(header_win);

    // Draw footer ONCE (fixed width ASCII)
    wattron(footer_win, A_BOLD);
    mvwprintw(footer_win, 0, 0, "================================================================================");
    wattroff(footer_win, A_BOLD);
    mvwprintw(footer_win, 1, 2, "UP/DOWN: Navigate  RIGHT: Expand  LEFT: Collapse  Enter: Activate  q: Quit");
    wrefresh(footer_win);

    // Navigation state
    int current_module = 0;
    int current_difficulty = -1;  // -1 means module level
    int current_quest = 0;
    std::vector<bool> module_expanded(modules.size(), false);
    std::vector<std::vector<bool>> difficulty_expanded(modules.size());

    for (size_t i = 0; i < modules.size(); ++i) {
        difficulty_expanded[i].resize(4, false);  // 4 difficulty levels
    }

    std::string result;
    bool done = false;

    while (!done) {
        // Only clear and redraw the content area
        werase(content_win);

        int row = 1;  // Start at row 1 within content window
        int displayed_index = 0;
        int content_height = getmaxy(content_win);

        // Display modules and quests
        for (size_t m = 0; m < modules.size(); ++m) {
            const auto& mod = modules[m];

            // Stop if we've filled the window
            if (row >= content_height - 1) break;

            // Module line
            if (current_module == m && current_difficulty == -1) {
                wattron(content_win, A_REVERSE);
            }

            std::string module_marker = module_expanded[m] ? "v" : ">";
            mvwprintw(content_win, row++, 2, "%s %s (%d quests)",
                     module_marker.c_str(), mod.name.c_str(), mod.total_quests);

            if (current_module == m && current_difficulty == -1) {
                wattroff(content_win, A_REVERSE);
            }

            // If expanded, show difficulties
            if (module_expanded[m]) {
                for (int d = 0; d < 4; ++d) {
                    if (row >= content_height - 1) break;

                    auto diff = static_cast<QuestDifficulty>(d);
                    int count = get_difficulty_count(mod, diff);
                    if (count == 0) continue;

                    // Difficulty line
                    if (current_module == m && current_difficulty == d) {
                        wattron(content_win, A_REVERSE);
                    }

                    std::string diff_marker = (difficulty_expanded[m][d]) ? "v" : ">";
                    mvwprintw(content_win, row++, 4, "%s %s (%d)",
                             diff_marker.c_str(), difficulty_to_string(diff).c_str(), count);

                    if (current_module == m && current_difficulty == d) {
                        wattroff(content_win, A_REVERSE);
                    }

                    // If expanded, show quests
                    if (difficulty_expanded[m][d]) {
                        auto it = mod.quests_by_difficulty.find(diff);
                        if (it != mod.quests_by_difficulty.end()) {
                            const auto& quests = it->second;
                            for (size_t q = 0; q < quests.size(); ++q) {
                                if (row >= content_height - 1) break;

                                const auto& quest = quests[q];

                                if (current_module == m && current_difficulty == d && current_quest == static_cast<int>(q)) {
                                    wattron(content_win, A_REVERSE);
                                }

                                std::string status = "";
                                try {
                                    status = quest_mgr_.is_active(mod.name, quest.id) ? " [ACTIVE]" : "";
                                } catch (...) {
                                    // Silently ignore errors checking quest status
                                }

                                mvwprintw(content_win, row++, 6, "- %s: %s [%d XP]%s",
                                         quest.id.c_str(), quest.title.c_str(),
                                         quest.reward_xp, status.c_str());

                                if (current_module == m && current_difficulty == d && current_quest == static_cast<int>(q)) {
                                    wattroff(content_win, A_REVERSE);
                                }
                            }
                        }
                    }
                }
            }
        }

        wrefresh(content_win);

        // Handle input from content window
        int ch = wgetch(content_win);

        switch (ch) {
            case KEY_UP:
                if (current_difficulty >= 0 && difficulty_expanded[current_module][current_difficulty]) {
                    // In quest list
                    if (current_quest > 0) {
                        current_quest--;
                    } else {
                        // Move to difficulty level
                        current_quest = 0;
                        // Stay at difficulty level
                    }
                } else if (current_difficulty >= 0) {
                    // At difficulty level, move to previous difficulty or module
                    bool found = false;
                    for (int d = current_difficulty - 1; d >= 0; --d) {
                        if (get_difficulty_count(modules[current_module], static_cast<QuestDifficulty>(d)) > 0) {
                            current_difficulty = d;
                            current_quest = 0;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        current_difficulty = -1;
                    }
                } else {
                    // At module level
                    if (current_module > 0) {
                        current_module--;
                        current_difficulty = -1;
                        current_quest = 0;
                    }
                }
                break;

            case KEY_DOWN:
                if (current_difficulty >= 0 && difficulty_expanded[current_module][current_difficulty]) {
                    // In quest list
                    auto diff_key = static_cast<QuestDifficulty>(current_difficulty);
                    auto it = modules[current_module].quests_by_difficulty.find(diff_key);
                    if (it != modules[current_module].quests_by_difficulty.end()) {
                        const auto& quests = it->second;
                        if (current_quest < static_cast<int>(quests.size()) - 1) {
                            current_quest++;
                        }
                    }
                } else if (current_difficulty >= 0) {
                    // At difficulty level, move to next difficulty
                    bool found = false;
                    for (int d = current_difficulty + 1; d < 4; ++d) {
                        if (get_difficulty_count(modules[current_module], static_cast<QuestDifficulty>(d)) > 0) {
                            current_difficulty = d;
                            current_quest = 0;
                            found = true;
                            break;
                        }
                    }
                    if (!found && current_module < static_cast<int>(modules.size()) - 1) {
                        current_module++;
                        current_difficulty = -1;
                        current_quest = 0;
                    }
                } else {
                    // At module level
                    if (module_expanded[current_module]) {
                        // Move into first difficulty
                        for (int d = 0; d < 4; ++d) {
                            if (get_difficulty_count(modules[current_module], static_cast<QuestDifficulty>(d)) > 0) {
                                current_difficulty = d;
                                current_quest = 0;
                                break;
                            }
                        }
                    } else if (current_module < static_cast<int>(modules.size()) - 1) {
                        current_module++;
                    }
                }
                break;

            case KEY_RIGHT:
                if (current_difficulty >= 0 && current_difficulty < 4) {
                    // Expand difficulty
                    if (current_module >= 0 && current_module < static_cast<int>(modules.size())) {
                        difficulty_expanded[current_module][current_difficulty] = true;
                    }
                } else if (current_module >= 0 && current_module < static_cast<int>(modules.size())) {
                    // Expand module
                    module_expanded[current_module] = true;
                }
                break;

            case KEY_LEFT:
                if (current_difficulty >= 0 && current_difficulty < 4 &&
                    current_module >= 0 && current_module < static_cast<int>(modules.size()) &&
                    difficulty_expanded[current_module][current_difficulty]) {
                    // Collapse difficulty
                    difficulty_expanded[current_module][current_difficulty] = false;
                    current_quest = 0;
                } else if (current_difficulty >= 0) {
                    // Move back to module
                    current_difficulty = -1;
                    current_quest = 0;
                } else if (current_module >= 0 && current_module < static_cast<int>(modules.size()) &&
                           module_expanded[current_module]) {
                    // Collapse module
                    module_expanded[current_module] = false;
                }
                break;

            case '\n':
            case KEY_ENTER:
                // Activate quest if we're at quest level
                if (current_difficulty >= 0 && difficulty_expanded[current_module][current_difficulty]) {
                    auto diff_key = static_cast<QuestDifficulty>(current_difficulty);
                    auto it = modules[current_module].quests_by_difficulty.find(diff_key);
                    if (it != modules[current_module].quests_by_difficulty.end()) {
                        const auto& quests = it->second;
                        if (current_quest < static_cast<int>(quests.size())) {
                            result = modules[current_module].name + ":" + quests[current_quest].id;
                            done = true;
                        }
                    }
                }
                break;

            case 'q':
            case 'Q':
            case 27:  // ESC
                done = true;
                break;
        }
    }

    // Clean up windows
    delwin(header_win);
    delwin(content_win);
    delwin(footer_win);

    endwin();
    return result;
}
