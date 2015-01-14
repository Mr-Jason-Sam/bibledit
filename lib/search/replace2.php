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


$database_config_general = Database_Config_General::getInstance ();
$database_config_user = Database_Config_User::getInstance ();
$database_search = Database_Search::getInstance ();


$siteUrl = Database_Config_General::getSiteURL ();


@$bible = request->query ['b'];
if (!isset ($bible)) $bible = request->database_config_user()->getBible ();


@$searchfor = request->query ['q'];
@$replacewith = request->query ['r'];
@$casesensitive = (request->query ['c'] == "true");
@$id = request->query ['id'];


if (isset ($id)) {
  
  // Get the Bible and passage for this identifier.
  $details = request->database_search()->getBiblePassage ($id);
  $bible = $details ['bible'];
  $book = $details ['book'];
  $chapter = $details ['chapter'];
  $verse = $details ['verse'];

  // Get the plain text.
  $text = request->database_search()->getBibleVerseText (bible, book, chapter, $verse);
  
  // Format it.
  $link = filter_passage_link_for_opening_editor_at (book, chapter, verse);
  $oldtext =  $text;
  $newtext = str_replace ($searchfor, $replacewith, $text);
  if ($replacewith != "") $newtext =  filter_string_markup_words (array ($replacewith), $newtext);
  
$output = <<< EOD
<div id="$id">
<p><a href="replace"> ✔ </a> <a href="delete"> ✗ </a> $link</p>
<p>$oldtext</p>
<p>$newtext</p>
</div>
EOD;
  
  // Output to browser.
  echo $output;

  // Done.  
  die;
}


$header = new Assets_Header (gettext("Replace"));
$header->run ();


$view = new Assets_View (__FILE__);


$view->view->bible = $bible;


$script = <<<EOD
var searchBible = "$bible";
EOD;
$view->view->script = $script;


$view->render ("replace2.php");


Assets_Page::footer ();


?>
