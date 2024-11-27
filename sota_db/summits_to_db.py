import csv
import sqlite3
import requests

# URL
csv_url = 'https://www.sotadata.org.uk/summitslist.csv'

# Define the CSV file and SQLite database file
csv_file = 'summitslist.csv'
sqlite_db = 'summitslist.sqlite'


# Download the CSV file
response = requests.get(csv_url)
response.raise_for_status()  # Check if the request was successful

# Save the CSV file locally
with open(csv_file, 'wb') as file:
    file.write(response.content)

# Connect to the SQLite database (it will be created if it doesn't exist)
conn = sqlite3.connect(sqlite_db)
cursor = conn.cursor()

# Create the table (if it doesn't exist)
cursor.execute('''
CREATE TABLE IF NOT EXISTS GPSData (
    SummitCode TEXT,
    AssociationName TEXT,
    RegionName TEXT,
    SummitName TEXT,
    AltM INTEGER,
    AltFt INTEGER,
    GridRef1 REAL,
    GridRef2 REAL,
    Longitude REAL,
    Latitude REAL,
    Points INTEGER,
    BonusPoints INTEGER,
    ValidFrom TEXT,
    ValidTo TEXT,
    ActivationCount INTEGER,
    ActivationDate TEXT,
    ActivationCall TEXT
)
''')

# Read the CSV file and insert data into the database
with open(csv_file, 'r') as file:
    reader = csv.reader(file)
    header = next(reader)  # Skip the header row
    for row in reader:
        cursor.execute('''
        INSERT INTO GPSData (
            SummitCode, AssociationName, RegionName, SummitName, AltM, AltFt, GridRef1, GridRef2, Longitude, Latitude, Points, BonusPoints, ValidFrom, ValidTo, ActivationCount, ActivationDate, ActivationCall
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ''', row)

# Commit the transaction and close the connection
conn.commit()
conn.close()

print(f"Data from {csv_file} has been successfully inserted into {sqlite_db}")