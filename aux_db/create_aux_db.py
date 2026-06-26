#!/usr/bin/env python3

import csv
import sqlite3
import requests
import os
import argparse
import pathlib


# Paths
CSV_URL    = 'https://www.sotadata.org.uk/summitslist.csv'
CSV_FILE   = 'summitslist.csv'
SQLITE_DB  = 'aux_db.sqlite'

POTA_CSV_URL  = 'https://pota.app/all_parks_ext.csv'
POTA_CSV_FILE = 'pota_parks.csv'

WWFF_CSV_URL  = 'https://wwff.co/wwff-data/wwff_directory.csv'
WWFF_CSV_FILE = 'wwff_directory.csv'


def parse_args():
    p = argparse.ArgumentParser(description='Build auxiliary database for llog')
    p.add_argument('--output', default=SQLITE_DB,
                   help=f'Path for the output SQLite DB (default: {SQLITE_DB})')
    return p.parse_args()


# ---------------------------------------------------------------------------
# Database helpers
# ---------------------------------------------------------------------------

def open_db(db_path):
    """Open the SQLite database and return (conn, cursor)."""
    try:
        conn = sqlite3.connect(db_path)
    except sqlite3.Error as e:
        print(f"Error connecting to database: {e}")
        raise
    return conn, conn.cursor()


def close_db(conn):
    """Commit and close the database connection."""
    conn.commit()
    conn.close()


# ---------------------------------------------------------------------------
# SOTA summits
# ---------------------------------------------------------------------------

def download_sota_csv(csv_url, csv_file):
    """Download the SOTA summits CSV if it is not already present locally."""
    if os.path.exists(csv_file):
        print(f"{csv_file} already exists. Skipping download.")
        return

    print(f"Downloading {csv_url}...")
    try:
        response = requests.get(csv_url)
        response.raise_for_status()
    except requests.exceptions.RequestException as e:
        print(f"Error downloading CSV file: {e}")
        raise

    with open(csv_file, 'wb') as f:
        f.write(response.content)


def create_sota_table(cursor):
    """Create the summit_data table (dropping any previous version)."""
    cursor.execute('DROP TABLE IF EXISTS summit_data')
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS summit_data (
            summit_code      TEXT,
            association_name TEXT,
            region_name      TEXT,
            summit_name      TEXT,
            alt_m            INTEGER,
            alt_ft           INTEGER,
            grid_ref1        REAL,
            grid_ref2        REAL,
            longitude        REAL,
            latitude         REAL,
            points           INTEGER,
            bonus_points     INTEGER,
            valid_from       TEXT,
            valid_to         TEXT,
            activation_count INTEGER,
            activation_date  TEXT,
            activation_call  TEXT
        )
    ''')


def import_sota_summits(conn, cursor, csv_file):
    """Read the SOTA CSV and populate the summit_data table."""
    create_sota_table(cursor)

    try:
        print(f"Opening {csv_file}...")
        with open(csv_file, 'r') as f:
            reader = csv.reader(f)
            next(reader)  # skip title row
            next(reader)  # skip header row
            for row in reader:
                try:
                    cursor.execute('''
                        INSERT INTO summit_data (
                            summit_code, association_name, region_name,
                            summit_name, alt_m, alt_ft, grid_ref1, grid_ref2,
                            longitude, latitude, points, bonus_points,
                            valid_from, valid_to, activation_count,
                            activation_date, activation_call
                        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                    ''', row)
                except sqlite3.Error as e:
                    print(f"Error inserting row {row}: {e}")
    except FileNotFoundError as e:
        print(f"Error: {e}")
        raise
    except csv.Error as e:
        print(f"Error reading CSV file: {e}")
        raise

    print(f"SOTA summit data from {csv_file} imported successfully.")


# ---------------------------------------------------------------------------
# POTA parks
# ---------------------------------------------------------------------------

def download_pota_csv(csv_url, csv_file):
    """Download the POTA all_parks_ext.csv if not already present locally."""
    if os.path.exists(csv_file):
        print(f"{csv_file} already exists. Skipping download.")
        return

    print(f"Downloading {csv_url}...")
    try:
        response = requests.get(csv_url)
        response.raise_for_status()
    except requests.exceptions.RequestException as e:
        print(f"Error downloading POTA CSV: {e}")
        raise

    with open(csv_file, 'wb') as f:
        f.write(response.content)


def create_pota_table(cursor):
    """Create the pota_park_data table (dropping any previous version)."""
    cursor.execute('DROP TABLE IF EXISTS pota_park_data')
    cursor.execute('''
        CREATE TABLE pota_park_data (
            reference    TEXT,
            name         TEXT,
            active       INTEGER,
            entity_id    INTEGER,
            location     TEXT,
            latitude     REAL,
            longitude    REAL,
            grid         TEXT
        )
    ''')
    cursor.execute('CREATE INDEX IF NOT EXISTS idx_pota_reference ON pota_park_data (reference)')


def import_pota_parks(conn, cursor, csv_file):
    """Read the POTA CSV and populate the pota_park_data table."""
    create_pota_table(cursor)

    try:
        print(f"Opening {csv_file}...")
        with open(csv_file, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for row in reader:
                try:
                    cursor.execute('''
                        INSERT INTO pota_park_data (
                            reference, name, active, entity_id,
                            location, latitude, longitude, grid
                        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                    ''', (
                        row.get('reference', ''),
                        row.get('name', ''),
                        int(row.get('active', 0) or 0),
                        int(row.get('entityId', 0) or 0),
                        row.get('locationDesc', ''),
                        float(row.get('latitude', 0) or 0),
                        float(row.get('longitude', 0) or 0),
                        row.get('grid', ''),
                    ))
                except (sqlite3.Error, ValueError) as e:
                    print(f"Error inserting POTA row {row}: {e}")
    except FileNotFoundError as e:
        print(f"Error: {e}")
        raise
    except csv.Error as e:
        print(f"Error reading POTA CSV: {e}")
        raise

    print(f"POTA park data from {csv_file} imported successfully.")


# ---------------------------------------------------------------------------
# WWFF areas
# ---------------------------------------------------------------------------

def download_wwff_csv(csv_url, csv_file):
    """Download the WWFF directory CSV if not already present locally."""
    if os.path.exists(csv_file):
        print(f"{csv_file} already exists. Skipping download.")
        return

    print(f"Downloading {csv_url}...")
    try:
        response = requests.get(csv_url)
        response.raise_for_status()
    except requests.exceptions.RequestException as e:
        print(f"Error downloading WWFF CSV: {e}")
        raise

    with open(csv_file, 'wb') as f:
        f.write(response.content)


def create_wwff_table(cursor):
    """Create the wwff_area_data table (dropping any previous version)."""
    cursor.execute('DROP TABLE IF EXISTS wwff_area_data')
    cursor.execute('''
        CREATE TABLE wwff_area_data (
            reference    TEXT,
            status       TEXT,
            name         TEXT,
            program      TEXT,
            dxcc         TEXT,
            continent    TEXT,
            iaru_locator TEXT,
            latitude     REAL,
            longitude    REAL,
            country      TEXT,
            valid_from   TEXT,
            valid_to     TEXT
        )
    ''')
    cursor.execute('CREATE INDEX IF NOT EXISTS idx_wwff_reference ON wwff_area_data (reference)')


def import_wwff_areas(conn, cursor, csv_file):
    """Read the WWFF directory CSV and populate the wwff_area_data table."""
    create_wwff_table(cursor)

    try:
        print(f"Opening {csv_file}...")
        with open(csv_file, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for row in reader:
                try:
                    cursor.execute('''
                        INSERT INTO wwff_area_data (
                            reference, status, name, program, dxcc,
                            continent, iaru_locator, latitude, longitude,
                            country, valid_from, valid_to
                        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                    ''', (
                        row.get('reference', ''),
                        row.get('status', ''),
                        row.get('name', ''),
                        row.get('program', ''),
                        row.get('dxcc', ''),
                        row.get('continent', ''),
                        row.get('iaruLocator', ''),
                        float(row.get('latitude', 0) or 0),
                        float(row.get('longitude', 0) or 0),
                        row.get('country', ''),
                        row.get('validFrom', ''),
                        row.get('validTo', ''),
                    ))
                except (sqlite3.Error, ValueError) as e:
                    print(f"Error inserting WWFF row {row}: {e}")
    except FileNotFoundError as e:
        print(f"Error: {e}")
        raise
    except csv.Error as e:
        print(f"Error reading WWFF CSV: {e}")
        raise

    print(f"WWFF area data from {csv_file} imported successfully.")


# ---------------------------------------------------------------------------
# Amateur radio modes
# ---------------------------------------------------------------------------

# Derived from the ADIF specification / WSJT-X mode list.
MODES = [
    ('8PSK125',        '599', 'PSK',     ''),
    ('8PSK125F',       '599', 'PSK',     ''),
    ('8PSK125FL',      '599', 'PSK',     ''),
    ('8PSK250',        '599', 'PSK',     ''),
    ('8PSK250F',       '599', 'PSK',     ''),
    ('8PSK250FL',      '599', 'PSK',     ''),
    ('8PSK500',        '599', 'PSK',     ''),
    ('8PSK500F',       '599', 'PSK',     ''),
    ('8PSK1000',       '599', 'PSK',     ''),
    ('8PSK1000F',      '599', 'PSK',     ''),
    ('8PSK1200F',      '599', 'PSK',     ''),
    ('AMTORFEC',       '599', 'TOR',     ''),
    ('ASCI',           '599', 'RTTY',    ''),
    ('CHIP64',         '599', 'CHIP',    ''),
    ('CHIP128',        '599', 'CHIP',    ''),
    ('DOM-M',          '599', 'DOMINO',  ''),
    ('DOM4',           '599', 'DOMINO',  ''),
    ('DOM5',           '599', 'DOMINO',  ''),
    ('DOM8',           '599', 'DOMINO',  ''),
    ('DOM11',          '599', 'DOMINO',  ''),
    ('DOM16',          '599', 'DOMINO',  ''),
    ('DOM22',          '599', 'DOMINO',  ''),
    ('DOM44',          '599', 'DOMINO',  ''),
    ('DOM88',          '599', 'DOMINO',  ''),
    ('DOMINOEX',       '599', 'DOMINO',  ''),
    ('DOMINOF',        '599', 'DOMINO',  ''),
    ('FMHELL',         '599', 'HELL',    ''),
    ('FSK31',          '599', 'PSK',     ''),
    ('FSKHELL',        '599', 'HELL',    ''),
    ('FSQCALL',        '599', 'MFSK',    ''),
    ('FST4',           '599', 'MFSK',    ''),
    ('FST4W',          '599', 'MFSK',    ''),
    ('FT4',            '599', 'MFSK',    ''),
    ('GTOR',           '599', 'TOR',     ''),
    ('HELL80',         '599', 'HELL',    ''),
    ('HELLX5',         '599', 'HELL',    ''),
    ('HELLX9',         '599', 'HELL',    ''),
    ('HFSK',           '599', 'HELL',    ''),
    ('ISCAT-A',        '599', 'ISCAT',   ''),
    ('ISCAT-B',        '599', 'ISCAT',   ''),
    ('JS8',            '599', 'MFSK',    ''),
    ('JT4A',           '599', 'JT4',     ''),
    ('JT4B',           '599', 'JT4',     ''),
    ('JT4C',           '599', 'JT4',     ''),
    ('JT4D',           '599', 'JT4',     ''),
    ('JT4E',           '599', 'JT4',     ''),
    ('JT4F',           '599', 'JT4',     ''),
    ('JT4G',           '599', 'JT4',     ''),
    ('JT9-1',          '599', 'JT9',     ''),
    ('JT9-2',          '599', 'JT9',     ''),
    ('JT9-5',          '599', 'JT9',     ''),
    ('JT9-10',         '599', 'JT9',     ''),
    ('JT9-30',         '599', 'JT9',     ''),
    ('JT9A',           '599', 'JT9',     ''),
    ('JT9B',           '599', 'JT9',     ''),
    ('JT9C',           '599', 'JT9',     ''),
    ('JT9D',           '599', 'JT9',     ''),
    ('JT9E',           '599', 'JT9',     ''),
    ('JT9E FAST',      '599', 'JT9',     ''),
    ('JT9F',           '599', 'JT9',     ''),
    ('JT9F FAST',      '599', 'JT9',     ''),
    ('JT9G',           '599', 'JT9',     ''),
    ('JT9G FAST',      '599', 'JT9',     ''),
    ('JT9H',           '599', 'JT9',     ''),
    ('JT9H FAST',      '599', 'JT9',     ''),
    ('JT65A',          '599', 'JT65',    ''),
    ('JT65B',          '599', 'JT65',    ''),
    ('JT65B2',         '599', 'JT65',    ''),
    ('JT65C',          '599', 'JT65',    ''),
    ('JT65C2',         '599', 'JT65',    ''),
    ('JTMS',           '599', 'MFSK',    ''),
    ('LSB',            '59',  'SSB',     ''),
    ('MFSK4',          '599', 'MFSK',    ''),
    ('MFSK8',          '599', 'MFSK',    ''),
    ('MFSK11',         '599', 'MFSK',    ''),
    ('MFSK16',         '599', 'MFSK',    ''),
    ('MFSK22',         '599', 'MFSK',    ''),
    ('MFSK31',         '599', 'MFSK',    ''),
    ('MFSK32',         '599', 'MFSK',    ''),
    ('MFSK64',         '599', 'MFSK',    ''),
    ('MFSK64L',        '599', 'MFSK',    ''),
    ('MFSK128',        '599', 'MFSK',    ''),
    ('MFSK128L',       '599', 'MFSK',    ''),
    ('NAVTEX',         '599', 'TOR',     ''),
    ('OLIVIA 4/125',   '599', 'OLIVIA',  ''),
    ('OLIVIA 4/250',   '599', 'OLIVIA',  ''),
    ('OLIVIA 8/250',   '599', 'OLIVIA',  ''),
    ('OLIVIA 8/500',   '599', 'OLIVIA',  ''),
    ('OLIVIA 16/500',  '599', 'OLIVIA',  ''),
    ('OLIVIA 16/1000', '599', 'OLIVIA',  ''),
    ('OLIVIA 32/1000', '599', 'OLIVIA',  ''),
    ('OPERA-BEACON',   '599', 'OPERA',   ''),
    ('OPERA-QSO',      '599', 'OPERA',   ''),
    ('PAC2',           '599', 'PAC',     ''),
    ('PAC3',           '599', 'PAC',     ''),
    ('PAC4',           '599', 'PAC',     ''),
    ('PAX2',           '599', 'PAX',     ''),
    ('PCW',            '599', 'CW',      ''),
    ('PSK10',          '599', 'PSK',     ''),
    ('PSK31',          '599', 'PSK',     ''),
    ('PSK63',          '599', 'PSK',     ''),
    ('PSK63F',         '599', 'PSK',     ''),
    ('PSK63RC10',      '599', 'PSK',     ''),
    ('PSK63RC20',      '599', 'PSK',     ''),
    ('PSK63RC32',      '599', 'PSK',     ''),
    ('PSK63RC4',       '599', 'PSK',     ''),
    ('PSK63RC5',       '599', 'PSK',     ''),
    ('PSK125',         '599', 'PSK',     ''),
    ('PSK125RC10',     '599', 'PSK',     ''),
    ('PSK125RC12',     '599', 'PSK',     ''),
    ('PSK125RC16',     '599', 'PSK',     ''),
    ('PSK125RC4',      '599', 'PSK',     ''),
    ('PSK125RC5',      '599', 'PSK',     ''),
    ('PSK250',         '599', 'PSK',     ''),
    ('PSK250RC2',      '599', 'PSK',     ''),
    ('PSK250RC3',      '599', 'PSK',     ''),
    ('PSK250RC5',      '599', 'PSK',     ''),
    ('PSK250RC6',      '599', 'PSK',     ''),
    ('PSK250RC7',      '599', 'PSK',     ''),
    ('PSK500',         '599', 'PSK',     ''),
    ('PSK500RC2',      '599', 'PSK',     ''),
    ('PSK500RC3',      '599', 'PSK',     ''),
    ('PSK500RC4',      '599', 'PSK',     ''),
    ('PSK800RC2',      '599', 'PSK',     ''),
    ('PSK1000',        '599', 'PSK',     ''),
    ('PSK1000RC2',     '599', 'PSK',     ''),
    ('PSKAM10',        '599', 'PSK',     ''),
    ('PSKAM31',        '599', 'PSK',     ''),
    ('PSKAM50',        '599', 'PSK',     ''),
    ('PSKFEC31',       '599', 'PSK',     ''),
    ('PSKHELL',        '599', 'HELL',    ''),
    ('QPSK31',         '599', 'PSK',     ''),
    ('Q65',            '599', 'MFSK',    ''),
    ('QPSK63',         '599', 'PSK',     ''),
    ('QPSK125',        '599', 'PSK',     ''),
    ('QPSK250',        '599', 'PSK',     ''),
    ('QPSK500',        '599', 'PSK',     ''),
    ('QRA64A',         '599', 'QRA64',   ''),
    ('QRA64B',         '599', 'QRA64',   ''),
    ('QRA64C',         '599', 'QRA64',   ''),
    ('QRA64D',         '599', 'QRA64',   ''),
    ('QRA64E',         '599', 'QRA64',   ''),
    ('ROS-EME',        '599', 'ROS',     ''),
    ('ROS-HF',         '599', 'ROS',     ''),
    ('ROS-MF',         '599', 'ROS',     ''),
    ('SIM31',          '599', 'PSK',     ''),
    ('SITORB',         '599', 'TOR',     ''),
    ('SLOWHELL',       '599', 'HELL',    ''),
    ('THOR-M',         '599', 'THOR',    ''),
    ('THOR4',          '599', 'THOR',    ''),
    ('THOR5',          '599', 'THOR',    ''),
    ('THOR8',          '599', 'THOR',    ''),
    ('THOR11',         '599', 'THOR',    ''),
    ('THOR16',         '599', 'THOR',    ''),
    ('THOR22',         '599', 'THOR',    ''),
    ('THOR25X4',       '599', 'THOR',    ''),
    ('THOR50X1',       '599', 'THOR',    ''),
    ('THOR50X2',       '599', 'THOR',    ''),
    ('THOR100',        '599', 'THOR',    ''),
    ('THRBX',          '599', 'THRB',    ''),
    ('THRBX1',         '599', 'THRB',    ''),
    ('THRBX2',         '599', 'THRB',    ''),
    ('THRBX4',         '599', 'THRB',    ''),
    ('THROB1',         '599', 'THRB',    ''),
    ('THROB2',         '599', 'THRB',    ''),
    ('THROB4',         '599', 'THRB',    ''),
    ('USB',            '59',  'SSB',     ''),
    ('FM',             '59',  'FM',      ''),
    ('AM',             '59',  'AM',      ''),
    ('CW',             '599', 'CW',      ''),
    ('RTTYM',          '599', 'RTTYM',   ''),
    ('SSTV',           '599', 'SSTV',    ''),
    ('WSPR',           '599', 'WSPR',    ''),
    ('MT63',           '599', 'MT63',    ''),
    ('MSK144',         '599', 'MSK144',  ''),
    ('FT8',            '599', 'FT8',     ''),
    ('FSK441',         '599', 'FSK441',  ''),
    ('C4FM',           '59',  'C4FM',    ''),
]


def import_modes(conn, cursor):
    """Create the mode table and populate it with known amateur radio modes."""
    cursor.execute('DROP TABLE IF EXISTS mode')
    cursor.execute('''
        CREATE TABLE mode (
            name        TEXT,
            default_rst TEXT DEFAULT "599",
            super_mode  TEXT DEFAULT "",
            comment     TEXT DEFAULT ""
        )
    ''')

    try:
        cursor.executemany(
            'INSERT INTO mode (name, default_rst, super_mode, comment) VALUES (?, ?, ?, ?)',
            MODES
        )
    except sqlite3.Error as e:
        print(f"Error inserting mode rows: {e}")
        raise

    print(f"Mode table populated with {len(MODES)} entries.")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    args = parse_args()
    db_path = args.output

    pathlib.Path(db_path).parent.mkdir(parents=True, exist_ok=True)

    # --- SOTA summits ---
    download_sota_csv(CSV_URL, CSV_FILE)

    # --- POTA parks ---
    download_pota_csv(POTA_CSV_URL, POTA_CSV_FILE)

    # --- WWFF areas ---
    download_wwff_csv(WWFF_CSV_URL, WWFF_CSV_FILE)

    conn, cursor = open_db(db_path)
    try:
        import_sota_summits(conn, cursor, CSV_FILE)
        import_pota_parks(conn, cursor, POTA_CSV_FILE)
        import_wwff_areas(conn, cursor, WWFF_CSV_FILE)
        import_modes(conn, cursor)
    finally:
        close_db(conn)

    for tmp in (CSV_FILE, POTA_CSV_FILE, WWFF_CSV_FILE):
        if os.path.exists(tmp):
            os.remove(tmp)
            print(f"Removed intermediate file {tmp}.")

    print(f"Database {db_path} built successfully.")


if __name__ == '__main__':
    main()
