<ah>GuckTux
<ai>der Gtk-Bouquet-Editor
getestet mit folgender Konfiguration: linux, sat, yadd vom 8.September 02, neutrino
 
<c2>Anmerkung zur Umstellung auf GTK-2.0
Leider sind die Listen-Felder gegenber der GTK-1.X immer noch sehr langsam. Dies wird
schon beim laden der Senderlisten deutlich. Extrem wird die Geschichte jedoch, wenn die
Sortierung per click auf den Spaltenkopf eingeschaltet wird.
 
Die Win32 Version der Gtk hinkt der Linux Version deutlich hinterher. Dadurch treten
einige Fehler unter win auf, die unter Linux bereits gefixt sind.
Unter win funzt auch das draggen noch nicht so richtig...
 
 
 
<c2>Tutorial
 
<ab>Erstellen des eigenen Bouquet
Ich habe gerade einen neuen Scan durchgeführt und m�chte ein eigenes Bouquet anlegen.
Daf�r ben�tige ich als erstes die Scans im Verzeichnis des Editors. Einfach den down-
load-Button dr�cken, oder die Dateien:
 'services.xml' und 'bouquets.xml' (f�r Neutrino) bzw.
 'services' und 'bouquets' (f�r Enigma) bzw.
 'lcars.dvd' (f�r LCars)
von Hand dorthin kopieren.
Jetzt klicke ich auf 'load Neutrino' um den neuen Scan (bouquets.xml) auszuw�hlen.
 
Danach kopiere ich die komplette Liste durch druck auf '->' ins rechte Listenfeld.
Im rechten Listenfeld wird nun der Inhalt des Bouquets angezeigt, der im Button dar�ber
genannt wird. In meinem Fall steht dort jetzt Premiere.
Da ich aber mein eigenes Bouquet erstellen mchte, wechsel ich in die Bouquets-Ansicht
(dazu einfach den Bouquet-Button dr�cken [das ist der, wo jetzt 'Premiere' steht])
und dr�cke auf den '+'- Button. Jetzt steht an erster Stelle ein leeres Boquet mit sinnlosem
Namen. Diesen kann ich jetzt durch links-click auf den Namen umbenennen. Ich denke mir
hier mal den Namen 'eigenes Bouquet' aus.
[F�r enigma trage ich hier '@eigenes Bouquet' ein, damit das sp�ter an pos 1 steht.]
Abschlie�end noch ein doppelclick auf das neue Bouquet um in die Sender-Ansicht zu
gelangen.
 
Da ich trotz PW-Abo nur KiKa und Phoenix ;-) gucke w�hle ich jetzt die beiden Sender in
der linken Liste aus und kopiere diese in die rechte Liste.
Jetzt speichere ich meine Arbeit noch mit 'save All'. Was natrlich nicht alles speichert,
sondern nur alle Bouquets f�r Neutrino und Enigma.
(Oder ich nutze Lcars dann dr�cke ich auf den 'save actual'-Button.)
Da ich diese Liste als Ausgangsbasis f�r die Zukunft behalten m�chte, speichere ich auch
noch das aktuelle Bouquet.
 
Anschlie�end schicke ich die neue Liste noch per Upload zur Box (Hierbei werden die
Bouquets von Neutrino und Enigma �berschrieben).
Um die neue Liste sofort verwenden zu k�nnen, drcke ich auf den Telnet-Button und
starte mit dem Skript 'restart Neutrino' Neutrino neu.
 
<ab>Eintragen des eigenen Bouquet nach neuem Scan
Als erstes lade ich mein eigenes Bouquet, welches dann mittels '->' komplett nach rechts
kopiert wird. Danach das gleiche mit dem neuen Senderscan. Fertig.
 
<ab>P.S:
Wenn ich die alte 'bouquets*' behalte und nur die neu gescannte 'services*' benutze gilt
folgendes:
Alle Sender, welche in der neuen 'services*' sind, aber noch nicht in der alten'bouquets*',
vorhanden waren werden im Bouquet *NEW* eingetragen. Alle Sender welche im 'bouquets*'
stehen, nicht aber in der 'services.*' vorhanden sind werden gel�scht.
 
<ab>Telnet
Immer wiederkehrende Arbeitsg�nge k�nnen auf die Buttons im unteren Bereich
gelegt werden. Dazu wird die 'makros.kat' mit einem Editor aufgerufen und
wie folgt erg�nzt:
Der MakroName, welcher sp�ter auf dem Button stehen soll, wird in spitzen
Klammern gesetzt. In die n�chsten Zeilen kommen dann die Befehle, die das Makro
ausf�hren soll. Also genau das, was ihr sonst m�hsamm von Hand tippen mu�test.
F�hrende Leerzeichen, tabs, usw. in den Zeilen sind verboten.
Kommentare werden durch # (als erstes Zeichen in einer Zeile) eingeleitet.
<ai>Jedes sinnvolle Makro bitte ans uns weiterleiten.
 
 
 
FAQ:
<c2>Nachdem ich ein neues Bouquet angelegt und benannt habe, komme ich mit
<c2>doppelclick nicht zurck zur Sender-Ansicht, sondern nur ins rename.
Klick einfach zwischen die beiden K�stchen neben dem Bouquet-Namen, oder w�hle erst ein
anderes Bouquet an und mache anschlie�end den doppelclick aufs gew�nschte Bouquet.
 
<c2>Wozu sind die K�stchen neben dem Bouquet-Namen?
Das funzt nur bei Neutrino. Hiermit k�nnen Bouquets versteckt, bzw. Altersschutz
ein-/ausgeschaltet werden.
 
<c2>Funktion xyz soll funktionieren, bei mir klappt das aber nicht.
Alles was ich KURZ angetestet habe und bei mir lief, ist f�r mich 100% i.O.
Falls es bei dir nicht, oder nicht richtig l�uft, liegt das daran, da� ich �ber
solche Sachen nicht informiert wurde. Also, gefunden Fehler zusammen mit m�glichst
genauer Beschreibung UND eingesetzten scans sofort mailen.
Sp�testens 1 Monat sp�ter l�uft das dann :)
 
 
 
<ab>TODO
 - dnd f�r windoof
 - usw...
 
<c2>Fragen und Vorschl�ge bitte ins Forum. (http://gucktux.berlios.de)
Viel Spa�...
 
Das Kanalarbeiter-Team
<ai>(Abadon, Mycket)
