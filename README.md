# llog
Log software for HAM operators

## Prerquisites

* libsqlite3-dev
* libgtk-3-dev
* python3
* hamutils python3 package
* sqlitebrowser

## Install

\$ mkdir build

\$ cd build

\$ cmake ..

\$ make -j9

\# make install

## Usage

1. generate an empty log database, go to the db directory, and issue \
    \$ create_db.sh
2. Copy the created file wherever convenient.
3. Open the database with sqlitebrowser. Create a new station in the station table.
4. Run llog -f /your/log/path/database.sqlite.
5. Next time you can run llog without any argument, and it'll open your database
6. Profit
