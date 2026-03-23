BEGIN
  sys.grant_permissions_between_users('FHS52423', 'FHS52415');
END;


--sql
-- =========================================
-- CLEAN LIBRARY SCHEMA (ORACLE 11g)
-- =========================================

-- ======================
-- SEQUENCES
-- ======================
--CREATE SEQUENCE media_seq START WITH 1;
CREATE SEQUENCE author_seq START WITH 1;
CREATE SEQUENCE customer_seq START WITH 1;
CREATE SEQUENCE genre_seq START WITH 1;
CREATE SEQUENCE location_seq START WITH 1;
CREATE SEQUENCE shelf_seq START WITH 1;
CREATE SEQUENCE compartment_seq START WITH 1;
CREATE SEQUENCE copy_seq START WITH 1;
CREATE SEQUENCE ledger_seq START WITH 1;
CREATE SEQUENCE reservation_seq START WITH 1;

-- ======================
-- BASE TABLES
-- ======================

CREATE TABLE MEDIA_TYPE (
    media_type_id NUMBER PRIMARY KEY,
    name VARCHAR2(30) NOT NULL UNIQUE
);

CREATE TABLE MEDIA (
    media_id NUMBER PRIMARY KEY,
    title VARCHAR2(100) NOT NULL,
    description VARCHAR2(500),
    age_restriction NUMBER,
    media_type_id NUMBER NOT NULL,
    CONSTRAINT media_type_fk FOREIGN KEY (media_type_id)
        REFERENCES MEDIA_TYPE(media_type_id)
);

CREATE TABLE AUTHORS (
    author_id NUMBER PRIMARY KEY,
    first_name VARCHAR2(50),
    last_name VARCHAR2(50) NOT NULL
);

CREATE TABLE GENRE (
    genre_id NUMBER PRIMARY KEY,
    name VARCHAR2(50) NOT NULL
);

CREATE TABLE CUSTOMER (
    customer_id NUMBER PRIMARY KEY,
    first_name VARCHAR2(50),
    last_name VARCHAR2(50) NOT NULL,
    birth_date DATE NOT NULL
);

-- ======================
-- SUBTYPES (MEDIA)
-- ======================

CREATE TABLE BOOKS (
    media_id NUMBER PRIMARY KEY,
    isbn VARCHAR2(50) NOT NULL,
    CONSTRAINT books_media_fk FOREIGN KEY (media_id)
        REFERENCES MEDIA(media_id)
);

CREATE TABLE AUDIO (
    media_id NUMBER PRIMARY KEY,
    codec VARCHAR2(20) NOT NULL,
    duration NUMBER(10) NOT NULL,
    CONSTRAINT audio_media_fk FOREIGN KEY (media_id)
        REFERENCES MEDIA(media_id)
);

CREATE TABLE MOVIES (
    media_id NUMBER PRIMARY KEY,
    format VARCHAR2(20) NOT NULL,
    duration NUMBER(10) NOT NULL,
    CONSTRAINT movies_media_fk FOREIGN KEY (media_id)
        REFERENCES MEDIA(media_id)
);

CREATE TABLE MAGAZINES (
    media_id NUMBER PRIMARY KEY,
    edition VARCHAR2(30),
    CONSTRAINT magazines_media_fk FOREIGN KEY (media_id)
        REFERENCES MEDIA(media_id)
);

-- ======================
-- RELATIONS
-- ======================

CREATE TABLE MEDIA_GENRE (
    media_id NUMBER,
    genre_id NUMBER,
    PRIMARY KEY (media_id, genre_id),
    CONSTRAINT mg_media_fk FOREIGN KEY (media_id)
        REFERENCES MEDIA(media_id),
    CONSTRAINT mg_genre_fk FOREIGN KEY (genre_id)
        REFERENCES GENRE(genre_id)
);

CREATE TABLE WRITTEN_BY (
    media_id NUMBER,
    author_id NUMBER,
    PRIMARY KEY (media_id, author_id),
    CONSTRAINT wb_media_fk FOREIGN KEY (media_id)
        REFERENCES MEDIA(media_id),
    CONSTRAINT wb_author_fk FOREIGN KEY (author_id)
        REFERENCES AUTHORS(author_id)
);

-- ======================
-- STORAGE STRUCTURE
-- ======================

CREATE TABLE LOCATION (
    location_id NUMBER PRIMARY KEY
);

CREATE TABLE SHELF (
    shelf_id NUMBER PRIMARY KEY,
    location_id NUMBER NOT NULL,
    CONSTRAINT shelf_location_fk FOREIGN KEY (location_id)
        REFERENCES LOCATION(location_id)
);

CREATE TABLE COMPARTMENT (
    compartment_id NUMBER PRIMARY KEY,
    shelf_id NUMBER NOT NULL,
    CONSTRAINT compartment_shelf_fk FOREIGN KEY (shelf_id)
        REFERENCES SHELF(shelf_id)
);

CREATE TABLE COPY (
    copy_id NUMBER PRIMARY KEY,
    barcode VARCHAR2(50) UNIQUE NOT NULL,
    media_id NUMBER NOT NULL,
    compartment_id NUMBER NOT NULL,
    CONSTRAINT copy_media_fk FOREIGN KEY (media_id)
        REFERENCES MEDIA(media_id),
    CONSTRAINT copy_compartment_fk FOREIGN KEY (compartment_id)
        REFERENCES COMPARTMENT(compartment_id)
);

-- ======================
-- TRANSACTIONS
-- ======================

CREATE TABLE LEDGER (
    ledger_id NUMBER PRIMARY KEY,
    customer_id NUMBER NOT NULL,
    copy_id NUMBER NOT NULL,
    start_date DATE NOT NULL,
    end_date DATE,
    return_date DATE,
    total_cost NUMBER(9,2),
    CONSTRAINT ledger_customer_fk FOREIGN KEY (customer_id)
        REFERENCES CUSTOMER(customer_id),
    CONSTRAINT ledger_copy_fk FOREIGN KEY (copy_id)
        REFERENCES COPY(copy_id)
);

CREATE TABLE RESERVATION (
    reservation_id NUMBER PRIMARY KEY,
    customer_id NUMBER NOT NULL,
    copy_id NUMBER NOT NULL,
    reservation_date DATE NOT NULL,
    CONSTRAINT reservation_customer_fk FOREIGN KEY (customer_id)
        REFERENCES CUSTOMER(customer_id),
    CONSTRAINT reservation_copy_fk FOREIGN KEY (copy_id)
        REFERENCES COPY(copy_id)
);

-- ======================
-- AUTO-INCREMENT TRIGGERS
-- ======================

CREATE OR REPLACE TRIGGER media_bi
BEFORE INSERT ON MEDIA
FOR EACH ROW
BEGIN
    IF :NEW.media_id IS NULL THEN
        SELECT media_seq.NEXTVAL INTO :NEW.media_id FROM dual;
    END IF;
END;

CREATE OR REPLACE TRIGGER author_bi
BEFORE INSERT ON AUTHORS
FOR EACH ROW
BEGIN
    IF :NEW.author_id IS NULL THEN
        SELECT author_seq.NEXTVAL INTO :NEW.author_id FROM dual;
    END IF;
END;


CREATE OR REPLACE TRIGGER customer_bi
BEFORE INSERT ON CUSTOMER
FOR EACH ROW
BEGIN
    IF :NEW.customer_id IS NULL THEN
        SELECT customer_seq.NEXTVAL INTO :NEW.customer_id FROM dual;
    END IF;
END;


CREATE OR REPLACE TRIGGER genre_bi
BEFORE INSERT ON GENRE
FOR EACH ROW
BEGIN
    IF :NEW.genre_id IS NULL THEN
        SELECT genre_seq.NEXTVAL INTO :NEW.genre_id FROM dual;
    END IF;
END;


CREATE OR REPLACE TRIGGER location_bi
BEFORE INSERT ON LOCATION
FOR EACH ROW
BEGIN
    IF :NEW.location_id IS NULL THEN
        SELECT location_seq.NEXTVAL INTO :NEW.location_id FROM dual;
    END IF;
END;


CREATE OR REPLACE TRIGGER shelf_bi
BEFORE INSERT ON SHELF
FOR EACH ROW
BEGIN
    IF :NEW.shelf_id IS NULL THEN
        SELECT shelf_seq.NEXTVAL INTO :NEW.shelf_id FROM dual;
    END IF;
END;


CREATE OR REPLACE TRIGGER compartment_bi
BEFORE INSERT ON COMPARTMENT
FOR EACH ROW
BEGIN
    IF :NEW.compartment_id IS NULL THEN
        SELECT compartment_seq.NEXTVAL INTO :NEW.compartment_id FROM dual;
    END IF;
END;


CREATE OR REPLACE TRIGGER copy_bi
BEFORE INSERT ON COPY
FOR EACH ROW
BEGIN
    IF :NEW.copy_id IS NULL THEN
        SELECT copy_seq.NEXTVAL INTO :NEW.copy_id FROM dual;
    END IF;
END;


CREATE OR REPLACE TRIGGER ledger_bi
BEFORE INSERT ON LEDGER
FOR EACH ROW
BEGIN
    IF :NEW.ledger_id IS NULL THEN
        SELECT ledger_seq.NEXTVAL INTO :NEW.ledger_id FROM dual;
    END IF;
END;


CREATE OR REPLACE TRIGGER reservation_bi
BEFORE INSERT ON RESERVATION
FOR EACH ROW
BEGIN
    IF :NEW.reservation_id IS NULL THEN
        SELECT reservation_seq.NEXTVAL INTO :NEW.reservation_id FROM dual;
    END IF;
END;

CREATE OR REPLACE TRIGGER customer_date_trunc
BEFORE INSERT OR UPDATE ON customer
FOR EACH ROW
BEGIN
    :NEW.birth_date := TRUNC(:NEW.birth_date);
END;


