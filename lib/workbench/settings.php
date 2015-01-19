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


require_once ("../bootstrap/bootstrap");
page_access_level (Filter_Roles::consultant ());


$database_config_user = Database_Config_User::getInstance ();


$name = request->query ['name'];
request->database_config_user()->setActiveWorkbench ($name);


@$preset = request->query ['preset'];
if (isset ($preset)) {
  workbenchSetURLs (workbenchDefaultURLs ($preset));
  workbenchSetWidths (workbenchDefaultWidths ($preset));
  workbenchSetHeights (workbenchDefaultHeights ($preset));
}


if (isset (request->post ['save'])) {
  $urls = array ();
  $widths = array ();
  $row_heights = array ();
  for ($row = 1; $row <= 3; $row++) {
    for ($column = 1; $column <= 5; $column++) {
      $urls [] = request->post ["url$row$column"];
      $widths [] = request->post ["width$row$column"];
    }
    $row_heights [] = request->post ["height$row"];
  }
  workbenchSetURLs ($urls);
  workbenchSetWidths ($widths);
  workbenchSetHeights ($row_heights);
  redirect_browser ("index");
  die;
}


$header = new Assets_Header (gettext("Edit workbench"));
$header->run ();
$view = new Assets_View (__FILE__);


$urls = workbenchGetURLs (false);
$widths = workbenchGetWidths ();
for ($urls as $key => $url) {
  $row = intval ($key / 5) + 1;
  $column = $key % 5 + 1;
  $variable = "url" . $row . $column;
  $view->view->$variable = $url;
  $variable = "width" . $row . $column;
  $view->view->$variable = $widths [$key];
}


$row_heights = workbenchGetHeights ();
for ($row_heights as $key => $height) {
  $row = $key + 1;
  $variable = "height" . $row;
  $view->view->$variable = $height;
}


$view->view->name = $name;


$view->render ("settings");


Assets_Page::footer ();


?>
