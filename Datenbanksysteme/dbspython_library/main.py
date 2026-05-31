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
def mediaSearch(data: str, customer_id) -> None:
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
        # Convert input to lowercase and wrap in % for a LIKE search
        "title": f"%{t_input.lower()}%" if t_input else "___NONE___",
        "genre": g_input.lower() if g_input else "___NONE___",
        "type": m_type.lower() if m_type else "___NONE___"
        }   
        # execute the login query
        cursor.execute(data['search_media_genre_type_title'], params)
        columns = [col[0] for col in cursor.description]
        results = cursor.fetchall()

        if results:
            print(f"\nFound {len(results)} matches:")
            print("-" * 30)
            for i, row in enumerate(results, start=1):
                d = dict(zip(columns, row))
                print(f"{i:2}. {d['TITLE']:<20} | {d['CATEGORY']:<10} | {d['TYPE']}")

            print("-" * 30)
            choice = input("Enter the number to borrow/reserve (or 'b' to go back): ")

            if choice.isdigit():
                index = int(choice) - 1  # Convert back to 0-based index
                if 0 <= index < len(results):
                    selected_media = results[index]
                    print(f"You selected: {selected_media[1]}") # Assuming index 1 is Title
                    fsk_check(data, selected_media[0], customer_id)
                else:
                    print("Invalid number.")
            elif choice.lower() == 'b':
                return
        else:
            print("No media found matching those criteria.")


    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done


def fsk_check(data: str, media_id, customer_id) -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()
    
        # execute the login query
        cursor.execute(data['fsk_check'], {"media_id": media_id, "c_id": customer_id})
        allowed_media = cursor.fetchone()

        if allowed_media:
            print("Access granted! You are old enough.")
            copy_available(data, media_id, customer_id)
        else:
            print("Access denied: You do not meet the age requirement for this media.")

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

def copy_available(data: str, media_id, customer_id) -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()
    
        # execute the login query
        cursor.execute(data['is_copy_available'], {"media_id": media_id})
        copy = cursor.fetchone()

        if copy:
            barcode = copy[0]
            check_reservations_queue(data, media_id, barcode,customer_id)
        else:
            print("There is no available copy of this media currently. Want to reserve it? (y/n)")
            choice = input("Selection: ")
            if choice.lower() == 'y':
                # call reservation function here
                reservation(data, media_id, customer_id)
            else:
                print("Returning to media search results...")
            
    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

def borrow(data: str, customer_id, barcode) -> bool:
    try:
        # Transactions start automatically in oracledb when executing DML/Queries
        cursor = db.conn.cursor()

        # 1. FAIL-SAFE: Check if this user already borrowed this item TODAY
        cursor.execute(data['check_borrowed_today'], {"c_id": customer_id, "barcode": barcode})
        already_borrowed_today = cursor.fetchone()
        
        if already_borrowed_today:
            print("\n[!] Policy Restriction: You cannot borrow the same item twice on the same day.")
            print("    Please choose a different media or try again tomorrow.")
            return False

        # 2. Proceed with the insert if the check passes
        cursor.execute(data['new_ledger_entry'], {"c_id": customer_id, "barcode": barcode})
        
        # Commit the transaction safely
        db.conn.commit()
        print(f"\nSuccessfully borrowed the Media!\nPlease return it in 21 Days.")
        return True

    except oracledb.IntegrityError as e:
        # Backup fail-safe in case of concurrent requests hitting the unique key constraint
        print("\n[!] Database Restriction: This item was already registered to you today.")
        db.conn.rollback()
        return False
    except oracledb.DatabaseError as e:
        print("\nAn error occurred executing the query:", e)
        db.conn.rollback()
        raise e
    finally:
        cursor.close()


def currently_borrowed(data: str, customer_id) -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # execute the login query
        cursor.execute(data['get_all_borrowed'], {"c_id": customer_id })
        columns = [col[0] for col in cursor.description]
        results = cursor.fetchall()

        if results:
            prettyprint.print_results(results, cursor)
            barcode = input("Enter the barcode of the media you want to return: ")
            returnCopy(data, customer[0], barcode)
        else:
            print("You currently have no borrowed media.")

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done


def returnCopy(data: str, customer_id: int, barcode: str) -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()
        #check if the copy is currently borrowed by the customer
        cursor.execute(data['check_borrowed_by_customer'], {"c_id": customer_id,"barcode": barcode })
        borrowed = cursor.fetchone()
        if not borrowed:
            print("This copy is not currently borrowed by you.")
            return
        
        #Calculate the cost and save it in the ledger
        cursor.execute(data['calculate_cost_save_in_ledger'], {"barcode": barcode })

        cursor.execute(data['get_price_to_pay'], {"barcode": barcode })

        priceToPay = cursor.fetchone()[0]
        #Python checks from left to right
        if priceToPay is not None and priceToPay > 0:
            print(f"You have to pay {priceToPay}€ for returning this media late.")
            #payment process
            input("Press Enter to confirm payment...") # Simulate payment confirmation
            cursor.execute(data['returnCopy'], {"barcode": barcode, "paid": priceToPay })
        else:
            cursor.execute(data['returnCopy'], {"barcode": barcode, "paid": 0 })

        
        update_reservation_onreturn(data, media_id=borrowed[7])
        statistic_update_on_return(data, barcode)

        # Commit transaction
        db.conn.commit()

        print(f"Successfully returned the Copy!\nThank you!")

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

def update_reservation_onreturn(data: str, media_id: str) -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # execute the login query
        cursor.execute(data['update_reservation_on_return'], {"media_id": media_id })
        # Commit transaction
        db.conn.commit()

        print(f"Reservation updated successfully.")

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

def reservation(data: str, media_id, customer_id) -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # execute the login query
        cursor.execute(data['create_reservation'], {"c_id": customer_id,"media_id": media_id })
        # Commit transaction
        db.conn.commit()

        print(f"Successfully reserved the Media!\nYou will be notified when it is available.")

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

def check_reservations(data: str, customer_id) -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # execute the login query
        cursor.execute(data['check_reservations'], {"c_id": customer_id })
        reservations = cursor.fetchall()

        if reservations:
            print("Open Reservations:")
            prettyprint.print_results(reservations, cursor)
        else:
            print("No open reservations.")

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

def statistic_update_on_return(data: dict, barcode) -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # execute the login query
        cursor.execute(data['statistic_update_on_return'], {"barcode": barcode })
        print("Rows updated:", cursor.rowcount)
        print(f"Statistics updated for returned media with barcode: {barcode}")
        # Commit transaction
        db.conn.commit()
        print(f"Statistics updated successfully.")
         #print stats
        cursor.execute(data['get_media_statistics'], {"barcode": barcode })
        print("Current statistics for this media:")
        prettyprint.print_results(cursor.fetchall(), cursor)


    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done


#Check if the reservation queue is empty or you are the next customer for a media, so it can be borrowed, using next_in_reservation_queue
def check_reservations_queue(data: str,media_id, barcode, customer_id) -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # execute the login query
        cursor.execute(data['next_in_reservation_queue'], {"media_id": media_id })
        next_customer = cursor.fetchone()

        #test print return
        #print(f"Next customer in reservation queue: {next_customer} and customer: {next_customer[2]} and {customer_id}")

        #0=Index,1=datetime,2=customer_id,3=media_id,4=status,5=barcode,6=start_date (ausgeliehen)
        if next_customer and next_customer[2] == customer_id:
            print("You are next in the reservation queue for this media. You can borrow it.")
            borrow(data, customer_id, barcode)
            media_picked_up(data, customer_id, barcode, next_customer[0])
            print(f"Media borrowed successfully.")
        elif next_customer:
            print(f"There are customers ahead of you in the reservation queue for this media. Wanna reserve it anyway? (y/n)")
            choice = input("Selection: ")

            if choice.lower() == 'y':
                # call reservation function here
                reservation(data, media_id, customer_id)
            else:
                print("Returning to media search results...")

        else:
            print("The reservation queue for this media is empty.")
            if borrow(data, customer_id, barcode):
                print(f"Media borrowed successfully.")
            else:
                print(f"Wait a bit before reborrowing this media.")

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

def media_picked_up(data: str, customer_id, barcode, res_id) -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # execute the login query
        cursor.execute(data['media_picked_up'], {"res_id": res_id,"c_id": customer_id, "barcode": barcode })
        statistic_update_on_return(data, barcode)
        # Commit transaction
        db.conn.commit()

        print(f"Media pickup confirmed successfully.")

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

def check_failed_pickup(data: str) -> None:
    try:
        cursor = db.conn.cursor()
        
        # 1. Identify which ones are about to fail
        cursor.execute(data['select_failed_pickup'])
        failed_pickups = cursor.fetchall()

        if failed_pickups:
            # 2. Update them to 'FAILED'
            cursor.execute(data['check_failed_pickup'])
            db.conn.commit()
            
            print("People failed their reservations, maybe now you can profit from it:")
            # Note: access the correct index for barcode/ID from failed_pickups
            print(f"Next customer in reservation queue: {failed_pickups}")

            for row in failed_pickups:
                media_id = row[3] # Based on your previous output, index 3 is Media_ID
                print(f"Promoting next customer for Media ID: {media_id}")
                
                # This calls your SQL once for every failed media type
                cursor.execute(data['update_reservation_on_return'], {"media_id": media_id})
                db.conn.commit()  # Commit after processing
            
        else:
            print("No failed pickups.")

    except oracledb.DatabaseError as e:
        print("An error occurred executing the query:", e)
        print_exc()  # print stack trace
        db.conn.rollback()  # rollback any changes in case of an error
        raise e
    finally:
        cursor.close()  # always close the cursor when done

def increase_reservation_time(data: str, customer_id) -> None:
    try:
        db.conn.begin()  # start transaction (usually not needed, but still best practice)

        # we need a cursor to execute queries
        cursor = db.conn.cursor()

        # execute the login query
        cursor.execute(data['increase_reservation_time'], {"c_id": customer_id })
        db.conn.commit()

        print(f"Successfully increased reservation time for all your reservations by 2 days!")

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
        check_failed_pickup(data) #check if there are failed pickups before showing the menu, so customers can profit from it
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
            

            print(f"\n--- Logged in as {customer[1]} ---")
            check_reservations(data, customer[0])
            print("1. Search for Media")
            print("2. Return Media")
            print("3. Check Reservations")
            print("4. More Reservation Time")
            print("q. Logout")
            
            subchoice = input("Selection: ")
            if subchoice == '1':
                mediaSearch(data, customer[0])
            elif subchoice == '2':
                #print all currently borrowed media by the customer
                print("\nYour currently borrowed media:")
                currently_borrowed(data, customer[0])
            elif subchoice == '3':
                print("\nYour current reservations:")
                check_reservations(data, customer[0])
            elif subchoice == '4':
                increase_reservation_time(data, customer[0])
            elif subchoice == 'q':
                print("Logging out...")
                customer = None  # This sends them back to the Login menu


    # when the program exits, the destructor of the OracleDatabase class will be called
    # and the database connection will be closed

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
