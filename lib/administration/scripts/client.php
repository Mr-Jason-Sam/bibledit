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
<h3><?php echo gettext("Client mode") ?></h3>

<?php if ($this->client == true) { ?>

  <p>
    <?php echo gettext("Client mode is enabled.") ?>
    <a href="?disable="><?php echo gettext("Disable it.") ?></a>
  </p>
  <p>
    <?php echo gettext("Server") ?>:
    <a href="<?php echo $this->address ?>" target="_blank"><?php echo $this->address ?></a>
  </p>
  <p><?php echo gettext("Your role:") ?> <?php echo $this->role ?></p>

<?php } else { ?>

  <p><?php echo gettext("To enable client mode, enter the details below, and click Enable.") ?></p>
  
  <br>
  
  <form action="client.php" name="server" method="post">
    <p><?php echo gettext("Address of Bibledit Server on the Internet") ?>:</p>
    <p><input type="text" name="address" maxlength="1000" class="focus fullwidth" value="<?php echo $this->address ?>" /></p>
    <table>
      <tr>
        <td align="right"><?php echo gettext("Username on the server") ?></td>
        <td><input type="text" name="user" maxlength="30" class="focus" /></td>
      </tr>
      <tr>
        <td align="right"><?php echo gettext("Password on the server") ?></td>
        <td><input type="password" name="pass" maxlength="30" class="focus" /></td>
      </tr>
      <tr>
        <td></td>
        <td><input type="submit" name="connect" class="focus" value="<?php echo gettext("Enable") ?>" /></td>
      </tr>
    </table>
  </form>
  
  <br>

  <p><a href="?demo="><?php echo gettext("Or else connect to the Bibledit Demo Server.") ?></a></p>

<?php } ?>

<p class="error"><?php echo $this->error ?></p>
<p class="success"><?php echo $this->success ?></p>
<p class="error"><?php echo $this->demo ?></p>

<br>

<p>
  <?php echo gettext("When client mode is enabled, Bibledit can be used offline as a client on a local computing device.") ?>
</p>

<br>

<p>
  <?php echo gettext("Client mode affects the following:") ?>
</p>
<ul>
<li>
  <?php echo gettext("Can synchronize the local Bibles with the Bibles on a Bibledit server.") ?>
</li>
<li>
  <?php echo gettext("Can synchronize the local Consultation Notes with the Consultation Notes on a Bibledit server.") ?>
</li>
<li>
  <?php echo gettext("After connecting to a server, the first time you synchronize, you will get the same data as on the server.") ?>
  <?php echo gettext("Local Bible data and Notes not on the server will be erased.") ?>
  <?php echo gettext("Bible data and Notes on the server will be downloaded.") ?>
  <?php echo gettext("This is good because it means that your data will be backed up to the server.") ?>
</li>
<li>
  <?php echo gettext("Does not send or receive email.") ?>
</li>
<li>
  <?php echo gettext("User is always logged in.") ?>
</li>
</ul>
