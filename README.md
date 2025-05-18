# llog
Log software for HAM operators

Inspired by [Ham2K](https://play.google.com/store/apps/details?id=com.ham2k.polo.beta&hl=en-US) for mobile devices.

This is mainly targets SOTA operators. The user case is that you have
your computer with you, and nothing else. Not even a smartphone. You have your GPS receiver connected, and GPSd set up.
The program connects to gpsd, and knows where you are. The closest SOTA summit is selected, and you can save your location
just by one click.

There is an SQLite database where all logged information is saved.

## Features
* Automatic location detection via GPSd
* Closest SOTA summit identification
* One-click location logging
* SQLite database for storing logs
* Integration with QRZ for call sign lookup
* User-friendly interface with GTK4
* ADI, ADX, CSV export
* FLDIGI integration using XMLRPC

## Prerequisites

* libsqlite3-dev
* libgtk-4-dev
* libgps-dev
* libhamlib-dev
* libxml2-dev
* libxmlrpc-core-c3-dev
* libcurl4-openssl-dev
* sqlitebrowser
* gpsd
* gpsd-tools
* gpsd-clients
* python3
    - sqlite3

## Install

\$ mkdir build

\$ cd build

\$ cmake ..

\$ make -j16

\# make install

## Usage

* Launch llog, and then create a new database by clicking File->New.
* Edit your station by clicking Edit->Log database. This will fire up sqlitebrowser.
* Once you are happy with your changes, reload the database by clicking
File->Reload
* CLick on the UTC button to get current UTC
* Click on the 'Summit ref' button the get the closest SOTA reference
* Fill out all the info you have
* Click the Log button. All data will be saved to the database
* Click on the Call button, and the default browser will be launched with a QRZ query
* Click on the Get button to get information from FLDIGI
* Enjoy!


Patches, improvements are welcome.
