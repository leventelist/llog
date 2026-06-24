# llog

A minimalist logging application for HAM radio operators, with a focus on
[SOTA](https://www.sota.org.uk/) (Summits on the Air) activations.


## Motivation

Inspired by [Ham2K](https://play.google.com/store/apps/details?id=com.ham2k.polo.beta&hl=en-US)
for mobile devices, llog is designed for operators who carry a laptop to the
summit — no smartphone required. Connect a GPS receiver, run GPSd, and llog
will find your location automatically, identify the nearest SOTA summit, and
let you log contacts with minimal effort.

All log data is stored in a local SQLite database.

I looked at other log softwares, they are good, but they were very complicated. Simply, you just can't
afford a complex software, when you are at the top of the summit. What you need is a simple, very easy
to use application.

---

## Features

- **Automatic location detection** via GPSd
- **Nearest SOTA summit identification** and one-click reference insertion
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

### Internet connection at build time

llog fetches the current SOTA summit references during the build. An internet
connection is required when running `cmake`.

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

1. Launch llog. A splash screen is shown while the database initialises.
2. Create a new log file via **File → New**.
3. Set up your station via **Edit → Log database** — this opens sqlitebrowser.
   Add your station details, save, then reload via **File → Reload**.
4. If your SOTA summit reference database ever becomes stale, rebuild it via
   **Edit → Rebuild aux database**.

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
| `-s` | Rebuild the auxiliary (SOTA) database on startup |
| `-v` | Print version and exit |
| `-h` | Print help and exit |

---

## License

Copyright (C) 2013–2025 Levente Kovacs — HA5OGL

Released under the [GNU General Public License v3](https://www.gnu.org/licenses/gpl-3.0.html).

Patches and improvements are welcome.
