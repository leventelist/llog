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
- **WSJT-X integration** via its UDP messages, with optional automatic logging
- **ADI, ADX, CSV export**
- **Upload to World Radio League** via its API, without sending a QSO twice
- **Upload to LoTW** through TQSL
- **Upload to eQSL.cc**, without sending a QSO twice
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
libjson-c-dev
libsecret-1-dev  (optional, see "Passwords and API keys")
```

### Runtime dependencies

```
gpsd
gpsd-tools
gpsd-clients
sqlitebrowser    (for Edit → Log database)
trustedqsl       (TQSL, for Upload → LoTW)
python3 + sqlite3 module
gnome-keyring    (or any other Secret Service provider, e.g. KWallet; optional)
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

You need Internet access for the aux database rebuild.

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

## Logging from WSJT-X

llog listens for the UDP messages of WSJT-X (and compatible programs, such as
JTDX). It can be used together with the FLDIGI integration.

1. In WSJT-X, open **File → Settings → Reporting**. Set **UDP Server** to
   `127.0.0.1` and **UDP Server port number** to `2237` (the defaults).
2. In llog, open **Edit → Preferences** and make sure **WSJT-X listener** is
   ticked, with the same address and port.

While you work a station, llog follows WSJT-X: the QRG, mode, call, grid and
sent report are updated when they change in WSJT-X. Fields WSJT-X has not
changed are left alone, so you can still edit them.

When you log the QSO in WSJT-X, llog fills in the entries from it, including
the QSO start time and both reports. Press **Log** to save it, or tick
**WSJT-X auto log** in Preferences to have it saved right away.

Only one program receives a unicast UDP port. To share WSJT-X with GridTracker
or JTAlert, use a multicast address (for example `239.255.0.1`) in WSJT-X and
in every listening program.

---

## Uploading to World Radio League

1. In World Radio League, generate an API key under **Integrations → Developer API**.
1. Open **Upload → World Radio League**, paste the key and click **Check key**.
   This shows your default logbook and lists your logbooks.
1. Optionally enter a **Logbook ID** (otherwise your default logbook is used) and a
   **From date** (`YYYY-MM-DD`) to upload only recent QSOs.
1. Click **Upload**.

If the Logbook ID is empty, llog looks up your default logbook at the start of every upload
and saves it in its configuration file. If the lookup fails, the saved one is used. If you
have several logbooks and none is set as the default, enter a Logbook ID.

The API takes one QSO per request and at most 60 per minute, so an upload takes
about one second per QSO. llog records every uploaded QSO in the `upload` table of the
log file and never sends it again. QSOs that WRL rejects (for example an unknown
SOTA/POTA/WWFF reference) are listed in the window and are retried on the next upload.

The API key is stored in your desktop keyring, see [Passwords and API keys](#passwords-and-api-keys).

## Uploading to LoTW

LoTW only accepts logs signed with your callsign certificate, so llog uses ARRL's TQSL
program (Debian package `trustedqsl`) to sign and upload them.

1. In TQSL, install your callsign certificate and create a station location.
1. Open **Upload → LoTW** and enter the station location's name exactly as in TQSL.
   If `tqsl` is not on your `PATH`, enter its full path as the TQSL program.
1. Optionally enter a **From date** (`YYYY-MM-DD`), then click **Upload**.

llog writes the QSOs to a temporary ADIF file and runs
`tqsl -x -d -u -a compliant -l <station location> <file>`. TQSL's output is shown in
the window. TQSL reports a single result for the whole file, so when it succeeds, or
only skips QSOs that are duplicates or outside the certificate's date range, every QSO
in the file is recorded as uploaded. If it fails, nothing is recorded and the next
upload sends the same QSOs again. llog does not pass a certificate password to TQSL,
so an unprotected certificate is the simplest setup.

## Uploading to eQSL.cc

1. Open **Upload → eQSL.cc** and enter your eQSL.cc user name (callsign) and password.
1. If your eQSL.cc account has several QTHs, enter the **QTH nickname** to upload to.
1. Optionally enter a **From date** (`YYYY-MM-DD`), then click **Upload**.

Each QSO is uploaded on its own, oldest first, with its callsign, date, time, band,
frequency, mode (as ADIF mode and submode), sent RST and power. A QSO is recorded as
uploaded when eQSL.cc accepts it or reports it as a duplicate, so it is not sent again.
QSOs eQSL.cc rejects (e.g. a date your account does not cover) are listed in the window
and retried on the next upload. A wrong user name or password, a network error or eQSL.cc
maintenance stops the upload.

The password is stored in your desktop keyring, see [Passwords and API keys](#passwords-and-api-keys).

---

## Passwords and API keys

llog keeps two secrets: the World Radio League API key (`wrl_api_key`) and the eQSL.cc
password (`eqsl_password`). They are stored in the desktop keyring through the
[Secret Service](https://specifications.freedesktop.org/secret-service/) API (libsecret),
the same place where your browser and mail client keep their passwords. GNOME Keyring and
KWallet both provide it. The keyring is encrypted and is unlocked when you log in.

Only when no keyring can be reached are the secrets written to the configuration file
`~/.config/llog/llog.cf` in plain text. The configuration file is always created with,
or tightened to, owner-only permissions (`0600`).

There is no "encrypted password in the config file" mode. llog is open source, so any
key built into the program is public, and a password encrypted with it would only be
obfuscated, not protected.

### In the keyring

Each secret is one keyring item with the schema `eu.logonex.llog.Secret` and the attribute
`key` set to the config option name. In Seahorse ("Passwords and Keys") they show up as
`llog wrl_api_key` and `llog eqsl_password`. You can view, change or delete them there,
or with `secret-tool`:

```bash
secret-tool lookup key eqsl_password
secret-tool clear key eqsl_password
```

llog reads the keyring only at startup, so restart it after changing an item outside llog.

### Loading at startup

For each secret, in this order:

1. If the configuration file holds a value, that value is used. This happens with a
   configuration file written by an older llog, or while no keyring was available.
1. Otherwise llog looks the secret up in the keyring. If there is no such item, the
   secret is empty.
1. If the keyring can not be reached (no Secret Service running, no D-Bus session,
   llog built without libsecret, or you dismissed the unlock prompt), the secret is
   empty and llog does not try the keyring again until it is restarted.

### Saving

Whenever llog saves its configuration (for example when you click **Upload** or **Check key**),
for each secret:

1. If the secret is unchanged since it was read from or written to the keyring, nothing
   is done, so the keyring is not contacted on every save.
1. If it changed, it is written to the keyring, and the configuration file gets an empty
   value. An emptied secret is removed from the keyring.
1. If the keyring refuses it, the secret is written to the configuration file in plain
   text, and a warning is printed on the terminal.
1. If the keyring could not be read at startup and the secret is empty, llog leaves the
   keyring alone. An unreadable keyring never makes llog delete an item stored in it.

### Moving from plain text to the keyring

No action is needed. A secret found in the configuration file is moved to the keyring
the first time llog saves its configuration, which happens right after startup, and the
configuration file no longer contains it. The same happens on the next start after a
session in which the keyring was unavailable.

### Without a keyring

On a minimal desktop or a headless machine with no Secret Service, llog works the same as
before: the secrets live in the configuration file in plain text, protected only by the
file's `0600` permissions. If `libsecret-1-dev` is missing at build time, CMake prints a
warning and llog is built without keyring support.

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
