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
Ich habe gerade einen neuen Scan durchgefÃ¼hrt und möchte ein eigenes Bouquet anlegen.
Dafür benötige ich als erstes die Scans im Verzeichnis des Editors. Einfach den down-
load-Button drücken, oder die Dateien:
 'services.xml' und 'bouquets.xml' (für Neutrino) bzw.
 'services' und 'bouquets' (für Enigma) bzw.
 'lcars.dvd' (für LCars)
von Hand dorthin kopieren.
Jetzt klicke ich auf 'load Neutrino' um den neuen Scan (bouquets.xml) auszuwählen.
 
Danach kopiere ich die komplette Liste durch druck auf '->' ins rechte Listenfeld.
Im rechten Listenfeld wird nun der Inhalt des Bouquets angezeigt, der im Button darüber
genannt wird. In meinem Fall steht dort jetzt Premiere.
Da ich aber mein eigenes Bouquet erstellen mchte, wechsel ich in die Bouquets-Ansicht
(dazu einfach den Bouquet-Button drücken [das ist der, wo jetzt 'Premiere' steht])
und drücke auf den '+'- Button. Jetzt steht an erster Stelle ein leeres Boquet mit sinnlosem
Namen. Diesen kann ich jetzt durch links-click auf den Namen umbenennen. Ich denke mir
hier mal den Namen 'eigenes Bouquet' aus.
[Für enigma trage ich hier '@eigenes Bouquet' ein, damit das später an pos 1 steht.]
Abschließend noch ein doppelclick auf das neue Bouquet um in die Sender-Ansicht zu
gelangen.
 
Da ich trotz PW-Abo nur KiKa und Phoenix ;-) gucke wähle ich jetzt die beiden Sender in
der linken Liste aus und kopiere diese in die rechte Liste.
Jetzt speichere ich meine Arbeit noch mit 'save All'. Was natrlich nicht alles speichert,
sondern nur alle Bouquets für Neutrino und Enigma.
(Oder ich nutze Lcars dann drücke ich auf den 'save actual'-Button.)
Da ich diese Liste als Ausgangsbasis für die Zukunft behalten möchte, speichere ich auch
noch das aktuelle Bouquet.
 
Anschließend schicke ich die neue Liste noch per Upload zur Box (Hierbei werden die
Bouquets von Neutrino und Enigma überschrieben).
Um die neue Liste sofort verwenden zu können, drcke ich auf den Telnet-Button und
starte mit dem Skript 'restart Neutrino' Neutrino neu.
 
<ab>Eintragen des eigenen Bouquet nach neuem Scan
Als erstes lade ich mein eigenes Bouquet, welches dann mittels '->' komplett nach rechts
kopiert wird. Danach das gleiche mit dem neuen Senderscan. Fertig.
 
<ab>P.S:
Wenn ich die alte 'bouquets*' behalte und nur die neu gescannte 'services*' benutze gilt
folgendes:
Alle Sender, welche in der neuen 'services*' sind, aber noch nicht in der alten'bouquets*',
vorhanden waren werden im Bouquet *NEW* eingetragen. Alle Sender welche im 'bouquets*'
stehen, nicht aber in der 'services.*' vorhanden sind werden gelöscht.
 
<ab>Telnet
Immer wiederkehrende Arbeitsgänge können auf die Buttons im unteren Bereich
gelegt werden. Dazu wird die 'makros.kat' mit einem Editor aufgerufen und
wie folgt ergänzt:
Der MakroName, welcher später auf dem Button stehen soll, wird in spitzen
Klammern gesetzt. In die nächsten Zeilen kommen dann die Befehle, die das Makro
ausführen soll. Also genau das, was ihr sonst mühsamm von Hand tippen mußtest.
Führende Leerzeichen, tabs, usw. in den Zeilen sind verboten.
Kommentare werden durch # (als erstes Zeichen in einer Zeile) eingeleitet.
<ai>Jedes sinnvolle Makro bitte ans uns weiterleiten.
 
 
 
FAQ:
<c2>Nachdem ich ein neues Bouquet angelegt und benannt habe, komme ich mit
<c2>doppelclick nicht zurck zur Sender-Ansicht, sondern nur ins rename.
Klick einfach zwischen die beiden Kästchen neben dem Bouquet-Namen, oder wähle erst ein
anderes Bouquet an und mache anschließend den doppelclick aufs gewünschte Bouquet.
 
<c2>Wozu sind die Kästchen neben dem Bouquet-Namen?
Das funzt nur bei Neutrino. Hiermit können Bouquets versteckt, bzw. Altersschutz
ein-/ausgeschaltet werden.
 
<c2>Funktion xyz soll funktionieren, bei mir klappt das aber nicht.
Alles was ich KURZ angetestet habe und bei mir lief, ist für mich 100% i.O.
Falls es bei dir nicht, oder nicht richtig läuft, liegt das daran, daß ich über
solche Sachen nicht informiert wurde. Also, gefunden Fehler zusammen mit möglichst
genauer Beschreibung UND eingesetzten scans sofort mailen.
Spätestens 1 Monat später läuft das dann :)
 
 
 
<ab>TODO
 - dnd für windoof
 - usw...
 
<c2>Fragen und Vorschläge bitte ins Forum. (http://gucktux.berlios.de)
Viel Spaß...
 
Das Kanalarbeiter-Team
<ai>(Abadon, Mycket)
