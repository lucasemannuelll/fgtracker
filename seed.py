import sqlite3
import random
from datetime import datetime, timedelta

DB_PATH = "stats.db"

def generate_mock_data(num_sessions=100):
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    # Ensure table exists matching the schema
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS sessions (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            datetime  TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
            fgm       INTEGER NOT NULL,
            fga       INTEGER NOT NULL
        );
    """)

    # Generate dates across the last 365 days in chronological order
    now = datetime.now()
    start_date = now - timedelta(days=365)
    
    timestamps = []
    for _ in range(num_sessions):
        random_seconds = random.randint(0, int((now - start_date).total_seconds()))
        dt = start_date + timedelta(seconds=random_seconds)
        timestamps.append(dt)
    
    timestamps.sort()

    # Insert randomized shooting data
    for dt in timestamps:
        fga = random.randint(10, 50)           # Total attempts per session
        fg_pct = random.uniform(0.25, 0.80)     # Realistic shooting percentage (30% - 65%)
        fgm = int(fga * fg_pct)                 # Makes
        
        datetime_str = dt.strftime("%Y-%m-%d %H:%M:%S")
        
        cursor.execute(
            "INSERT INTO sessions (datetime, fgm, fga) VALUES (?, ?, ?);",
            (datetime_str, fgm, fga)
        )

    conn.commit()
    conn.close()
    print(f"Successfully inserted {num_sessions} mock sessions into '{DB_PATH}'.")

if __name__ == "__main__":
    generate_mock_data()
