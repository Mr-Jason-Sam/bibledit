/*
Copyright (©) 2003-2015 Teus Benschop.

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


#include <config/logic.h>
#include <filter/string.h>
#include <filter/url.h>
#include <database/books.h>
#include <database/config/general.h>
#include <config.h>


// Returns the Bibledit version number.
const char * config_logic_version ()
{
  return VERSION;
}


// Return the network port configured for the server.
const char * config_logic_network_port ()
{
  return NETWORK_PORT;
}


// Returns whether client mode is enabled during configure.
bool config_logic_client_prepared ()
{
#ifdef CLIENT_PREPARED
  return true;
#endif
  return false;
}


// Returns whether demo mode is enabled during configure.
bool config_logic_demo_enabled ()
{
  return (strcmp (DEMO, "yes") == 0);
}


// Returns the maximum number of parallel tasks to run.
int config_logic_max_parallel_tasks ()
{
  return PARALLEL_TASKS;
}


// Returns whether bare browser mode is enable during configure.
bool config_logic_bare_browser ()
{
  return (strcmp (BARE_BROWSER, "yes") == 0);
}
