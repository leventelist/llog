#!/usr/bin/python3

import llog
from hamutils.adif import ADIWriter
import argparse
import datetime


def main():

    parser = argparse.ArgumentParser(description='This script creates an adif file from llog database')
    parser.add_argument('-d', '--db_file', help='filename of the database', action='store', required=True)
    parser.add_argument('-o', '--out_file', help='filename of the output adif', action='store', required=True)
    parser.add_argument('-p', '--pattern', help='SQL pattern to filter QSOs', action='store')
    args = parser.parse_args()

    llogd = llog.Llog(args.db_file)
    llogd.open_database()

    if args.pattern == None:
        qsos = llogd.get_qsos()
    else:
        qsos = llogd.get_qsos(args.pattern)

    #Open the ADIF for writing
    adif = open(args.out_file, 'wb')
    adi = ADIWriter(adif)

    for qso in qsos:
        qso_time = qso['UTC'][:2] + ":" + qso['UTC'][2:]
        date = datetime.datetime.fromisoformat(qso['date'] + "T" + qso_time)

        qso_band = llogd.get_band_from_freqency(qso['QRG'])

        stations = llogd.get_stations(qso['station'])
        station = stations[0]

        sub_mode = qso['mode']
        modes = llogd.get_mode(sub_mode)
        mode = modes[0]

        adi.add_qso(call = qso['call'], datetime_on = date, mode = mode['super_mode'], name = qso['name'], band = qso_band, freq = qso['QRG'], my_gridsquare = station['QRA'], my_rig = station['rig'], qth = qso['QTH'], STATION_CALLSIGN = station['CALL'], RST_RCVD = qso['rxrst'], RST_SENT = qso['txrst'], GRIDSQUARE = qso['QRA'], OPERATOR = station['OPERATOR_CALL'])

    adi.close()
    llogd.close_database()

if __name__ == '__main__':
    main()