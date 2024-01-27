import argparse
import json
from datetime import datetime, timedelta
from collections import defaultdict
import requests

API_KEY = 'd61b8748f9c4ce9abf8b970c5a14565b0f5ec5fc2cb373ed35f599660a6fde18'
CLIENT_ID = 'Mzk2Mjk4NTh8MTcwNjMxNDU0MC4zNzM3NTQ3'
BASE_URL = 'https://api.seatgeek.com/2'

def parseargs():
    parser = argparse.ArgumentParser(description="Given a json file of MLB schedule, search seatgeek for prices of each game")
    parser.add_argument("infile", help="File that has mlb schedules")
    
    return parser.parse_args()

def load_schedule(file_path):
    with open(file_path, 'r') as file:
        schedule = json.load(file)
    return schedule

def search_seatgeek_events(team, date):
    endpoint = f"/events"
    tomorrow = datetime.strptime(date+"/24", '%m/%d/%y') + timedelta(days=1)
    params = {
        'q': team,
        'datetime_local.gte': date,
        'datetime_local.lte': tomorrow,
        'per_page': 1000,
        'client_id': CLIENT_ID
    }

    response = requests.get(BASE_URL + endpoint, params=params)

    if response.status_code == 200:
        data = response.json()
        return data.get('events', [])[0]
    else:
        print(f"Error: {response.status_code}, {response.text}")

    return response

def main():
    args = parseargs()
    mlb_schedule = args.infile
    schedules = load_schedule(mlb_schedule)
    prices = {}
    total = 0

    if not schedules:
        print("Invalid or empty schedule.")
        return

    for schedule in schedules:
        for game in schedule:
            price = search_seatgeek_events(schedule[game], game)["stats"]["lowest_price"]
            prices[schedule[game]] = price
            total += price

    print(f"Total Cost {total}; Average Cost: {total/len(prices)}")
    with open("results_prices.json", 'w') as file:
        json.dump(prices, file)

if __name__ == "__main__":
    main()
