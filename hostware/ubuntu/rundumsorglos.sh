#!/bin/bash
if [[ $(id -u) -ne 0 ]] 
then
  echo "please run this script with root permissions."
  exit
fi

echo "This script will do the following:"
echo "- Download and install badge-tool (into /usr/local/bin)"
echo "- create a new secret and store it in the file secret.txt)"
echo "- write the secret to a token"
echo "- reset the token counter"
echo "- set the length of generated tokens to 8"
echo "- install keepass, the HOTP Plugin and its dependencies"
echo
echo "If you are unhappy with that abort now using CTRL+C otherwise hit enter."
read

apt-get -qq update
apt-get -qq install mono-complete keepass2
wget --quiet "https://github.com/c3re/byoy/raw/master/hostware/commandline/badge-tool" \
  -O /usr/local/bin/badge-tool
chmod +x /usr/local/bin/badge-tool
dd if=/dev/urandom bs=1 count=32 status=noxfer 2> /dev/null|xxd -p -c32 > secret

echo -n "Your new HOTP secret is: "
cat secret

badge-tool -s $(cat secret)
badge-tool -d 8
badge-tool -r
