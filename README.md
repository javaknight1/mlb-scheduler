## MLB Scheduler

This C++ and Python script will take a given MLB Schedule for the year and find as many possible outcomes where you would be able to go to all the ballparks in one season.

### Criteria

Within the code, I have a couple of embedded criteria:

- If attending a game, only attend one game in a day
- If attending multiple games on a weekend, only go to ballparks that are near each other (for example: Mets and Yankees)
- Since we will be working a regular 9-5 job during the season, only consider Fri-Mon games
- For me, I'm not available for every week of the season, so I've hardcoded those days (see getGames function)

### Algorithm

Since there were over a million scenarios AND had less than 30 weeks in a season, I needed to create a creative way to find the best schedule. By reading the code, you will be able to see this:

- Filter out all the home games that aren't viable (non-weekends and vacations)
- For teams that are close to each other, first find weekends where I would be able to attend two home games in a single weekend (for example: Chicago or LA)
- After that, we will take the remaining teams and find a home game on a weekend where we aren't attending any games
- When a solution is found, it's added to a referenced results vector
- When we hit around 10,000 solutions in our referenced results vector, we upload these to a JSON file

### How to Run

1. Update the `Makefile` with the correct paths to `json.h` library. Currently, it is setup with homebrew on macbook.
2. Run `make && ./mlb-scheduler.app schedules/home_2024_schedule.json`

### Future Improvements

Since I mainly created this to create a schedule for myself for the 2024 season, I doubt I will use this again or improve on it. But, I hope others will find usefulness in this. :)

- Performance
  - Looking at the code, it works, but I'm sure there are ways we can clean it up and optimize it
- Customization
  - Many things in the code are specifically designed for me (filtering out my vacations, hardcoding the output filename, etc.)
  - It would be nice to have a cleaner command line interface that would allow you to have stronger control of those variables
- More flexible filtering
  - When it comes to filtering the dates and such, they were MY personal preference
  - It would be nice to create a more flexible interface to cater to individual needs
 
### Contact

Feel free to reach out to me on github or Instagram @realrobavery with any questions. Happy coding!!
