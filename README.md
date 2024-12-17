# llog
Log software for HAM operators

Inspired by [Ham2K](https://play.google.com/store/apps/details?id=com.ham2k.polo.beta&hl=en-US) for mobile devices.

This is mainly targets SOTA operators. The user case is that you have
your computer with you, and nothing else. Not even a smartphone.

## Prerequisites

* libsqlite3-dev
* libgtk-4-dev
* libgps-dev
* libhamlib-dev
* python3
    - SQLite3
    - csv
    - requests
* hamutils python3 package
* libxmlrpc-core-c3-dev
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

You can always edit your database with sqlitebrowser.

Patches, improvements are welcome.
