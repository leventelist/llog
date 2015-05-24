#!/bin/bash

SQL=summits.sql
OUT=summits.sqlite

rm -f $OUT
cat $SQL | sqlite3 -batch $OUT

