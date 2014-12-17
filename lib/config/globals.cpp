/*
Copyright (©) 2003-2014 Teus Benschop.

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


#include <config/globals.h>
#include <config.h>


using namespace std;


string config_globals_document_root = DIRECTORY_SEPARATOR;
bool config_globals_unit_testing = false;
bool config_globals_open_installation = false;
bool config_globals_client_prepared = false;
bool config_globals_running = true;
thread * config_globals_worker = NULL;
thread * config_globals_timer = NULL;
bool config_globals_setup_running = false;
bool config_globals_mail_receive_running = false;
bool config_globals_mail_send_running = false;
