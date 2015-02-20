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
$assets_header = new Assets_Header (translate("Bibles"));
$assets_header->run();
$view = new Assets_View (__FILE__);
$bibles = access_bible_bibles ();
// Add general Bible.
$bibles [] = Notes_Logic::generalBibleName ();
$view.set_variable ("bibles = $bibles;
$view->render ("bible-n");
Assets_Page::footer ();
?>
