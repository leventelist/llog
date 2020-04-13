#!/bin/sh

create_database() {

	rm -f $2
	cat $1 | sqlite3 -batch $2

}

create_database llog.sql llog.sqlite

