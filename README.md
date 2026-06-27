# llog

A minimalist logging application for HAM radio operators, with a focus on outdoor operations.
That include support for
1. [SOTA](https://www.sota.org.uk/) Summits on the Air,
1. [POTA](https://parksontheair.com/) Parks on the Air, and
1. [WWFF](https://wwff.co/) World Wide Flora and Fauna
operations.

I personally use this for everyday fixed station logging.

## Motivation

Inspired by [Ham2K](https://play.google.com/store/apps/details?id=com.ham2k.polo.beta&hl=en-US)
for mobile devices, `llog` is designed for operators who carry a laptop to the
summit — no smartphone required. Connect a GPS receiver, run GPSd, and `llog`
will find your location automatically, identify the nearest SOTA/POTA/WWFF location, and
let you log contacts with minimal effort.

I looked at other log softwares, they are good, but they were very complicated. Simply, you just can't
afford a complex software, when you are at the top of the summit. What you need is a simple, very easy
to use application.

All log data is stored in a local SQLite database. I usually keep all my logfiles in a git
repository, so I can move around computers. `llog` is designed to work well with this user-case. The log
file is kept where you want. This make it easy to integrate into your working order.

---

## Features

- **Automatic location detection** via GPSd
- **Nearest SOTA/POTA/WWFF identification** and one-click reference insertion
- **Splash screen on startup** while the database initialises
- **SQLite logging** with a clean, queryable schema
- **Duplicate QSO detection** with visual warning
- **QRZ lookup** — click the Call button to open the browser
- **FLDIGI integration** via XML-RPC (Get button populates fields from FLDIGI)
- **ADI, ADX, CSV export**
- **Auxiliary database rebuild** from Edit menu
- **GTK4 interface** with resizable column view of logged contacts

---

## Prerequisites

### Build dependencies

```
libsqlite3-dev
libgtk-4-dev
libgps-dev
libhamlib-dev
libxml2-dev
libxmlrpc-core-c3-dev
libcurl4-openssl-dev
```

### Runtime dependencies

```
gpsd
gpsd-tools
gpsd-clients
sqlitebrowser    (for Edit → Log database)
python3 + sqlite3 module
```

### Internet connection

`llog` fetches the current SOTA/POTA/WWFF references when
1. running at the very first time,
1. invoked with th `-s` command line option, or
1. requested from the GUI.

---

## Building and installing

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
sudo make install
```

---

## Getting started

1. Launch `llog`. A splash screen is shown while the database initialises.
1. Create a new log file via **File → New**.
1. Set up your station via **Edit → Log database** — this opens sqlitebrowser.
   Add your station details, save, then reload via **File → Reload**.
1. If your SOTA summit reference database ever becomes stale, rebuild it via
   **Edit → Rebuild aux database**.

**!!!CAUTION!!!**

Launch `llog` before you go to the field. With the first run, it
generates a database for static data. If you miss this step,
you'll end up an empty mode list, and the summit references will
also be missing.

You should explicitly request `llog` to update its database from time to time.

---

## Logging a contact

| Action | How |
|---|---|
| Get current UTC | Click the **UTC** button |
| Insert nearest SOTA summit | Click the **Summit ref** button |
| Look up a callsign on QRZ | Click the **Call** button |
| Import data from FLDIGI | Click the **Get** button |
| Save the contact | Click the **Log** button |

Fields that are not cleared after logging (QRG, mode, power, summit ref) are
intentionally kept so you don't have to re-enter them between contacts.

---

## Command line options

| Option | Description |
|---|---|
| `-f <file>` | Set the log database file |
| `-s` | Force rebuild the auxiliary (SOTA/POTA/WWFF) database on startup |
| `-v` | Print version and exit |
| `-h` | Print help and exit |

---

## License

Copyright (C) 2013–2026 Levente Kovacs — HA5OGL

Released under the [GNU General Public License v3](https://www.gnu.org/licenses/gpl-3.0.html).

Patches and improvements are welcome.
