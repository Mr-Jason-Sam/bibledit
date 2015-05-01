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


#include <config/libraries.h>


extern string config_globals_document_root;
extern bool config_globals_unit_testing;
extern bool config_globals_open_installation;
extern bool config_globals_client_prepared;
extern bool config_globals_running;
extern thread * config_globals_worker;
extern thread * config_globals_timer;
extern bool config_globals_mail_receive_running;
extern bool config_globals_mail_send_running;
extern bool config_globals_quit_at_midnight;
extern int config_globals_touch_enabled;
extern int config_globals_timezone_offset_utc;