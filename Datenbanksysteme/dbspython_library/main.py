# Demo program to connect python to oracle database
import datetime
from traceback import print_exc
import oracledb
import credentials_helper
import prettyprint
import yaml
from yaml.loader import SafeLoader
from database_helper import OracleDatabase, IsolationLevel, LockMode


# Press Umschalt+F10 to execute it or replace it with your code.
# Press Double Shift to search everywhere for classes, files, tool windows, actions, and settings.


def drop_test_table() -> None:
    cursor = db.conn.cursor()
    try:
        cursor.execute("drop table test")
    # ignore error if table does not exist
    except oracledb.DatabaseError as e:
        pass
    finally:
        cursor.close()


def create_test_table() -> None:
    drop_test_table()
    cursor = db.conn.cursor()
    cursor.execute("create table test (id number PRIMARY KEY, name varchar2(50))")


def simple_query():
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # execute the query,  always use bind variables to prevent SQL injection
        cursor.execute("select * from hr.employees "
                       "where employee_id>:id", {'id': 100})

        # fetch all results
        # be careful with large result sets, you might run out of memory --> use fetchmany or fetchone in a loop
        results = cursor.fetchall()

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        cursor.close()
        raise e

    prettyprint.print_results(results, cursor)

    cursor.close()  # always close the cursor when done

    db.conn.commit()  # commit any changes (close transaction)


def row_by_row() -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # execute the query
        cursor.execute("select * from hr.employees")

        # loop over all rows
        for row in cursor:
            # access columns by index
            print(f"id:{row[0]}, firstname: {row[1]}, lastname: {row[2]}")

        # alternative way to loop over all rows by explicitly using fetchone
        # while True:
        #    row = cursor.fetchone()
        #    if row is None:
        #        break
        #    print(f"id:{row[0]}, firstname: {row[1]}, lastname: {row[2]}")  # access columns by index

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

    db.conn.commit()  # commit any changes (close transaction)


def row_by_row_column_names() -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # execute the query
        cursor.execute("select employee_id, first_name, last_name from hr.employees")

        # using this method, you can access columns by name, internally python assigns a number to each column
        # the number of the column is the same as the index of the column in the result set
        # the number of columns must be the same as the number of columns in the select statement
        for employee_id, first_name, last_name in cursor:
            print(f"id:{employee_id}, firstname: {first_name}, lastname: {last_name}")

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

    db.conn.commit()  # commit any changes (close transaction)


def query_with_rowfactory_for_column_names() -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # execute the query
        cursor.execute("select * from hr.employees")

        # define a rowfactory that creates a dictionary with the column names, instead of the standard list per rows
        # do not use with statements containing multiple columns with the same name
        db.convert2dict(cursor)

        for row in cursor:
            print(f"id:{row['EMPLOYEE_ID']}, firstname: {row['FIRST_NAME']}, lastname: {row['LAST_NAME']}")

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

    db.conn.commit()  # commit any changes (close transaction)


def ddl_with_insert_showcasing_transactions() -> None:
    create_test_table()

    # create the table and insert some data
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # insert one row
        cursor.execute("insert into test values (:id, :name)", {'id': 1, 'name': 'test'})

        # insert multiple rows
        # be careful with executemany:
        #   in case of an error, the already inserted rows are not rolled back
        #   and the proceeding rows are not inserted
        data = [{'id': 2, 'name': 'test2'}, {'id': 3, 'name': 'test3'}]
        cursor.executemany("insert into test values (:id, :name)", data)

        # select all rows
        cursor.execute("select * from test")

        # fetch all results
        # be careful with large result sets, you might run out of memory --> use fetchmany or fetchone
        print("until now we have inserted 3 rows and are going to commit the transaction")
        results = cursor.fetchall()
        prettyprint.print_results(results, cursor)
    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

    db.conn.commit()  # commit any changes (close transaction)

    print("we now have committed our work and the table should contain 3 rows")

    # let's now insert another row without committing and then provoking an error to see what happens
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        print("we are inserting another row")
        # insert one row
        cursor.execute("insert into test values (:id, :name)", {'id': 4, 'name': 'test4'})

        # let's see how many rows we have
        cursor.execute("select * from test")
        results = cursor.fetchall()
        prettyprint.print_results(results, cursor)
        print("until now we have inserted 4 rows and are going to provoke an error, but last inserted row is not "
              "committed yet")
        print("let's provoke an error by inserting a row with an already existing id")
        cursor.execute("insert into test values (:id, :name)", {'id': 1, 'name': 'test-error'})

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        db.conn.rollback()  # rollback any changes in case of an error
        print("transaction rolled back, no changes committed")
        # raise e # in this case we do not want to rethrow the error, because we want to continue
    else:
        db.conn.commit()  # commit any changes if there was no error
    finally:
        cursor.close()  # always close the cursor when done

    print("let's see how many rows we have now")
    cursor = db.conn.cursor()
    cursor.execute("select * from test")
    results = cursor.fetchall()
    prettyprint.print_results(results, cursor)
    cursor.close()
    print("we have 3 rows left, because we did not commit the 4th insert and the the transaction was rolled back")

    drop_test_table()


def load_sql_from_file(file: str) -> None:
    data = None
    # with automatically closes the file (no need to call close)
    with open(file) as f:  # load the yaml file
        data = yaml.load(f, Loader=SafeLoader)
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # execute the first query in the file
        cursor.execute(data['my_first_sql_statement'])
        print(f"Oracle system time: {cursor.fetchone()[0]}")

        # execute the second query and provide the parameters using bind variables
        result = cursor.execute(data['my_second_sql_statement'], {'department_name': 'S%', 'last_name': 'M%'})
        db.convert2dict(cursor)
        for row in result:
            print('Person ' + row['FIRST_NAME'] + ' ' + row['LAST_NAME'] + ' works in department ' +
                  row['DEPARTMENT_NAME'])

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

    db.conn.commit()  # commit any changes (close transaction)


def nested_transactions() -> None:
    create_test_table()

    # start our transaction
    db.conn.begin()  # start transaction (usually not needed, but still best practice)
    with (db.conn.cursor() as cursor):
        cursor.execute("insert into test values (:id, :name)", {'id': 1, 'name': 'test'})
        print("we have inserted a row, but we have not committed the transaction yet")
        cursor.execute("select * from test")
        prettyprint.print_results(cursor.fetchall(), cursor)
        print("but we are going to make a savepoint, so we can rollback to this point later")
        cursor.execute("savepoint firstinsert")
        print("now let's make another 2 inserts where the second is provoking an error and using exception handling "
              "to rollback to the savepoint")
        try:
            cursor.execute("insert into test values (:id, :name)", {'id': 2, 'name': 'test2'})
            cursor.execute("insert into test values (:id, :name)", {'id': 1, 'name': 'test3'})
        except oracledb.DatabaseError as e:
            print("An error occurred executing the query:", e)
            print("we are now rolling back to the savepoint")
            cursor.execute("rollback to firstinsert")
            print("we have rolled back to the savepoint, but we still have not committed the transaction yet")

        print("let's see how many rows we have, the first row should still be there, the second should be gone and "
              "third was the one provoking the error")
        cursor.execute("select * from test")
        results = cursor.fetchall()
        prettyprint.print_results(results, cursor)
        print("we can now decide if we want to commit the transaction or rollback the whole transaction")
        print("let's rollback to the start of the transaction")
        db.conn.rollback()
        print("transaction rolled back, everything should be gone now")
        cursor.execute("select * from test")
        prettyprint.print_results(cursor.fetchall(), cursor)

    drop_test_table()


def change_isolation_level() -> None:
    print("each transaction in oracle starts in READ COMMITTED mode")
    print("changing isolation level MUST be the first statement in a transaction")
    print("we are using the helper class OracleDatabase to simplify it for you (have a look at the source code)")
    print("lets set the transaction level to SERIALIZABLE")
    db.conn.begin()
    db.set_isolation_level(IsolationLevel.SERIALIZABLE)
    db.conn.commit()
    print("after transaction has been closed the isolation level is back to READ COMMITTED")


def lock_table() -> None:
    print("be careful with locking tables, if possible use a 'SELECT ... FOR UPDATE' statement if you want to lock a "
          "row only.")
    print("be aware of the fact that locks are only released at the end of the transaction")
    create_test_table()
    db.conn.begin()
    # this method is for your convenience, it is not part of the oracledb module, look at the source code
    print("first we get a SHARED lock to the table, meaning other transactions can still read from the table, "
          "but not update it")
    db.lock_table("test", LockMode.SHARED)
    print("SHARED locks can be upgraded to EXCLUSIVE locks, but not the other way around")
    db.lock_table("test", LockMode.EXCLUSIVE)
    print("locks are only released at the end of the transaction")
    db.conn.commit()

def date_handling():
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # execute the query,  always use bind variables to prevent SQL injection
        cursor.execute("select * from hr.employees "
                       "where hire_date <:datum", {'datum': datetime.date(2003, 1, 1)})

        # fetch all results
        # be careful with large result sets, you might run out of memory --> use fetchmany or fetchone in a loop
        results = cursor.fetchall()

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        cursor.close()
        raise e

    prettyprint.print_results(results, cursor)

    cursor.close()  # always close the cursor when done

    db.conn.commit()  # commit any changes (close transaction)

# Login Attempt

def login(data: str, lname: str, bdate: str) -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # Parameters
        params = {
            "last_name": lname,
            "birth_d": bdate
        }
        # execute the login query
        cursor.execute(data['customer_exists'], params)
        customer = cursor.fetchone()

        if customer:
            print(f"Success! Welcome {lname}.")
            return customer
        else:
            # 4. Handle "Not Found" case
            print(f"\n[!] Customer {lname} ({bdate}) not found.")
            print("1. Try again")
            print("2. Register new account")
            print("3. Back to menu")
            
            choice = input("Selection: ")
            if choice == '1':
                # Ask for new info and call itself again (recursion)
                new_ln = input("Enter Last Name: ")
                new_bd = input("Enter Birth Date: ")
                return login(data, new_ln, new_bd)
            elif choice == '2':
                # Call your registration function here
                # return register(file, lname, bdate)
                print("Redirecting to registration...")
                return registration(data)
            else:
                return None

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

def registration(data: str) -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # Get firstname, lastname and birthdate
        fname = input("Firstname: ")
        lname = input("Lastname: ")
        bdate = input("Date of Birth (DD.MM.YYYY): ")
        # Parameters
        params = {
            "first_n": fname,
            "last_n": lname,
            "birth_d": bdate
        }
        # execute the login query
        cursor.execute(data['new_customer'], params)
        # Commit transaction
        db.conn.commit()

        print(f"Successfully registered {lname}!")

        # 3. After registering, we usually want to fetch the new user 
        # so they are "logged in" automatically.
        params = {
            "last_name": lname,
            "birth_d": bdate
        }
        cursor.execute(data['customer_exists'], params)
        return cursor.fetchone()

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

# Find a Media
def mediaSearch(data: str) -> None:
    print("\n*** MEDIA SEARCH ***")
    print("Leave a field blank if you don't want to filter by it.")
        
    # Get user input
    t_input = input("Search Title: ").strip()
    g_input = input("Search Genre: ").strip()
    m_type  = input("Search Media-Type: ").strip()

    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # Parameters
        params = {
            "title": f"%{t_input}%" if t_input else "___NONE___",
            "genre": g_input if g_input else "___NONE___",
            "type": m_type if m_type else "___NONE___"
        }
        # execute the login query
        cursor.execute(data['search_media_genre_type_title'], params)
        columns = [col[0] for col in cursor.description]
        results = cursor.fetchall()

        if results:
            print(f"\nFound {len(results)} matches:")
            print("-" * 30)
            for row in results:
                # Print as a dictionary or formatted string
                print(dict(zip(columns, row)))
        else:
            print("No media found matching those criteria.")


    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    # get credentials using the credentials_helper
    # default constructor first tries to read credentials from environment variables
    # if this fails, it tries to read credentials from a file
    # if this fails, it tries to read credentials from the user input
    credentials = credentials_helper.CredentialsHelper()

    # create database connection in constructor of OracleDatabase class
    # this is a helper class to simplify the connection handling for you
    # have a look at the source code to see how it works
    db = OracleDatabase(username=credentials.get_username(), password=credentials.get_password(),
                        dsn=credentials.get_dsn())
    # turn auto commit off, we want to control the transactions ourselves
    db.auto_commit(False)

    file = "my_sql_file.yaml"

    data = None
    customer = None
    # with automatically closes the file (no need to call close)
    with open(file) as f:  # load the yaml file
        data = yaml.load(f, Loader=SafeLoader)
    # Read user input in a loop
    while True:
        # If no one is logged in, show the Auth Menu
        if not customer:
            print("\nWelcome to the Library!")
            choice = input("1. Login\n2. Create Account\nq. Quit\nSelection: ")

            if choice == '1':
                lname = input("Lastname: ")
                bdate = input("Date of Birth (DD.MM.YYYY): ")
                customer = login(data, lname, bdate)
            elif choice == '2':
                customer = registration(data)
            elif choice == 'q':
                break
        
        # If someone IS logged in, show the Library Menu
        else:
            print(f"\n--- Logged in as {customer[1]} ---") # Assuming index 1 is name
            print("1. Search for Media")
            print("2. Return Media")
            print("q. Logout")
            
            subchoice = input("Selection: ")
            if subchoice == '1':
                mediaSearch(data)
            elif subchoice == '2':
                # returnMedia(data) # Create this function later!
                pass 
            elif subchoice == 'q':
                print("Logging out...")
                customer = None  # This sends them back to the Login menu


    # when the program exits, the destructor of the OracleDatabase class will be called
    # and the database connection will be closed

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
