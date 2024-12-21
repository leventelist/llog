#!/usr/bin/env python3

import csv
import sqlite3
import requests
import os

# URL
csv_url = 'https://www.sotadata.org.uk/summitslist.csv'

# Define the CSV file and SQLite database file
csv_file = 'summitslist.csv'
sqlite_db = 'summitslist.sqlite'


# Check if the CSV file already exists
if os.path.exists(csv_file):
    print(f"{csv_file} already exists. Skipping download.")
else:
    # Download the CSV file
    print(f"Downloading {csv_url}...")
    try:
        response = requests.get(csv_url)
        response.raise_for_status()  # Check if the request was successful
    except requests.exceptions.RequestException as e:
        print(f"Error downloading CSV file: {e}")
        exit(1)

    # Save the CSV file locally
    with open(csv_file, 'wb') as file:
        file.write(response.content)

# Connect to the SQLite database (it will be created if it doesn't exist)
try:
    conn = sqlite3.connect(sqlite_db)
except sqlite3.Error as e:
    print(f"Error connecting to database: {e}")
    exit(1)
cursor = conn.cursor()

# Drop the table if it exists
cursor.execute('DROP TABLE IF EXISTS summit_data')

# Create the table (if it doesn't exist)
cursor.execute('''
CREATE TABLE IF NOT EXISTS summit_data (
    summit_code TEXT,
    association_name TEXT,
    region_name TEXT,
    summit_name TEXT,
    alt_m INTEGER,
    alt_ft INTEGER,
    grid_ref1 REAL,
    grid_ref2 REAL,
    longitude REAL,
    latitude REAL,
    points INTEGER,
    bonus_points INTEGER,
    valid_from TEXT,
    valid_to TEXT,
    activation_count INTEGER,
    activation_date TEXT,
    activation_call TEXT
)
''')

# Read the CSV file and insert data into the database
try:
    print(f"Opening {csv_file}...")
    with open(csv_file, 'r') as file:
        reader = csv.reader(file)
        header = next(reader)  # Skip the title row
        header = next(reader)  # Skip the header row
        for row in reader:
            try:
                cursor.execute('''
                INSERT INTO summit_data (
                    summit_code, association_name, region_name, summit_name, alt_m, alt_ft, grid_ref1, grid_ref2, longitude, latitude, points, bonus_points, valid_from, valid_to, activation_count, activation_date, activation_call
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                ''', row)
            except sqlite3.Error as e:
                print(f"Error inserting row {row}: {e}")
except FileNotFoundError as e:
    print(f"Error: {e}")
except csv.Error as e:
    print(f"Error reading CSV file: {e}")

# Commit the transaction and close the connection
conn.commit()
conn.close()

print(f"Data from {csv_file} has been successfully inserted into {sqlite_db}")
exit(0)
