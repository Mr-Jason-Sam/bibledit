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


$database_config_user = Database_Config_User::getInstance ();
$database_config_bible = Database_Config_Bible::getInstance ();
$database_bibles = Database_Bibles::getInstance ();


@$switchbook = $_GET ['switchbook'];
@$switchchapter = $_GET ['switchchapter'];
if (isset ($switchbook) && isset ($switchchapter)) {
  $ipc_focus = Ipc_Focus::getInstance();
  $switchbook = Filter_Numeric::integer_in_string ($switchbook);
  $switchchapter = Filter_Numeric::integer_in_string ($switchchapter);
  $ipc_focus->set ($switchbook, $switchchapter, 1);
  Navigation_Passage::recordHistory ($switchbook, $switchchapter, 1);
}


$header = new Assets_Header (gettext("Edit USFM"));
$header->setNavigator ();
$header->run ();


@$changebible = $_GET['changebible'];
if (isset ($changebible)) {
  if ($changebible == "") {
    $dialog_list = new Dialog_List2 (gettext("Select which Bible to open in the editor"));
    $bibles = access_bible_bibles ();
    for ($bibles as $bible) {
      $dialog_list->add_row ($bible, "&changebible=$bible");
    }
    $dialog_list->run();
  } else {
    $database_config_user->setBible ($changebible);
  }
}


$view = new Assets_View (__FILE__);


// Get active Bible, and check read access to it.
// If needed, change Bible to one it has read access to.
$bible = access_bible_clamp ($database_config_user->getBible ());
$view->view->bible = $bible;


// Write access?
$write_access = access_bible_write ($bible);
$view->view->write_access = $write_access;


// Store the active Bible in the page's javascript.
$view->view->navigationCode = Navigation_Passage::code ($bible);


$chapterLoaded = gettext("Loaded");
$chapterSaving = gettext("Saving...");
$chapterRetrying = gettext("Retrying...");
$write_access = $write_access ? "true" : "false";
$script = <<<EOD
var usfmEditorChapterLoaded = '$chapterLoaded';
var usfmEditorChapterSaving = '$chapterSaving';
var usfmEditorChapterRetrying = '$chapterRetrying';
var usfmEditorWriteAccess = $write_access;
EOD;
$view->view->script = $script;


$class = Filter_CustomCSS::getClass ($bible);
$font = $database_config_bible->getTextFont ($bible);
$direction = $database_config_bible->getTextDirection ($bible);
$view->view->custom_class = $class;
$view->view->custom_css = Filter_CustomCSS::getCss ($class, Fonts_Logic::getFontPath ($font), $direction);


$view->render ("index.php");


Assets_Page::footer ();


/*
Tests for the USFM editor:
* Autosave on going to another passage.
* Autosave on document unload.
* Autosave shortly after any change.
*/


?>
