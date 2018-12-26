# HOTP-Firmware
Wer sich noch nicht mit dem Thema HOTP oder Einmalpasswörter beschäftig hat wird sich vermutlich mit der HOTP-Firmware schwer tun. Deshalb eine grobe Erklärung zu HOTP: Anstatt immer das gleiche Passwort einzugeben ändert sich das Passwort nach jeder Verwendung und die alten werden ungültig. Wie ist das möglich? Anstatt des ganzen Passworts tippt man nur etwas was man aus dem Passwort ableiten kann in den Computer ein. Das Passwort nennt man bei HOTP Secret und das abgeleitete Einmalpasswort nennt man Token. Das Secret wird mit einer Zahl, dem sogenannten Counter verknüpft. Über Counter und Secret wird ein Hash gebildet. Der Hash über Counter und Secret ist dann der Token. Nachdem der Token erzeugt wurde wird der Counter um einen erhöht, alle Token mit kleinerem Counter sind ab jetzt ungültig. Das Secret kann mit einer Hostware (badge-tool) auf den Token geschrieben werden. 

## Badge-Tool herunterladen

Downloaden:

`git clone https://github.com/c3re/byoy`

Das badge-tool liegt dann unter byoy/hostware/commandline


## Secret auf den Stick schreiben
Das Secret ist in unserem Fall eine Random 32-Byte Folge. Diese kann man sich mit folgendem Befehl schön aus seinem urandom holen:

`dd if=/dev/urandom bs=1 count=32 status=noxfer 2> /dev/null|xxd -p -c32`

Alternativ auch hier:

https://www.random.org/cgi-bin/randbyte?nbytes=32&format=h

Die Leerzeichen zwischen den Bytes und das Newline muss dannach aber entfernt werden.

Das Secret kann per badge-tool -s "secret" dann auf den Stick geschrieben werden.

Damit badge-tool mit dem USB-Stick kommunizieren kann, muss man entweder root sein oder man platziert die 52-laborbadge.rules in /etc/udev/rules.d und lädt die Regeln mit `udevadm control --reload-rules` neu.

Das Secret auch noch einmal an einer anderen Stelle sichern, das ist der Backup key falls der HOTP-Stick verloren geht. Zusätzlich wird das Secret benötigt um den Stick bei der Gegenstelle, beispielsweise Keepass, einzurichten.

Also: secret schreiben:

`badge-tool -s 1dd26a1871cead95ffb3385bd257bf5aa9a0d649bf3150a65bed9941eb4885ad`

am besten den counter auf 0 zurücksetzen


`badge-tool -r`

# KeePass

`apt install keepass2`

Download HOTP Keepass Plugin
https://keepass.info/extensions/v2/otpkeyprov/OtpKeyProv-2.6.zip


entpacken und beide dateien nach /usr/lib/keepass2/plugins kopieren den plugins ordner muss man vorher anlegen

apt install mono-complete

Dann viele screenshots.

Amount of digits per Token = 8 setzen

Einmal im Keepass,

dann im badge-tool:
badge-tool -d 8

Zum entsperren notwendige Token=3
Look-Ahead count: 40
