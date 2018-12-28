#!/bin/bash
if [[ $(id -u) -ne 0 ]] 
then
  echo "please run this script with root permissions."
  exit
fi

echo "This script will: " 
echo "- install KeePass with HOTP support"
echo "- generate a new random secret"
echo "- flash the secret to the HOTP-stick"
echo
echo -n "If you are unhappy with that abort now using CTRL+C otherwise hit enter."
read
echo
echo

########################### Installing Keepass ##########################
echo -n "Installing keepass...              "
apt-get -qq update
apt-get -qq install mono-complete keepass2 libusb-0.1-4 > /dev/null

if [[ $? -eq 0 ]] 
then
  echo -e "[ \033[01;32m OK \033[0m ]"
fi

########################### Installing HOTP Plugin ##########################
echo -n "Installing HOTP-plugin...          "
mkdir -p /usr/lib/keepass2/plugins
wget --quiet "https://keepass.info/extensions/v2/otpkeyprov/OtpKeyProv-2.6.zip" \
-O /usr/lib/keepass2/plugins/OtpKeyProv-2.6.zip
unzip -o -qq /usr/lib/keepass2/plugins/OtpKeyProv-2.6.zip -d /usr/lib/keepass2/plugins/
rm  /usr/lib/keepass2/plugins/OtpKeyProv-2.6.zip

if [[ $? -eq 0 ]] 
then
  echo -e "[ \033[01;32m OK \033[0m ]"
fi

########################### Installing Badgetool ##########################
echo -n "Installing badge-tool...           "
wget --quiet "https://github.com/c3re/byoy/raw/master/hostware/commandline/badge-tool" \
  -O /usr/local/bin/badge-tool
chmod +x /usr/local/bin/badge-tool

if [[ $? -eq 0 ]] 
then
  echo -e "[ \033[01;32m OK \033[0m ]"
fi

########################### generating secret ############################
echo -n "Generating secret...               "
dd if=/dev/urandom bs=1 count=32 status=noxfer 2> /dev/null|xxd -p -c32 > secret

if [[ $? -eq 0 ]] 
then
  echo -e "[ \033[01;32m OK \033[0m ]"
fi

########################### configuring the USB-Stick ######################
echo -n "plz insert the stick & hit enter..."
read
echo -n "Flashing secrect...                "
badge-tool -s $(cat secret)
badge-tool -d 8
badge-tool -r
if [[ $? -eq 0 ]] 
then
  echo -e "[ \033[01;32m OK \033[0m ]"
fi
echo
echo
echo -n "Congratulations! everything worked fine. Here is your secret:"
echo
echo "####################################################################"
echo "# $(cat secret) #"
echo "####################################################################"
echo
echo
echo "Please keep it in a safe place as it is your recovery if you loose your stick."
echo
echo
echo "You can now follow this tutorial to configure your KeePass"
echo "https://github.com/c3re/byoy/blob/master/docs/hotp.md#keepass"
