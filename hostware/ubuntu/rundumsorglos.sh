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

########################### Installing Keepass ##########################
apt-get -qq update
apt-get -qq install mono-complete keepass2 libusb-0.1-4

mkdir -p /usr/lib/keepass2/plugins
########################### Installing HOTP Plugin ##########################
wget --quiet "https://keepass.info/extensions/v2/otpkeyprov/OtpKeyProv-2.6.zip" \
-O /usr/lib/keepass2/plugins
unzip /usr/lib/keepass2/plugins/OtpKeyProv-2.6.zip -d /usr/lib/keepass2/plugins/
rm  /usr/lib/keepass2/plugins/OtpKeyProv-2.6.zip

########################### Installing Badgetool ##########################
wget --quiet "https://github.com/c3re/byoy/raw/master/hostware/commandline/badge-tool" \
  -O /usr/local/bin/badge-tool
chmod +x /usr/local/bin/badge-tool

########################### generating secret ############################
dd if=/dev/urandom bs=1 count=32 status=noxfer 2> /dev/null|xxd -p -c32 > secret

echo -n "Your new HOTP secret is: "
cat secret

########################### configuring the USB-Stick ######################
badge-tool -s $(cat secret)
badge-tool -d 8
badge-tool -r
