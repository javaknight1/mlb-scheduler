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
using namespace std;

Json::Value load_schedule(const string& filePath) {
    Json::Value schedule;

    ifstream file(filePath, ifstream::binary);
    if (file.is_open()) {
        try {
            file >> schedule;
        } catch (const Json::Exception& e) {
            cerr << "Error reading JSON file: " << e.what() << endl;
        }

        file.close();
    } else {
        cerr << "Error opening file: " << filePath << endl;
    }

    return schedule;
}

unordered_map<string, vector<string>> convert_json_to_map(const Json::Value& jsonData) {
    unordered_map<string, vector<string>> resultMap;

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

vector<string> get_games(const vector<string>& schedule) {
    vector<string> games;

    for (const auto& game : schedule) {
        tm datetime_obj = {};
        istringstream dateStream(game + "/24");
        dateStream >> get_time(&datetime_obj, "%m/%d/%y");
        mktime(&datetime_obj);

        if (datetime_obj.tm_wday == 5 || datetime_obj.tm_wday == 6 || datetime_obj.tm_wday == 0 || datetime_obj.tm_wday == 1) {
            if (!(datetime_obj.tm_mon == 4 && datetime_obj.tm_mday < 15) && 
                !(datetime_obj.tm_mon == 5 && datetime_obj.tm_mday < 15) && 
                !(datetime_obj.tm_mon == 3 && datetime_obj.tm_mday > 21)) {
                games.push_back(game);
            }
        }
    }

    return games;
}

string create_datestr_for_matching(const string& gameday, int delta) {
    // Assuming gameday is in the format "MM/DD/YY"
    tm tm_date = {};
    istringstream dateStream(gameday + "/24");
    dateStream >> get_time(&tm_date, "%m/%d/%y");

    tm_date.tm_mday += delta;
    mktime(&tm_date);

    return to_string(tm_date.tm_mon+1) + "/" + to_string(tm_date.tm_mday);
}

vector<string> generate_close_dates(const string& game, bool include_itself = false, int day_range = 3) {
    vector<string> result;

    for (int i = 1; i <= day_range; ++i) {
        result.insert(result.begin(), create_datestr_for_matching(game, i));
        result.push_back(create_datestr_for_matching(game, -i));
    }

    if (include_itself) {
        result.push_back(create_datestr_for_matching(game, 0));
    }

    return result;
}

vector<vector<string>> find_matching_games(const vector<string>& home_games, const vector<string>& away_games) {
    vector<vector<string>> matches;

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

bool compare_dates(const vector<string>& a, const vector<string>& b) {
    // Assuming the date is in the format "MM/DD/YY"
    string dateA = a[1] + "/24";
    string dateB = b[1] + "/24";

    tm timeA = {};
    tm timeB = {};

    istringstream streamA(dateA);
    istringstream streamB(dateB);

    streamA >> get_time(&timeA, "%m/%d/%y");
    streamB >> get_time(&timeB, "%m/%d/%y");

    return mktime(&timeA) < mktime(&timeB);
}

void save_results(vector<vector<vector<string>>>& solutions) {    
    int attempts = 1;
    if (solutions.size() % attempts == 0) {
        cout << "Found " << attempts <<  " more solutions..." << endl;
        ofstream file("all_results_cpp.json");
        Json::Value json_results;
        for (auto& solution : solutions) {
            Json::Value json_games;
            sort(solution.begin(), solution.end(), compare_dates);
            for (const auto& games : solution) {
                string formatted_date = games[1];
                Json::Value json_item;
                json_games[formatted_date] = games[0];
            }
            json_results.append(json_games);
        }
        cout << "..writing to file." << endl;
        file << json_results;
        file.close();
        exit(3);
    }
}

void find_single_permutation_games(const unordered_map<string, vector<string>>& schedule, const vector<string>& single_teams, 
                unordered_set<string>& ballparks_visited, unordered_set<string>& dates_visited,
                vector<vector<string>>& current, vector<vector<vector<string>>>& results) {
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

void find_matching_permutation_games(const unordered_map<string, vector<string>>& schedule, const vector<string>& single_teams,
                const unordered_map<string, vector<string>>& team_pairs, unordered_set<string>& ballparks_visited,
                unordered_set<string>& dates_visited, vector<vector<string>>& current, vector<vector<vector<string>>>& results) {
    if (ballparks_visited.size() == team_pairs.size()) {
        find_single_permutation_games(schedule, single_teams, ballparks_visited, dates_visited, current, results);
        return;
    }

    for (const auto& team_pair : team_pairs) {
        const string& team = team_pair.first;

        if (ballparks_visited.find(team) != ballparks_visited.end()) {
            continue;
        }

        for (const auto& nearby_team : team_pair.second) {
            if (ballparks_visited.find(nearby_team) != ballparks_visited.end()) {
                continue;
            }

            auto matches = find_matching_games(schedule.at(team), schedule.at(nearby_team));

            if (matches.empty()) {
                cerr << "Could not find any matches for " << team << " and " << nearby_team << endl;
            }

            ballparks_visited.insert(team);
            ballparks_visited.insert(nearby_team);

            for (const auto& match : matches) {
                if (dates_visited.find(match[0]) == dates_visited.end() &&
                    dates_visited.find(match[1]) == dates_visited.end()) {
                    for (const auto& date : generate_close_dates(match[0], true)) {
                        dates_visited.insert(date);
                    }
                    current.push_back({nearby_team, match[0]});
                    current.push_back({team, match[1]});

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
    unordered_map<string, vector<string>> team_pairs = {
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

    vector<string> single_teams = {
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
        cerr << "Expected: ./mlb-scheduler.app <homegames.json>" << endl;
        exit(1);
    }

    // Load JSON file
    Json::Value jsonSchedule = load_schedule(argv[1]);
    if (jsonSchedule.empty()) {
        cerr << "Invalid or empty schedule." << endl;
        exit(2);
    }

    // Filter out certain dates
    unordered_map<string, vector<string>> schedule = convert_json_to_map(jsonSchedule);
    for (const auto& pair : schedule) {
        schedule[pair.first] = get_games(pair.second);
    }

    unordered_set<string> ballparks;
    unordered_set<string> dates;
    vector<vector<string>> current;
    vector<vector<vector<string>>> results;

    find_matching_permutation_games(schedule, single_teams, team_pairs, ballparks, dates, current, results);
}