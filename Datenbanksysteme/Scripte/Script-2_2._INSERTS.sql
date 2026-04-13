
-- CUSTOMER
INSERT INTO CUSTOMERS (customer_first_name, customer_last_name, birth_date) 
VALUES ('Alexander', 'Lux', TO_DATE('01.01.1950', 'DD.MM.YYYY'));

INSERT INTO CUSTOMERS (customer_first_name, customer_last_name, birth_date) 
VALUES ('Susanne', 'Lastowicka', TO_DATE('02.02.1902', 'DD.MM.YYYY'));

INSERT INTO CUSTOMERS (customer_first_name, customer_last_name, birth_date) 
VALUES ('Max', 'Mustermann', TO_DATE('02.02.2002', 'DD.MM.YYYY'));


-- Media Types
INSERT INTO MEDIA_TYPES (type_id, type) VALUES (1, 'BOOK');
INSERT INTO MEDIA_TYPES (type_id, type) VALUES (2, 'MOVIE');
INSERT INTO MEDIA_TYPES (type_id, type) VALUES (3, 'AUDIO');
INSERT INTO MEDIA_TYPES (type_id, type) VALUES (4, 'MAGAZINE');

-- Genres
INSERT INTO GENRES (category) VALUES ('SCI-FI');
INSERT INTO GENRES (category) VALUES ('Romance');
INSERT INTO GENRES (category) VALUES ('Dystopie');
INSERT INTO GENRES (category) VALUES ('Lifestyle');

SELECT * FROM GENRES g;

-- Insert a Book
INSERT INTO MEDIAS (title, description, age_restriction, MEDIA_TYPES_type_id)
VALUES ('1984', 'Written 1948', 16, 1);

INSERT INTO BOOKS (media_id, isbn_id)
VALUES (MEDIA_SEQ.CURRVAL, '978-3-16-148410-0');

-- Insert a Movie
INSERT INTO MEDIAS (title, description, age_restriction, MEDIA_TYPES_type_id)
VALUES ('Star Wars Episode I', 'In a galaxy far away', 6, 2);

INSERT INTO MOVIES (media_id, format, duration)
VALUES (MEDIA_SEQ.CURRVAL, '35mm', TO_DATE('02:16:00', 'HH24:MI:SS'));

SELECT * FROM Medias;

-- Location
INSERT INTO LOCATIONS (name, street) VALUES ('Stadtbibliothek', 'Kinostraße 4');

-- Shelf (Requires loc_id from above, likely ID 1 if it's the first insert)
INSERT INTO SHELVES (shelf_id, LOCATIONS_loc_id, shelf_code) 
VALUES ('A1', 1, 'REGAL-A1');

-- Compartment (Requires shelf_id and loc_id)
INSERT INTO COMPARTMENTS (SHELVES_shelf_id, SHELVES_LOCATIONS_loc_id, position)
VALUES ('A1', 1, 'Mitte');

--COPIES
INSERT INTO COPIES (barcode_id, MEDIAS_media_id, COMPARTMENTS_comp_id, COMPARTMENTS_shelf_id, COMPARTMENTS_loc_id)
VALUES (
    'BC001', 
    (SELECT media_id FROM MEDIAS WHERE title = '1984'),
    (SELECT comp_id FROM COMPARTMENTS WHERE position = 'Mitte' AND ROWNUM = 1),
    'A1',
    1
);

--AUTHOR
INSERT INTO AUTHORS (first_name, last_name) VALUES ('George', 'Orwell');

INSERT INTO AUTHORS
( FIRST_NAME, LAST_NAME)
VALUES('Sapphṓ', 'Ψάπφω');

-- Relationship written_by
INSERT INTO written_by (BOOKS_media_id, AUTHORS_author_id)
VALUES (
    (SELECT media_id FROM MEDIAS WHERE title = '1984'),
    (SELECT author_id FROM AUTHORS WHERE last_name = 'Orwell')
);


---AI FAKE DATA

-- Authors
INSERT INTO AUTHORS (first_name, last_name) VALUES ('Frank', 'Herbert');
INSERT INTO AUTHORS (first_name, last_name) VALUES ('J.R.R.', 'Tolkien');
INSERT INTO AUTHORS (first_name, last_name) VALUES ('Christopher', 'Nolan');

-- Additional Genres
INSERT INTO GENRES (category) VALUES ('Fantasy');
INSERT INTO GENRES (category) VALUES ('Action');
INSERT INTO GENRES (category) VALUES ('Documentary');

-- A Fantasy Book
INSERT INTO MEDIAS (title, description, age_restriction, MEDIA_TYPES_type_id)
VALUES ('The Hobbit', 'There and back again', 6, 1);

--Pin book to media with currval
INSERT INTO BOOKS (media_id, isbn_id) 
VALUES (MEDIA_SEQ.CURRVAL, '978-0261102217');

INSERT INTO written_by (BOOKS_media_id, AUTHORS_author_id)
VALUES (MEDIA_SEQ.CURRVAL, (SELECT author_id FROM AUTHORS WHERE last_name = 'Tolkien'));

-- A Sci-Fi Movie
INSERT INTO MEDIAS (title, description, age_restriction, MEDIA_TYPES_type_id)
VALUES ('Interstellar', 'Exploring the stars to save humanity', 12, 2);

INSERT INTO MOVIES (media_id, format, duration)
VALUES (MEDIA_SEQ.CURRVAL, 'IMAX', TO_DATE('02:49:00', 'HH24:MI:SS'));


-- New Location
INSERT INTO LOCATIONS (name, street) VALUES ('Zweigstelle West', 'Hauptplatz 10');

-- Shelves for Location 2 (ID 2)
INSERT INTO SHELVES (shelf_id, LOCATIONS_loc_id, shelf_code) VALUES ('S1', 2, 'SCI-FI-SECTION');
INSERT INTO SHELVES (shelf_id, LOCATIONS_loc_id, shelf_code) VALUES ('F1', 2, 'FANTASY-SECTION');

-- Compartments
--create new entry with nextval
INSERT INTO COMPARTMENTS (comp_id, SHELVES_shelf_id, SHELVES_LOCATIONS_loc_id, position) 
VALUES (COMPARTMENT_SEQ.NEXTVAL, 'S1', 2, 'Top Shelf');
INSERT INTO COMPARTMENTS (comp_id, SHELVES_shelf_id, SHELVES_LOCATIONS_loc_id, position) 
VALUES (COMPARTMENT_SEQ.NEXTVAL, 'F1', 2, 'Bottom Shelf');


-- Copy of The Hobbit in the Fantasy Section
INSERT INTO COPIES (barcode_id, MEDIAS_media_id, COMPARTMENTS_comp_id, COMPARTMENTS_shelf_id, COMPARTMENTS_loc_id)
VALUES ('BC-HOBBIT-01', 
       (SELECT media_id FROM MEDIAS WHERE title = 'The Hobbit'), 
       (SELECT comp_id FROM COMPARTMENTS WHERE position = 'Bottom Shelf' AND SHELVES_LOCATIONS_loc_id = 2), 
       'F1', 2);

-- Copy of Interstellar in the Sci-Fi Section
INSERT INTO COPIES (barcode_id, MEDIAS_media_id, COMPARTMENTS_comp_id, COMPARTMENTS_shelf_id, COMPARTMENTS_loc_id)
VALUES ('BC-INT-001', 
       (SELECT media_id FROM MEDIAS WHERE title = 'Interstellar'), 
       (SELECT comp_id FROM COMPARTMENTS WHERE position = 'Top Shelf' AND SHELVES_LOCATIONS_loc_id = 2), 
       'S1', 2);

-- A rental for Max Mustermann
INSERT INTO LEDGER (start_date, end_date, CUSTOMERS_customer_id, COPIES_barcode_id, COPIES_media_id, total_cost)
VALUES (
    TRUNC(SYSDATE) - 2, -- 00:00:00 Uhr vor zwei Tagen
    TRUNC(SYSDATE) + 5, 
    (SELECT customer_id FROM CUSTOMERS WHERE customer_last_name = 'Mustermann'),
    'BC-INT-001',
    (SELECT media_id FROM MEDIAS WHERE title = 'Interstellar'),
    4.50
);

-- A reservation for Alexander Lux
INSERT INTO RESERVATIONS (reservation_id, time_reservation, CUSTOMERS_customer_id, MEDIAS_media_id, status, 
                          LEDGER_CUSTOMERS_customer_id, LEDGER_COPIES_barcode_id, LEDGER_COPIES_media_id, LEDGER_start_date)
VALUES (
    RESERVATION_SEQ.NEXTVAL,
    SYSDATE, -- Hier ist die Uhrzeit egal, da kein FK
    (SELECT customer_id FROM CUSTOMERS WHERE customer_last_name = 'Lux'),
    (SELECT media_id FROM MEDIAS WHERE title = 'The Hobbit'),
    'PENDING',
    (SELECT customer_id FROM CUSTOMERS WHERE customer_last_name = 'Mustermann'),
    'BC-INT-001',
    (SELECT media_id FROM MEDIAS WHERE title = 'Interstellar'),
    TRUNC(SYSDATE) - 2 -- Muss EXAKT mit dem Datum im Ledger übereinstimmen
);

--SELECT

SELECT m.TITLE, c.BARCODE_ID, l.NAME as LOCATION, comp.POSITION
FROM MEDIAS m
JOIN COPIES c ON m.MEDIA_ID = c.MEDIAS_media_id
JOIN LOCATIONS l ON c.COMPARTMENTS_loc_id = l.LOC_ID
JOIN COMPARTMENTS comp ON c.COMPARTMENTS_comp_id = comp.COMP_ID;

SELECT 
    c.BARCODE_ID,
    m.TITLE,
    mt.TYPE AS MEDIEN_TYP,
    l.NAME AS STANDORT,
    s.SHELF_CODE AS REGAL,
    comp.POSITION AS FACH
FROM COPIES c
JOIN MEDIAS m ON c.MEDIAS_media_id = m.MEDIA_ID
JOIN MEDIA_TYPES mt ON m.MEDIA_TYPES_type_id = mt.TYPE_ID
JOIN COMPARTMENTS comp ON c.COMPARTMENTS_comp_id = comp.COMP_ID 
    AND c.COMPARTMENTS_shelf_id = comp.SHELVES_shelf_id 
    AND c.COMPARTMENTS_loc_id = comp.SHELVES_LOCATIONS_loc_id
JOIN SHELVES s ON comp.SHELVES_shelf_id = s.SHELF_ID 
    AND comp.SHELVES_LOCATIONS_loc_id = s.LOCATIONS_loc_id
JOIN LOCATIONS l ON s.LOCATIONS_loc_id = l.LOC_ID
ORDER BY l.NAME, s.SHELF_CODE;

SELECT 
    cust.CUSTOMER_FIRST_NAME || ' ' || cust.CUSTOMER_LAST_NAME AS KUNDE,
    m.TITLE AS MEDIUM,
    led.COPIES_BARCODE_ID AS BARCODE,
    TO_CHAR(led.START_DATE, 'DD.MM.YYYY') AS AUSGELIEHEN_AM,
    TO_CHAR(led.END_DATE, 'DD.MM.YYYY') AS RUECKGABE_BIS,
    led.TOTAL_COST AS PREIS
FROM LEDGER led
JOIN CUSTOMERS cust ON led.CUSTOMERS_customer_id = cust.CUSTOMER_ID
JOIN MEDIAS m ON led.COPIES_MEDIA_ID = m.MEDIA_ID
WHERE led.RETURN_DTIME IS NULL -- Nur aktuell ausgeliehene Medien
ORDER BY led.END_DATE ASC;

SELECT 
    a.FIRST_NAME || ' ' || a.LAST_NAME AS AUTOR,
    m.TITLE AS BUCH_TITEL,
    b.ISBN_ID AS ISBN,
    m.AGE_RESTRICTION AS FSK
FROM AUTHORS a
JOIN written_by wb ON a.AUTHOR_ID = wb.AUTHORS_author_id
JOIN BOOKS b ON wb.BOOKS_media_id = b.MEDIA_ID
JOIN MEDIAS m ON b.MEDIA_ID = m.MEDIA_ID
ORDER BY a.LAST_NAME;


SELECT 
    m.TITLE,
    c.BARCODE_ID,
    NVL(s.NUMBER_TIME_BORROWED, 0) AS ANZAHL_LEIHEN,
    NVL(s.SUM_TIME_BORROWED, 0) AS GESAMTSTAGE_VERLIEHEN,
    (SELECT SUM(total_cost) FROM LEDGER WHERE COPIES_BARCODE_ID = c.BARCODE_ID) AS GESAMT_UMSATZ
FROM COPIES c
JOIN MEDIAS m ON c.MEDIAS_media_id = m.MEDIA_ID
LEFT JOIN STATISTICS s ON c.BARCODE_ID = s.COPIES_BARCODE_ID 
    AND c.MEDIAS_media_id = s.COPIES_MEDIAS_media_id
ORDER BY ANZAHL_LEIHEN DESC;
