INSERT INTO CUSTOMER (first_name, last_name, birth_date) VALUES ('Alexander','Lux','01.01.1900');

SELECT * FROM CUSTOMER;

UPDATE CUSTOMER c SET birth_date = TO_DATE('01.01.1900','DD.MM.YYYY')
WHERE first_name = 'Alexander';


INSERT INTO CUSTOMER (first_name, last_name, birth_date) VALUES ('Susanne','Lastowicka',TO_DATE('02.02.1902','DD.MM.YYYY'));
INSERT INTO CUSTOMER (first_name, last_name, birth_date) VALUES ('Max','Mustermann',TO_DATE('02.02.2002','DD.MM.YYYY'));
