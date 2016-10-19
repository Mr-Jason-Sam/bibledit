/*
Copyright (©) 2003-2016 Teus Benschop.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include <database/statistics.h>
#include <filter/url.h>
#include <filter/string.h>
#include <database/sqlite.h>


// Database resilience: It contains statistical and non-essential data.
// It is checked and optionally recreated at least once a day.


void Database_Statistics::create ()
{
  SqliteDatabase sql = SqliteDatabase (name ());
  sql.add ("CREATE TABLE IF NOT EXISTS changes (timestamp integer, user text, count integer);");
  sql.execute ();
}


void Database_Statistics::optimize ()
{
  bool healthy_database = database_sqlite_healthy (name ());
  if (!healthy_database) {
    filter_url_unlink (database_sqlite_file (name ()));
    create ();
  }
}


void Database_Statistics::store_changes (int timestamp, string user, int count)
{
  SqliteDatabase sql = SqliteDatabase (name ());
  sql.add ("INSERT INTO changes VALUES (");
  sql.add (timestamp);
  sql.add (",");
  sql.add (user);
  sql.add (",");
  sql.add (count);
  sql.add (");");
  sql.execute ();
}


// Fetches the distinct users from the database.
vector <string> Database_Statistics::get_users ()
{
  SqliteDatabase sql = SqliteDatabase (name ());
  sql.add ("SELECT DISTINCT user FROM changes ORDER BY user;");
  vector <string> users = sql.query () ["user"];
  return users;
}


// Fetches the change statistics from the database for $user.
map <int, int> Database_Statistics::get_changes (string user)
{
  map <int, int> changes;
  SqliteDatabase sql = SqliteDatabase (name ());
  sql.add ("SELECT timestamp, count FROM changes WHERE user =");
  sql.add (user);
  sql.add ("ORDER BY timestamp DESC;");
  vector <string> timestamps = sql.query () ["timestamp"];
  vector <string> counts = sql.query () ["count"];
  for (size_t i = 0; i < timestamps.size (); i++) {
    int timestamp = convert_to_int (timestamps[i]);
    int count = convert_to_int (counts[i]);
    changes [timestamp] = count;
  }
  return changes;
}


const char * Database_Statistics::name ()
{
  return "statistics";
}
