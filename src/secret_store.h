/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2026  Levente Kovacs
 *
 *	This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * http://levente.logonex.eu
 * ha5ogl.levente@gmail.com
 */

#ifndef SECRET_STORE_H
#define SECRET_STORE_H

#include <stdbool.h>
#include <stddef.h>

/*Passwords and API keys kept in the desktop keyring (Secret Service) via libsecret.
 * Every call returns false when the keyring can not be reached, or when llog was built
 * without libsecret, so the caller can fall back to the plain text config file.*/

/*Looks up the secret named key. On success value holds it, or is empty if the keyring has no such item.*/
bool secret_store_load(const char *key, char *value, size_t len);

/*Stores value under key. An empty value removes the item from the keyring.*/
bool secret_store_save(const char *key, const char *value);

#endif
