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
?>
<h1><?php echo gettext("Select Notes") ?></h1>
<p>
<?php echo gettext("Select notes that refer to:") ?>
  <a <?php if ($this->passageselector == 3) { ?>class="active"<?php } ?> href="select.php?passageselector=3"><?php echo gettext("any passage") ?></a>
  |
  <a <?php if ($this->passageselector == 0) { ?>class="active"<?php } ?> href="select.php?passageselector=0"><?php echo gettext("the current verse") ?></a>
  |
  <a <?php if ($this->passageselector == 1) { ?>class="active"<?php } ?> href="select.php?passageselector=1"><?php echo gettext("the current chapter") ?></a>
  |
  <a <?php if ($this->passageselector == 2) { ?>class="active"<?php } ?> href="select.php?passageselector=2"><?php echo gettext("the current book") ?></a>
</p>
<p>
<?php echo gettext("Select notes that have been edited:") ?>
  <a <?php if ($this->editselector == 0) { ?>class="active"<?php } ?> href="select.php?editselector=0"><?php echo gettext("at any time") ?></a>
  |
  <a <?php if ($this->editselector == 1) { ?>class="active"<?php } ?> href="select.php?editselector=1"><?php echo gettext("during the last 30 days") ?></a>
  |
  <a <?php if ($this->editselector == 2) { ?>class="active"<?php } ?> href="select.php?editselector=2"><?php echo gettext("during the last 7 days") ?></a>
  |
  <a <?php if ($this->editselector == 3) { ?>class="active"<?php } ?> href="select.php?editselector=3"><?php echo gettext("since yesterday") ?></a>
  |
  <a <?php if ($this->editselector == 4) { ?>class="active"<?php } ?> href="select.php?editselector=4"><?php echo gettext("today") ?></a>
</p>
<p>
<?php echo gettext("Select notes that have not been edited for:") ?>
  <a <?php if ($this->noneditselector == 0) { ?>class="active"<?php } ?> href="select.php?noneditselector=0"><?php echo gettext("any time") ?></a>
  |
  <a <?php if ($this->noneditselector == 1) { ?>class="active"<?php } ?> href="select.php?noneditselector=1"><?php echo gettext("a day") ?></a>
  |
  <a <?php if ($this->noneditselector == 2) { ?>class="active"<?php } ?> href="select.php?noneditselector=2"><?php echo gettext("two days") ?></a>
  |
  <a <?php if ($this->noneditselector == 3) { ?>class="active"<?php } ?> href="select.php?noneditselector=3"><?php echo gettext("a week") ?></a>
  |
  <a <?php if ($this->noneditselector == 4) { ?>class="active"<?php } ?> href="select.php?noneditselector=4"><?php echo gettext("a month") ?></a>
  |
  <a <?php if ($this->noneditselector == 5) { ?>class="active"<?php } ?> href="select.php?noneditselector=5"><?php echo gettext("a year") ?></a>
</p>
<p>
<?php echo gettext("Select notes that have a certain status:") ?>
  <a <?php if ($this->statusselector == "") { ?>class="active"<?php } ?> href="select.php?statusselector="><?php echo gettext("Any") ?></a>
  <?php for ($this->statusids as $offset => $statusid) { ?>
    |
    <a <?php if ($this->statusselector == $statusid) { ?>class="active"<?php } ?> href="select.php?statusselector=<?php echo $this->statusids[$offset] ?>"><?php echo $this->statuslocs[$offset] ?></a>
  <?php } ?>
</p>
<p>
<?php echo gettext("Select notes that belong to:") ?>
  <a <?php if ($this->bibleselector == "") { ?>class="active"<?php } ?> href="select.php?bibleselector="><?php echo gettext("any Bible") ?></a>
  <?php for ($this->bibles as $offset => $bible) { ?>
    |
    <a <?php if ($this->bibleselector == $bible) { ?>class="active"<?php } ?> href="select.php?bibleselector=<?php echo $this->bibles[$offset] ?>"><?php echo $this->bibles[$offset] ?></a>
  <?php } ?>
</p>
<p>
<?php echo gettext("Select notes that have been assigned to somebody:") ?>
  <a <?php if ($this->assignmentselector == "") { ?>class="active"<?php } ?> href="select.php?assignmentselector="><?php echo gettext("do not care about this") ?></a>
  <?php for ($this->assignees as $offset => $assignee) { ?>
    |
    <a <?php if ($this->assignmentselector == $assignee) { ?>class="active"<?php } ?> href="select.php?assignmentselector=<?php echo $this->assignees[$offset] ?>"><?php echo $this->assignees[$offset] ?></a>
  <?php } ?>
  <?php if ($this->nonexistingassignee) { ?>
    <?php echo gettext("Currently looking for notes assigned to:") ?> <?php echo $this->assignmentselector ?>
  <?php } ?>
</p>
<p>
<?php echo gettext("Select notes by subscription:") ?>
  <a <?php if ($this->subscriptionselector == 0) { ?>class="active"<?php } ?> href="select.php?subscriptionselector=0"><?php echo gettext("do not care about this") ?></a>
  |
  <a <?php if ($this->subscriptionselector == 1) { ?>class="active"<?php } ?> href="select.php?subscriptionselector=1"><?php echo gettext("notes I am subscribed to") ?></a>
</p>
<?php echo gettext("Select notes that have a severity of:") ?>
  <a <?php if ($this->severityselector == -1) { ?>class="active"<?php } ?> href="select.php?severityselector=-1"><?php echo gettext("Any") ?></a>
  <?php for ($this->severities as $offset => $severity) { ?>
    |
    <a <?php if ($this->severityselector == $severity[0]) { ?>class="active"<?php } ?> href="select.php?severityselector=<?php echo $this->severities[$offset][0] ?>"><?php echo $this->severities[$offset][1] ?></a>
  <?php } ?>
</p>
</p>
<form action="select.php?textselector=1" name="text" method="post">
<?php echo gettext("Select notes that contain text:") ?>
  <a <?php if ($this->textselector == 0) { ?>class="active"<?php } ?> href="select.php?textselector=0"><?php echo gettext("Any") ?></a>
  |
  <a <?php if ($this->textselector == 1) { ?>class="active"<?php } ?> href="select.php?textselector=1"><?php echo gettext("Specific text or words:") ?></a>
  <?php if ($this->textselector == 1) { ?>
      <input type="text" name="text" value="<?php echo $this->searchtext ?>" />
      <input type="submit" name="save" value=<?php echo gettext("Save") ?> />
  <?php } ?>
</form>
</p>
<br>
<p><?php echo gettext("Number of notes selected:") ?> <?php echo $this->count ?></p>
<br>
<?php echo gettext("Passage inclusion:") ?>
  <a <?php if ($this->passageinclusionselector == 0) { ?>class="active"<?php } ?> href="select.php?passageinclusionyselector=0"><?php echo gettext("Do not include the passage text") ?></a>
  <a <?php if ($this->passageinclusionselector == 1) { ?>class="active"<?php } ?> href="select.php?passageinclusionyselector=1"><?php echo gettext("Include the passage text") ?></a>
</p>
<?php echo gettext("Text inclusion:") ?>
  <a <?php if ($this->textinclusionselector == 0) { ?>class="active"<?php } ?> href="select.php?textinclusionyselector=0"><?php echo gettext("Do not include the note text") ?></a>
  <a <?php if ($this->textinclusionselector == 1) { ?>class="active"<?php } ?> href="select.php?textinclusionyselector=1"><?php echo gettext("Include the note text") ?></a>
</p>
<h3><a href="index.php"><?php echo gettext("Display the notes list") ?></a></h3>
<script type="text/javascript" src="poll.js?<?php echo config_logic_version () ?>"></script>
