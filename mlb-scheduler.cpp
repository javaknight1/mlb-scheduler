#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>
#include <string.h>
#include <json/json.h>

Json::Value loadSchedule(const std::string& filePath) {
    Json::Value schedule;

    std::ifstream file(filePath, std::ifstream::binary);
    if (file.is_open()) {
        try {
            file >> schedule;
        } catch (const Json::Exception& e) {
            std::cerr << "Error reading JSON file: " << e.what() << std::endl;
        }

        file.close();
    } else {
        std::cerr << "Error opening file: " << filePath << std::endl;
    }

    return schedule;
}

std::unordered_map<std::string, std::vector<std::string>> convertJsonToMap(const Json::Value& jsonData) {
    std::unordered_map<std::string, std::vector<std::string>> resultMap;

    if (jsonData.isObject()) {
        for (const auto& team : jsonData.getMemberNames()) {
            if (jsonData[team].isArray()) {
                for (const auto& date : jsonData[team]) {
                    if (date.isString()) {
                        resultMap[team].emplace_back(date.asString());
                    }
                }
            }
        }
    }

    return resultMap;
}

std::vector<std::string> getGames(const std::vector<std::string>& schedule) {
    std::vector<std::string> games;

    for (const auto& game : schedule) {
        std::tm datetime_obj = {};
        std::istringstream dateStream(game + "/24");
        dateStream >> std::get_time(&datetime_obj, "%m/%d/%y");
        std::mktime(&datetime_obj);

        if (datetime_obj.tm_wday == 5 || datetime_obj.tm_wday == 6 || datetime_obj.tm_wday == 0 || datetime_obj.tm_wday == 1) {
            if (!(datetime_obj.tm_mon == 4 && datetime_obj.tm_mday < 15) && !(datetime_obj.tm_mon == 5 && datetime_obj.tm_mday < 15)) {
                games.push_back(game);
            }
        }
    }

    return games;
}

std::string create_datestr_for_matching(const std::string& gameday, int delta) {
    // Assuming gameday is in the format "MM/DD/YY"
    std::tm tm_date = {};
    std::istringstream dateStream(gameday + "/24");
    dateStream >> std::get_time(&tm_date, "%m/%d/%y");

    tm_date.tm_mday += delta;
    std::mktime(&tm_date);

    return std::to_string(tm_date.tm_mon+1) + "/" + std::to_string(tm_date.tm_mday);
}

std::vector<std::string> generate_close_dates(const std::string& game, bool include_itself = false, int day_range = 3) {
    std::vector<std::string> result;

    for (int i = 1; i <= day_range; ++i) {
        result.push_back(create_datestr_for_matching(game, i));
        result.push_back(create_datestr_for_matching(game, -i));
    }

    if (include_itself) {
        result.push_back(create_datestr_for_matching(game, 0));
    }

    return result;
}

std::vector<std::vector<std::string>> find_matching_games(const std::vector<std::string>& home_games, const std::vector<std::string>& away_games) {
    std::vector<std::vector<std::string>> matches;

    for (const auto& home : home_games) {
        auto gamedays = generate_close_dates(home);

        for (const auto& gameday : gamedays) {
            for (const auto& away : away_games) {
                if (gameday == away) {
                    matches.push_back({away, home});
                }
            }
        }
    }

    return matches;
}

bool compareDates(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    // Assuming the date is in the format "MM/DD/YY"
    std::string dateA = a[1] + "/24";
    std::string dateB = b[1] + "/24";

    std::tm timeA = {};
    std::tm timeB = {};

    std::istringstream streamA(dateA);
    std::istringstream streamB(dateB);

    streamA >> std::get_time(&timeA, "%m/%d/%y");
    streamB >> std::get_time(&timeB, "%m/%d/%y");

    return std::mktime(&timeA) < std::mktime(&timeB);
}

void save_results(std::vector<std::vector<std::vector<std::string>>>& solutions) {    
    int attempts = 1000;
    if (solutions.size() % attempts == 0) {
        std::cout << "Found " << attempts <<  " more solutions..." << std::endl;
        std::ofstream file("all_results_cpp.json");
        Json::Value json_results;
        for (auto& solution : solutions) {
            Json::Value json_games;
            std::sort(solution.begin(), solution.end(), compareDates);
            for (const auto& games : solution) {
                std::string formatted_date = games[1];
                Json::Value json_item;
                json_games[formatted_date] = games[0];
            }
            json_results.append(json_games);
        }
        std::cout << "..writing to file." << std::endl;
        file << json_results;
        file.close();
        std::exit(3);
    }
}

void find_single_permutation_games(const std::unordered_map<std::string, std::vector<std::string>>& schedule,
                                        const std::vector<std::string>& single_teams,
                                        std::unordered_set<std::string>& ballparks_visited,
                                        std::unordered_set<std::string>& dates_visited,
                                        std::vector<std::vector<std::string>>& current,
                                        std::vector<std::vector<std::vector<std::string>>>& results) {
    if (ballparks_visited.size() == schedule.size()) {
        results.push_back(current);
        save_results(results);
        return;
    }

    for (const auto& single_team : single_teams) {
        if (ballparks_visited.find(single_team) != ballparks_visited.end()) {
            continue;
        }

        for (const auto& date : schedule.at(single_team)) {
            if (dates_visited.find(date) != dates_visited.end()) {
                continue;
            }

            for (const auto& d : generate_close_dates(date, true)) {
                dates_visited.insert(d);
            }

            ballparks_visited.insert(single_team);
            current.push_back({single_team, date});

            find_single_permutation_games(schedule, single_teams, ballparks_visited, dates_visited, current, results);

            current.pop_back();
            ballparks_visited.erase(single_team);

            for (const auto& d : generate_close_dates(date, true)) {
                dates_visited.erase(d);
            }
        }
    }
}

void find_matching_permutation_games(const std::unordered_map<std::string, std::vector<std::string>>& schedule,
                                        const std::vector<std::string>& single_teams,
                                        const std::unordered_map<std::string, std::vector<std::string>>& team_pairs,
                                        std::unordered_set<std::string>& ballparks_visited,
                                        std::unordered_set<std::string>& dates_visited,
                                        std::vector<std::vector<std::string>>& current,
                                        std::vector<std::vector<std::vector<std::string>>>& results) {
    if (ballparks_visited.size() == team_pairs.size()) {
        find_single_permutation_games(schedule, single_teams, ballparks_visited, dates_visited, current, results);
        return;
    }

    for (const auto& team_pair : team_pairs) {
        const std::string& team = team_pair.first;

        if (ballparks_visited.find(team) != ballparks_visited.end()) {
            continue;
        }

        for (const auto& nearby_team : team_pair.second) {
            if (ballparks_visited.find(nearby_team) != ballparks_visited.end()) {
                continue;
            }

            auto matches = find_matching_games(schedule.at(team), schedule.at(nearby_team));

            if (matches.empty()) {
                std::cerr << "Could not find any matches for " << team << " and " << nearby_team << std::endl;
            }

            ballparks_visited.insert(team);
            ballparks_visited.insert(nearby_team);

            for (const auto& match : matches) {
                if (dates_visited.find(match[0]) == dates_visited.end() &&
                    dates_visited.find(match[1]) == dates_visited.end()) {
                    for (const auto& date : generate_close_dates(match[0], true)) {
                        dates_visited.insert(date);
                    }
                    current.push_back({team, match[0]});
                    current.push_back({nearby_team, match[1]});

                    find_matching_permutation_games(schedule, single_teams, team_pairs, ballparks_visited, dates_visited, current, results);

                    current.pop_back();
                    current.pop_back();

                    for (const auto& date : generate_close_dates(match[0], true)) {
                        dates_visited.erase(date);
                    }
                }
            }

            ballparks_visited.erase(team);
            ballparks_visited.erase(nearby_team);
        }
    }
}


int main(int argc, char* argv[])
{
    std::unordered_map<std::string, std::vector<std::string>> team_pairs = {
        {"New York Yankees", {"Boston Red Sox", "Philadelphia Phillies"}},
        {"New York Mets", {"Boston Red Sox", "Philadelphia Phillies"}},
        {"Boston Red Sox", {"New York Mets", "New York Yankees"}},
        {"Philadelphia Phillies", {"New York Yankees", "New York Mets"}},

        {"Baltimore Orioles", {"Washington Nationals"}},
        {"Washington Nationals", {"Baltimore Orioles"}},

        {"Los Angeles Angels", {"Los Angeles Dodgers"}},
        {"Los Angeles Dodgers", {"Los Angeles Angels"}},

        {"Chicago White Sox", {"Chicago Cubs"}},
        {"Chicago Cubs", {"Chicago White Sox"}},

        {"Texas Rangers", {"Houston Astros"}},
        {"Houston Astros", {"Texas Rangers"}},

        {"St. Louis Cardinals", {"Kansas City Royals"}},
        {"Kansas City Royals", {"St. Louis Cardinals"}},

        {"San Francisco Giants", {"Oakland Athletics"}},
        {"Oakland Athletics", {"San Francisco Giants"}},

        {"Detroit Tigers", {"Toronto Blue Jays"}},
        {"Toronto Blue Jays", {"Detroit Tigers"}}
    };

    std::vector<std::string> single_teams = {
        "Cincinnati Reds",
        "Cleveland Guardians",
        "Miami Marlins",
        "Tampa Bay Rays",
        "Pittsburgh Pirates",
        "Arizona Diamondbacks",
        "Colorado Rockies",
        "San Diego Padres",
        "Seattle Mariners",
        "Milwaukee Brewers",
        "Minnesota Twins",
        "Atlanta Braves"
    };

    if (argc < 2) {
        std::cerr << "Expected: ./mlb-scheduler <homegame.json>" << std::endl;
        std::exit(1);
    }

    // Load JSON file
    Json::Value jsonSchedule = loadSchedule(argv[1]);
    if (jsonSchedule.empty()) {
        std::cerr << "Invalid or empty schedule." << std::endl;
        std::exit(2);
    }

    // Filter out certain dates
    std::unordered_map<std::string, std::vector<std::string>> schedule = convertJsonToMap(jsonSchedule);
    for (const auto& pair : schedule) {
        schedule[pair.first] = getGames(pair.second);
    }

    std::unordered_set<std::string> ballparks;
    std::unordered_set<std::string> dates;
    std::vector<std::vector<std::string>> current;
    std::vector<std::vector<std::vector<std::string>>> results;

    find_matching_permutation_games(schedule, single_teams, team_pairs, ballparks, dates, current, results);
}