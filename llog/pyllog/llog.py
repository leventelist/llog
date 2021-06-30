import sqlite3

class Llog():
    """llog class."""

    def __init__(self, arg):
        self.log_database_file = arg
        self.db_cursor = 1
        self.db_conn = 2

    def open_database(self):
        print("\nOpening database " + self.log_database_file + "\n")
        self.db_conn = sqlite3.connect(self.log_database_file)
        self.db_conn.row_factory = sqlite3.Row
        self.db_cursor = self.db_conn.cursor()

    def close_database(self):
        self.db_cursor.close()

    def get_qsos(self, pattern = None):
        print("Returning QSOs.")
        if pattern != None:
            query = "SELECT rowid, * FROM log WHERE {0}".format(pattern)
        else:
            query = "SELECT rowid, * FROM log;"
        self.db_cursor.execute(query)
        return self.db_cursor.fetchall()

    def get_stations(self, pattern = None):
        print("Returning stations")
        if pattern != None:
            query = "SELECT rowid, * FROM station WHERE rowid={0}".format(pattern)
        else:
            query = "SELECT rowid, * FROM station;"
        self.db_cursor.execute(query)
        return self.db_cursor.fetchall()

    def get_mode(self, mode = None):
        print("Returning modes")
        if mode != None:
            query = "SELECT rowid, * FROM mode WHERE mode={0}".format(mode)
        else:
            query = "SELECT rowid, * FROM mode;"
        self.db_cursor.execute(query)
        return self.db_cursor.fetchall()

    def get_band_from_freqency(self,freqency):
        if 0.136 < freqency and freqency < 0.137:
            bnad = '2190m'
        elif 0.501 < freqency and freqency < 0.504:
            band = '560m'
        elif 1.8 < freqency and freqency <2.0:
            band = '160m'
        elif 3.5 < freqency and freqency <4.0:
            band = '80m'
        elif 5.102 < freqency and freqency <5.404:
            band = '60m'
        elif 7.0 < freqency and freqency <7.3:
            band = '40m'
        elif 10.0 < freqency and freqency < 10.15:
            bnad = '30m'
        elif 14.0 < freqency and freqency < 14.35:
            band = '20m'
        elif 18.068 < freqency and freqency < 18.168:
            band = '17m'
        elif 21.0 < freqency and freqency < 21.45:
            band = '15m'
        elif 24.890 < freqency and freqency < 24.99:
            band = '12m'
        elif 28.0 < freqency and freqency < 29.7:
            band = '10m'
        elif 50 < freqency and freqency < 54:
            band = '6m'
        elif 70 < freqency and freqency < 71:
            band = '4m'
        elif 144 < freqency and freqency <148:
            band = '2m'
        elif 222 < freqency and freqency <225:
            band = '1.25m'
        elif 420 < freqency and freqency <450:
            band = '70cm'
        elif 902 < freqency and freqency < 928:
            band = '33cm'
        elif 1240 < freqency and freqency < 1300:
            band = '23cm'
        elif 2300 < freqency and freqency < 2450:
            band = '13cm'
        elif 3300 < freqency and freqency <3500:
            band = '9cm'
        elif 5650 < freqency and freqency <5925:
            band = '6cm'
        elif 10000 < freqency and freqency <10500:
            band = '3cm'
        elif 24000 < freqency and freqency < 24250:
            band = '1.25cm'
        elif 47000 < freqency and freqency < 47200:
            band = '6mm'
        elif 75500 < freqency and freqency < 81000:
            band = '4mm'
        elif 119980 < freqency and freqency <120020:
            band = '2.5mm'
        elif 142000 < freqency and freqency <149000:
            band = '2mm'
        elif 241000 < freqency and freqency <250000:
            band = '1mm'
        else:
            band = None

        return band
