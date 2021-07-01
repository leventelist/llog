#!/bin/python3

import llog

def main():
    llogd = llog.Llog("log.sqlite")
    llogd.open_database()
    qsos = llogd.get_qsos("call='R2AIJ'")
    for qso in qsos:
        print(qso['rowid'], qso['date'] + " " + qso['UTC'] + " " + qso['call'])
    qsos = llogd.get_qsos()
    for qso in qsos:
        print(qso['rowid'], qso['date'] + " " + qso['UTC'] + " " + qso['call'])

    stations = llogd.get_stations()
    for station in stations:
        print(str(station['rowid']) + " " + station['name'] + " " + station["CALL"])
    stations = llogd.get_stations(1)
    for station in stations:
        print(str(station['rowid']) + " " + station['name'] + " " + station["CALL"])

    llogd.close_database();

if __name__ == '__main__':
    main()
