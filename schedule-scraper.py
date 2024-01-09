#!/usr/bin/python
# -*- coding: utf-8 -*-

import argparse
import re
import json

DATEREGEX = r'(Sunday|Monday|Tuesday|Wednesday|Thursday|Friday|Saturday)\, (February|March|April|May|June|July|August|September|October) (\d{1,2})\, \d\d\d\d\n'
GAMEREGEX = r'(TBD|\d\d?\:\d\d (am|pm)) (\(Spring\))?(.*) @ (.*)'

def parseargs():
    parser = argparse.ArgumentParser(description="Given a plaintext file of MLB schedule, this will convert it into a json file.")
    parser.add_argument("infile", help="File that has mlb schdule to parse")
    parser.add_argument("outfile", help="Path to json output file")
    
    return parser.parse_args()

def monthToNum(shortMonth):
    return {
            'january' : 1,
            'february' : 2,
            'march' : 3,
            'april' : 4,
            'may' : 5,
            'june' : 6,
            'july' : 7,
            'august' : 8,
            'september' : 9, 
            'october' : 10,
            'november' : 11,
            'december' : 12
    }[shortMonth.lower()]

if __name__ == "__main__":
    args = parseargs()
    schedule_json = []

    with open(args.infile, 'r') as f:
        month = 0
        day = 0
        for line in f:
            # blank line, ignore
            if line == "":
                continue

            matchDate = re.match(DATEREGEX, line, re.M|re.I)
            if matchDate:
                month = monthToNum(matchDate.group(2))
                day = matchDate.group(3)
                continue

            matchGame = re.match(GAMEREGEX, line, re.M|re.I)
            if matchGame:
                spring = matchGame.group(3) == "(Spring)"
                time = matchGame.group(1)
                home = matchGame.group(5)
                away = matchGame.group(4)
                schedule_json.append({
                    'month': month,
                    'day': day,
                    'time': time,
                    'springtraining': spring,
                    'home': home.strip(),
                    'away': away.strip()
                })

        with open(args.outfile, 'w') as f:
            json.dump(schedule_json, f)

