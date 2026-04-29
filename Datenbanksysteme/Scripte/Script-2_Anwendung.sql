-- ==========================================
-- RÜCKGABE
-- ==========================================

-- 1. Medium/Kopie identifizieren (Schritt: "Strichcode")
-- Prüft, ob der Barcode existiert und holt die Stammdaten
SELECT c.*, m.TITLE 
FROM COPIES c
JOIN MEDIAS m ON c.Medias_MEDIA_ID = m.MEDIA_ID
WHERE c.BARCODE_ID = :barcode
FOR UPDATE WAIT 5;

-- 2. Kosten berechnen & im Ledger speichern (Schritt: "Überzogen?" & "Cost")
-- Dieser Befehl berechnet die Gebühr und schreibt sie in TOTAL_COST.
-- GREATEST(0, ...) verhindert negative Kosten bei rechtzeitiger Abgabe.
-- 'cost'-Wert aus PRICES, der aktuell 'valid' ist.
-- Dieser Wert wird mit der Anzahl der Tage Multipliziert.
UPDATE LEDGER l
SET l.TOTAL_COST = GREATEST(0, 
    ROUND(
        (SYSDATE - l.END_DATE) * (
            SELECT p.cost 
            FROM PRICES p
            -- Verknüpfung über MEDIAS, um den passenden Preis für die Kopie zu finden
            JOIN MEDIAS m ON (m.MEDIA_TYPES_TYPE_ID = p.type_id AND m.genre_id = p.genre_id)
            JOIN COPIES c ON (c.Medias_MEDIA_ID = m.MEDIA_ID)
            WHERE c.BARCODE_ID = l.COPIES_BARCODE_ID
            AND p.valid <= SYSDATE -- Preis muss bereits gültig sein
            ORDER BY p.valid DESC    -- Das aktuellste Datum zuerst
            FETCH FIRST 1 ROW ONLY
        ), 2)
    )
WHERE l.COPIES_BARCODE_ID = :barcode 
AND l.RETURN_DTIME IS NULL;

-- 3. Status-Abfrage für die UI (Entscheidung: "Pay")
-- Zeigt dem Mitarbeiter an, ob und wie viel der Kunde zahlen muss.
-- Falls TOTAL_COST > 0, muss der Prozess "GELD EINTREIBEN" folgen.
SELECT TOTAL_COST, 
       CASE WHEN TOTAL_COST > 0 THEN 'ZAHLUNG ERFORDERLICH' ELSE 'KEINE GEBÜHR' END AS payment_status
FROM LEDGER
WHERE COPIES_BARCODE_ID = :barcode 
AND RETURN_DTIME IS NULL
FOR UPDATE;

-- 4. Rückgabe registrieren (Update des bestehenden Ledger-Eintrags)
UPDATE LEDGER
SET RETURN_DTIME = SYSDATE, 
    COST_PAID = :paid  -- Hier wird eingetragen, ob bezahlt wurde (Prozess "Pay")
WHERE COPIES_BARCODE_ID = :barcode 
AND RETURN_DTIME IS NULL;

-- 5. Statistik updaten (Falls kein Datenbank-Trigger genutzt wird)
-- Erhöht die Anzahl der Ausleihen und summiert die Zeit
UPDATE STATISTICS s
SET number_time_borrowed = number_time_borrowed + 1,
    sum_time_borrowed = sum_time_borrowed + (SELECT SYSDATE - START_DATE FROM LEDGER WHERE COPIES_BARCODE_ID = :barcode AND RETURN_DTIME = SYSDATE)
WHERE statistic_id = (SELECT m.MEDIA_ID FROM MEDIAS m JOIN COPIES c ON m.MEDIA_ID = c.Medias_MEDIA_ID WHERE c.BARCODE_ID = :barcode);


-- ==========================================
-- KUNDENVERWALTUNG & SUCHE
-- ==========================================

-- Prüfen, ob Kunde existiert (ist Kunde?)
SELECT * FROM CUSTOMERS WHERE CUSTOMER_LAST_NAME = :last_name AND BIRTH_DATE = :birth_d;

-- Neuen Kunden anlegen (Anlegen neues Kunden)
INSERT INTO CUSTOMERS (CUSTOMER_FIRST_NAME, CUSTOMER_LAST_NAME, BIRTH_DATE)
VALUES (:first_n, :last_n, :birth_d);

-- Suche Medien (Titel, Genre oder Typ)
SELECT m.*, g.CATEGORY, mt.TYPE 
FROM MEDIAS m
JOIN MEDIA_TYPES mt ON m.MEDIA_TYPES_TYPE_ID = mt.TYPE_ID
JOIN GENRES g ON m.MEDIA_ID = g.GENRE_ID
WHERE m.TITLE LIKE %:search% OR g.CATEGORY = :genre OR mt.TYPE = :type;


-- ==========================================
-- VERFÜGBARKEIT & VERLEIHE
-- ==========================================

-- FSK Prüfung (FSK berechtigt?)
-- Rechnet das Alter in Jahren aus und vergleicht mit age_restriction
SELECT * FROM MEDIAS m
WHERE m.MEDIA_ID = :media_id
AND m.age_restriction <= FLOOR(MONTHS_BETWEEN(SYSDATE, (SELECT birth_date FROM CUSTOMERS WHERE customer_id = :c_id)) / 12);

-- Prüfen, ob eine Kopie aktuell frei ist (ist Ausgeliehen?)
-- Ein Medium ist frei, wenn es eine Kopie gibt, die nicht in einem offenen Ledger-Eintrag steht
SELECT * FROM COPIES c
WHERE c.Medias_MEDIA_ID = :media_id
AND c.BARCODE_ID NOT IN (SELECT COPIES_BARCODE_ID FROM LEDGER WHERE RETURN_DTIME IS NULL)
FOR UPDATE WAIT 5;

-- Verleihen: Neuen Eintrag im Ledger anlegen
INSERT INTO LEDGER (START_DATE, END_DATE, TOTAL_COST, COST_PAID, CUSTOMERS_CUSTOMER_ID, COPIES_BARCODE_ID)
VALUES (SYSDATE, SYSDATE + 21, (SELECT cost FROM PRICES WHERE creation_date = (SELECT MAX(creation_date) FROM PRICES)), 0, :c_id, :barcode); --Ausleihen für 3 Wochen


-- ==========================================
-- RESERVIERUNG
-- ==========================================

-- Neue Reservierung anlegen (Warteschlange)
INSERT INTO RESERVATIONS (TIME_RESERVATION, STATUS, CUSTOMER_ID, MEDIA_ID)
VALUES (SYSDATE, 'PENDING', :c_id, :media_id);

-- Prüfen: Wer ist der nächste in der Schlange? (next in Queue?)
SELECT * FROM RESERVATIONS 
WHERE MEDIA_ID = :media_id AND STATUS = 'PENDING'
ORDER BY TIME_RESERVATION ASC
FOR UPDATE SKIP LOCKED -- Überspringt Datensätze, die bereits von anderen in Bearbeitung sind
FETCH FIRST 1 ROW ONLY;

-- Reservierung abschließen (Wenn abgeholt wird)
UPDATE RESERVATIONS
SET STATUS = 'DONE'
WHERE RESERVATION_ID = :res_id;