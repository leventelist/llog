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
        print(query)
        self.db_cursor.execute(query)
        return self.db_cursor.fetchall()

    def get_stations(self, pattern = None):
        print("Returning stations")
        if pattern != None:
            query = "SELECT rowid, * FROM station WHERE rowid={0}".format(pattern)
        else:
            query = "SELECT rowid, * FROM station;"
        print(query)
        self.db_cursor.execute(query)
        return self.db_cursor.fetchall()
