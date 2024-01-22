import argparse
import json
from datetime import datetime, timedelta
from collections import defaultdict

def parseargs():
    parser = argparse.ArgumentParser(description="Given a json file of MLB schedule, this will generate a schedule to visit all the ballparks.")
    parser.add_argument("infile", help="File that has mlb schedules")
    
    return parser.parse_args()

def load_schedule(file_path):
    with open(file_path, 'r') as file:
        schedule = json.load(file)
    return schedule

def generate_ballpark_schedule(homegames_schedule, team, dates, count):

    if len(homegames_schedule) == len(dates):
        # Found a solution!
        return count
    
    teamname = list(homegames_schedule.keys())[team]
    hometeam_games = homegames_schedule[teamname]
    solutions = 0
    
    for date in hometeam_games:
        if date not in dates:
            dates.add(date)
            solutions += generate_ballpark_schedule(homegames_schedule, team+1, dates, count+1)
            dates.remove(date)

    return solutions

def get_games(schedule):
    games = []
    for game in schedule:
        datetime_obj = datetime.strptime(game+"/24", '%m/%d/%y')
        if datetime_obj.weekday() in [4, 5, 6, 0]:
            games.append(game)
    return games

def create_datestr_for_matching(gameday, delta):
    day = gameday + timedelta(days=delta)
    return str(day.month) + "/" + str(day.day)

def generate_close_dates(game, include_itself=False, day_range=3):
    gameday = datetime.strptime(game+"/24", '%m/%d/%y')
    result = []

    for i in range(1, day_range+1):
        result.append(create_datestr_for_matching(gameday, i))
        result.append(create_datestr_for_matching(gameday, i*-1))

    if include_itself:
        result.append(create_datestr_for_matching(gameday, 0))
    return result

def find_matching_games(teams):
    matches = []
    for game in teams[0]:
        gamedays = generate_close_dates(game)
        
        for g in gamedays:
            if g in teams[1]:
                matches.append([game, g])

    return matches

all_results = []

def find_matching_permutation_games(schedule, single_teams, team_pairs, ballparks_visited, dates_visited, working):

    if len(ballparks_visited) == len(team_pairs):
        find_single_permutation_games(schedule, single_teams, ballparks_visited, dates_visited, working)
        return

    for team in team_pairs:

        if team in ballparks_visited:
            continue

        for nearby_team in team_pairs[team]:
            if nearby_team in ballparks_visited:
                continue

            matches = find_matching_games([schedule[team], schedule[nearby_team]])

            if len(matches) == 0:
                import pdb; pdb.set_trace()
                print(f"Could not find any matches for {team} and {nearby_team}")

            ballparks_visited.add(team)
            ballparks_visited.add(nearby_team)
            for match in matches:
                # print(f"Found a match! {team} - {match[0]} and {nearby_team} - {match[1]}")
                if match[0] not in dates_visited and match[1] not in dates_visited:
                    for date in generate_close_dates(match[0], True):
                        dates_visited.add(date)
                    working.append([team, match[0]])
                    working.append([nearby_team, match[1]])
                    # if len(ballparks_visited) > 11:
                    #     import pdb; pdb.set_trace()
                    #     print(f"You are {len(ballparks_visited)} levels deep....")
                    find_matching_permutation_games(schedule, single_teams, team_pairs, ballparks_visited, dates_visited, working)
                    working.pop()
                    working.pop()
                    for date in generate_close_dates(match[0], True):
                        dates_visited.remove(date)
            ballparks_visited.remove(team)
            ballparks_visited.remove(nearby_team)

def find_single_permutation_games(schedule, single_teams, ballparks_visited, dates_visited, working):

    if len(ballparks_visited) == len(schedule):
        if len(all_results) % 1000 == 0:
            print("Found 1000 more solutions...")
            with open("all_results.json", 'w') as file:
                json.dump(all_results, file)
        all_results.append(sorted(working, key=lambda x: datetime.strptime(x[1]+"/24", '%m/%d/%y')))

    for single_team in single_teams:
        if single_team in ballparks_visited:
            continue
        for date in schedule[single_team]:
            if date in dates_visited:
                continue

            for d in generate_close_dates(date, True):
                dates_visited.add(d)
            ballparks_visited.add(single_team)
            working.append([single_team, date])

            find_single_permutation_games(schedule, single_teams, ballparks_visited, dates_visited, working)

            working.pop()
            ballparks_visited.remove(single_team)
            for d in generate_close_dates(date, True):
                dates_visited.discard(d)

def main():
    args = parseargs()
    mlb_schedule = args.infile
    schedule = load_schedule(mlb_schedule)

    team_pairs = {
        "New York Yankees": [ "Boston Red Sox", "Philadelphia Phillies"],
        "New York Mets": ["Boston Red Sox", "Philadelphia Phillies"],
        "Boston Red Sox": ["New York Mets", "New York Yankees"],
        "Philadelphia Phillies": ["New York Yankees", "New York Mets"],

        "Baltimore Orioles": ["Washington Nationals"],
        "Washington Nationals": ["Baltimore Orioles"],

        "Los Angeles Angels": ["Los Angeles Dodgers"],
        "Los Angeles Dodgers": ["Los Angeles Angels"],

        "Chicago White Sox": ["Chicago Cubs"],
        "Chicago Cubs": ["Chicago White Sox"],

        "Texas Rangers": ["Houston Astros"],
        "Houston Astros": ["Texas Rangers"],

        "St. Louis Cardinals": ["Kansas City Royals"],
        "Kansas City Royals": ["St. Louis Cardinals", ],

        "San Francisco Giants": ["Oakland Athletics"],
        "Oakland Athletics": ["San Francisco Giants"],

        "Detroit Tigers": ["Toronto Blue Jays"],
        "Toronto Blue Jays": ["Detroit Tigers"]
    }

    single_teams = [
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
    ]

    if not schedule:
        print("Invalid or empty schedule.")
        return

    for s in schedule:
        schedule[s] = get_games(schedule[s])

    find_matching_permutation_games(schedule, single_teams, team_pairs, set(), set(), [])

if __name__ == "__main__":
    main()

