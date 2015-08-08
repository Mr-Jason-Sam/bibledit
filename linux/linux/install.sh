#!/bin/bash

echo Installing Bibledit on Linux.

echo Please provide the password for the administrative user and press Enter:
read PASSWORD
echo $PASSWORD

clear
echo Updating the software sources...
sleep 1
echo $PASSWORD | sudo -S apt-get update
sleep 4

clear
echo Installing the software Bibledit relies on...
sleep 1
echo $PASSWORD | sudo -S apt-get --yes --force-yes install build-essential
echo $PASSWORD | sudo -S apt-get --yes --force-yes install git
echo $PASSWORD | sudo -S apt-get --yes --force-yes install zip
echo $PASSWORD | sudo -S apt-get --yes --force-yes install pkgconf
echo $PASSWORD | sudo -S apt-get --yes --force-yes install libxml2-dev
echo $PASSWORD | sudo -S apt-get --yes --force-yes install libsqlite3-dev
echo $PASSWORD | sudo -S apt-get --yes --force-yes install libcurl4-openssl-dev
echo $PASSWORD | sudo -S apt-get --yes --force-yes install libssl-dev
echo $PASSWORD | sudo -S apt-get --yes --force-yes install libatspi2.0-dev
echo $PASSWORD | sudo -S apt-get --yes --force-yes install libgtk-3-dev
sleep 4

clear
echo Downloading Bibledit...
cd
rm bibledit-1.0.187.tar.gz
wget http://bibledit.org/linux/bibledit-1.0.187.tar.gz
if [ $? -ne 0 ]
then
  echo Failed to download Bibledit
  exit
fi
sleep 4

clear
echo Unpacking Bibledit in folder bibledit...
mkdir -p bibledit
tar xf bibledit-1.0.187.tar.gz -C bibledit --strip-components=1
if [ $? -ne 0 ]
then
  echo Failed to unpack Bibledit
  exit
fi
sleep 4

clear
echo Building Bibledit...
sleep 1
cd bibledit
./configure --enable-client --enable-paratext
if [ $? -ne 0 ]
then
  echo Failed to configure Bibledit
  exit
fi
make clean
make --jobs=4
if [ $? -ne 0 ]
then
  echo Failed to build Bibledit
  exit
fi
sleep 4

clear
sleep 1
echo $PASSWORD | sudo -S cp linux/bibledit.sh /usr/bin/bibledit
echo If there were no errors, Bibledit should be working now.
echo Bibledit works best with the Google Chrome browser.
echo Install the browser.
echo To have Bibledit automatically open in Chrome, set Chrome as the default browser.
echo --
echo To start Bibledit, open a terminal, and type:
echo \"bibledit\" 
echo and press Enter.