<?php
/*
Copyright (©) 2003-2013 Teus Benschop.

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
page_access_level (MANAGER_LEVEL);
Assets_Page::header (gettext ("Versifications"));
$view = new Assets_View (__FILE__);
$database_versifications = Database_Versifications::getInstance();


if (isset ($_GET['delete'])) {
  $name = $_GET['delete'];
  $confirm = $_GET['confirm'];
  if ($confirm != "") {
    $database_versifications->delete ($name);
  } else {
    $dialog_yes = new Dialog_Yes (NULL, gettext ("Would you like to delete this versification system?"), "delete");
    die;
  }
}


if (isset($_POST['new'])) {
  $name = $_POST['entry'];
  $database_versifications->create ($name);
}


if (isset ($_GET['new'])) {
  $dialog_entry = new Dialog_Entry ("", gettext ("Please enter the name for the new versification system"), "", "new", "");
  die;
}


$systems = $database_versifications->getSystems();
$view->view->systems = $systems;
$view->render ("index.php");
Assets_Page::footer ();


?>
