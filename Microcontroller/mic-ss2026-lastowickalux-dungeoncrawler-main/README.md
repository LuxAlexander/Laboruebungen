# mic-ss2026-LastowickaLux-DungeonCrawler

## Name
 WorkingTitle Lux in the flux.

## Description
1. Steuerung, Quick-Time-Events (QTE) und Fallen-Mechanik

    Joystick-Steuerung: Der Charakter wird mit dem analogen Joystick in vier Richtungen (oben, unten, links, rechts) durch die Räume des Dungeons bewegt. Die aktuelle Position, auf der seriellen Schnittstelle, und einfache Bewegungsanimationen des Charakters werden auf dem integrierten Display angezeigt.

    Quick-Time-Events (Kämpfe): Bei einer Begegnung mit Gegnern kann ein QTE starten. Auf dem LC-Display erscheint eine geforderte Richtung (z. B. ein Pfeil nach oben). Spieler haben ein kurzes, fest definiertes Zeitfenster (z. B. 1,5 Sekunden), um die entsprechende Eingabe über den Joystick zu tätigen. Bei korrekter Eingabe gilt der Kampf als gewonnen und der Spieler bekommt keinen Schaden.

    Fallen: Fallenräume können zufällig auftauchen. Der Spieler muss eine Entscheidung treffen, die entweder die Falle auslöst (und der Spieler verliert zB Leben oder Geld) oder den Spieler eine Belohnung oder nichts gibt.

2. Schadensmodell und Game-Over-Bedingung

    Lebens-Zähler: Der Charakter startet das Spiel mit einer festgelegten Anzahl an Lebenspunkten (z. B. 100 Lebenspunkte), die in der seriellen Konsole angezeigt werden.

    Schadensauslösung:

        Trifft der Charakter bei einer Falle die falsche Entscheidung oder wird von einem Monster getroffen, verringert sich die Lebensanzeige um eine bestimmte Anzahl an Punkten.

        Wird ein Quick-Time-Event falsch oder zu spät ausgeführt, gilt der Kampf als verloren und die Lebensanzeige verringert sich.

    Game Over: Sobald die Lebensanzeige den Wert 0 erreicht, stoppt das Spielgeschehen. Auf dem Display und der seriellen Schnittstelle wird der Text "Game Over" angezeigt und eine entsprechende Gameover-Melodie abgespielt. Das Spiel kann erst durch Drücken der Reset-Taste neu gestartet werden.

3. Audio, Story und Dialog-System

    Soundeffekte: Bestimmte Aktionen lösen ein direktes akustisches Feedback aus (z. B. ein "Swoosh"-Ton bei erfolgreichem Ausweichen, ein tiefer Ton bei erlittenem Schaden und ein Bestätigungston bei erfolgreichem QTE).

    Dialog-System: Manche Gegner und NPCs können einen Monolog/Dialog mit dem Spieler beginnen.


## Visuals

## Installation