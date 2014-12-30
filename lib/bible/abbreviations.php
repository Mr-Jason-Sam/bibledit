<?php
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


require_once ("../bootstrap/bootstrap.php");
page_access_level (Filter_Roles::translator ());


Assets_Page::header (gettext("Abbreviations"));
$view = new Assets_View (__FILE__);


$database_config_bible = Database_Config_Bible::getInstance();


$success_message = "";
$error_message = "";


// The name of the Bible.
$bible = access_bible_clamp ($_GET['bible']);
$view->view->bible = filter_string_sanitize_html ($bible);


// Data submission.
if (isset($_POST['submit'])) {
  $data = $_POST['data'];
  $data = trim ($data);
  if ($data != "") {
    if (Validate_Utf8::valid ($data)) {
      $database_config_bible->setBookAbbreviations ($bible, $data);
      $success_message = gettext("The abbreviations were saved.");
    } else {
      $error_message = gettext("Please supply valid Unicode UTF-8 text.");
    }
  } else {
    $success_message = gettext("Nothing was saved.");
  }
}


$data = $database_config_bible->getBookAbbreviations ($bible);
$data = Filter_Abbreviations::display ($data);
$view->view->data = $data;


$view->view->success_message = $success_message;
$view->view->error_message = $error_message;


$view->render ("abbreviations.php");


Assets_Page::footer ();


?>
