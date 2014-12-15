<?php
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

require_once ("../bootstrap/bootstrap.php");
page_access_level (Filter_Roles::manager ());
Assets_Page::header (gettext("Import"));
set_time_limit (0);
$folder = $_GET ['folder'];
if (file_exists ($folder)) {
  $workingdirectory = dirname (__FILE__);
  $command = "php $workingdirectory/importcli.php $folder";
  Tasks_Logic::queue (Tasks_Logic::PHP, array ("$workingdirectory/importcli.php", $folder));
  $view = new Assets_View (__FILE__);
  $view->render ("import4.php");
} else {
  Assets_Page::error (gettext("Could not find the folder with the consultation notes"));
}
Assets_Page::footer ();

?>
