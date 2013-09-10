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
?>
<div>
  <span><?php echo $this->navigationHtml ?></span>
  |
  <span id="usfmstatus"></span>
</div>
<pre id="usfmeditor" contenteditable="true" style="white-space: pre-wrap; word-wrap:break-word;"></pre>
<script>
<?php echo $this->script; ?>
</script>
<script type="text/javascript" src="../rangy/rangy-core.js"></script>
<script type="text/javascript" src="index.js"></script>
<?php echo $this->navigationCode; ?>
