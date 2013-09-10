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
<h1><?php echo gettext ("Password protected network repository setup") ?></h1>
<?php if ($this->object == "consultationnotes") { ?>
<p><?php echo gettext ("Consultation Notes") ?></p>
<?php } else { ?>
<p><?php echo gettext ("Bible") ?>: <?php echo $this->object ?></p>
<?php } ?>
<p><?php echo gettext ("In this tutorial you are going to create a password protected git repository on the Internet.") ?></p>
<p><?php echo gettext ("You can host the repository on github.com. The tutorial assumes that user 'joe' has an account on github.com. His password is 'pazz'. He has created a repository with the name 'repo'.") ?></p>
<p><?php echo gettext ("To test access to the new repository from another computer on the network, type:") ?></p>
<pre>git ls-remote https://joe:pazz@github.com/joe/repo.git</pre>
<p><?php echo gettext ("If everything is fine, no errors will occur.") ?></p>

<a name="url"></a>
<h2><?php echo gettext ("Repository URL") ?></h2>
<p><?php echo gettext ("The repository that was created in this tutorial has the following URL:") ?></p>
<pre>https://joe:pazz@github.com/joe/repo.git</pre>
<p><?php echo gettext ("Your own repository that you have created has a URL too, the one that will be used from now on. Please enter it below, and press the Submit button to save it.") ?></p>
<form action="collaboration_password_protected_network_setup.php?object=<?php echo $this->object ?>#url" name="url" method="post">
  <p><input type="text" name="urlvalue" value="<?php echo $this->url ?>" /></p>
  <p><input type="submit" name="url" value=<?php echo gettext ("Submit") ?> /></p>
</form>

<h2><a href="collaboration_repo_read.php?object=<?php echo $this->object ?>"><?php echo gettext ("Next page") ?></a></h2>
