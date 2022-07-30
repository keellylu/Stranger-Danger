import sqlite3
import datetime
import requests
import json
import smtplib, ssl

settings_db = '/var/jail/home/team63/userinfo/settings.db'

def vigenere_cipher(password, username, decode=False):
    '''
    Encrypts the user's password using a Vigenere ciphere, where the key is the user's username. 
    '''
    keyword = username
    data = password
    l_data = len(data)
    l_keyword = len(keyword)
    index = 0
    fixed_keyword = ""

    # Make sure the length of the keyword is the same as the length of the message
    for i in range(1, (l_data+1)):
        if (i - 1) % (l_keyword) == 0:
            index = 0
        fixed_keyword += keyword[index]
        index += 1  
    
    if decode == False: #encrypt message
        encrypted = ""
        for i in range(len(data)):
            shifted = (ord(data[i]) + (ord(fixed_keyword[i])-32))
            if shifted > 126:
                shifted = (shifted % 126) + 31
            encrypted += (chr(shifted))
        return encrypted
    else: #decrypt message
        decrypted = ""
        for i in range(len(data)):
            shifted = (ord(data[i]) - (ord(fixed_keyword[i]) - 32))
            if shifted < 32:
                shifted = 127 - (32 - shifted)
            decrypted += (chr(shifted))
        return decrypted


def request_handler(request):
    if request['method'] == 'GET':
        # See all users in the database
        if request['values']['view'] == 'true':
            conn = sqlite3.connect(settings_db)
            c = conn.cursor() 
            things = c.execute('''SELECT * FROM dated_table''').fetchall()
            output_table = "User List<br>"
            for entry in things:
                contact = str(entry[1])
                keyword = str(entry[2])
                usr = str(entry[0])
                
                output_table += f"{usr}: emergency contact {contact}, keyword {keyword}"
                if entry != things[len(things)-1]:
                    output_table += "<br>"
                    output_table += "<br>"
            return output_table
        # Get the specific information associated with one user
        elif request['values']['view'] == 'false' and request['values']['action'] == 'get_user_info':
            conn = sqlite3.connect(settings_db)
            c = conn.cursor()
            user = request['values']['user']
            things = c.execute('''SELECT * FROM dated_table WHERE user = ?;''', (user,)).fetchone()
            conn.commit()
            conn.close()
            return {"user":things[0], "recipient":things[1], "keyword":things[2], "email_body":things[4]}
        # Test if the password sent from the ESP matches the one we have stored in the database
        elif request['values']['view'] == 'false' and request['values']['action'] == 'match_password':
            conn = sqlite3.connect(settings_db)
            c = conn.cursor()
            user = request['values']['user']
            things = c.execute('''SELECT * FROM dated_table WHERE user = ?;''', (user,)).fetchone()
            conn.commit()
            conn.close()
            if vigenere_cipher(things[5], things[0], True) == request['values']['password']:
                return "true"
            else:
                return "false"
        # See if the username is one that has been recorded in our database
        elif request['values']['view'] == 'false' and request['values']['action'] == 'see_if_user_exists':
            conn = sqlite3.connect(settings_db)
            c = conn.cursor()
            user = request['values']['user']
            things = c.execute('''SELECT * FROM dated_table WHERE user = ?;''', (user,)).fetchone()
            conn.commit()
            conn.close()
            if things is None:
                return "false"
            else:
                return "true"

        
    elif request['method'] == 'POST':
        user = request['form']['user']
        conn = sqlite3.connect(settings_db)
        c = conn.cursor()  # move cursor into database (allows us to execute commands)
        
        # Send the user's location to their close contact
        if request['form']['action'] == 'send_email':
            user_info = c.execute('''SELECT * FROM dated_table WHERE user = ?;''', (user,)).fetchall()
            location = request['form']['location']
            for entry in user_info:
                name = str(entry[3])
                receiver_email = str(entry[1])
            try:
                # Use SMTP protocol to send the email
                port = 465  # For SSL
                smtp_server = "smtp.gmail.com"
                sender_email = "strangerdangersystem@gmail.com"  # We created an email address for the project to avoid password security issues
                password = "sj3,N)e&m-A=^pA&"
                message = f"""\
                SAFETY ALERT NOTIFICATION

                {name}'s last location was {location}."""
                context = ssl.create_default_context()
                with smtplib.SMTP_SSL(smtp_server, port, context=context) as server:
                    server.login(sender_email, password)
                    server.sendmail(sender_email, receiver_email, message)
                   
                conn.commit() # commit commands
                conn.close() # close connection to database
                return "EMAIL SUCCESS"
            except:
                # Occurs when the email is not a valid email
                conn.commit() # commit commands
                conn.close() # close connection to database
                return "EMAIL ERROR"
    
        # The user wants to change their username, so we update this information in the database
        elif request['form']['action'] == 'change_username':
            password = request['form']['password']
            new_user = request['form']['new_user']
            user_info = c.execute('''SELECT * FROM dated_table WHERE user = ?;''', (user,)).fetchall()
            for us in user_info:
                if vigenere_cipher(us[5],user,True) == password:
                    existing_user = c.execute('''SELECT * FROM dated_table WHERE user = ?;''', (new_user,)).fetchall()
                    # We only update the user's username if it is an unused one, as we do not want two people to have the same username
                    if existing_user is None or len(existing_user) == 0:
                        c.execute('''UPDATE dated_table SET user = ? WHERE user = ?;''', (new_user, user))
                        conn.commit()
                        conn.close()
                        return "USERNAME CHANGED SUCCESSFULLY"
                    else:
                        return "USERNAME TAKEN"
            return "USER NOT FOUND"

        elif request['form']['action'] == 'set_settings':
            c.execute('''CREATE TABLE IF NOT EXISTS dated_table (user text, contact text, keyword text, name text, prefs text, password text);''')
            users = c.execute('''SELECT * FROM dated_table;''').fetchall()
            password = request['form']['password']
            # The user is a returning user, who has changed their on-record information in some way
            # This includes the password to the account, which can be modified as well
            if request['form']['account'] == 'update':
                keyword = request['form']['keyword']
                contact = request['form']['contact']
                name = request['form']['name']
                prefs = request['form']['prefs']
                for us in users:
                    if str(us[0]) == user:
                        if contact != "":
                            c.execute('''UPDATE dated_table SET contact = ? WHERE user = ?;''', (contact, user))
                        if keyword != "":
                            c.execute('''UPDATE dated_table SET keyword = ? WHERE user = ?;''', (keyword, user))
                        if name != "":
                            c.execute('''UPDATE dated_table SET name = ? WHERE user = ?;''', (name, user))
                        if prefs != "":
                            c.execute('''UPDATE dated_table SET prefs = ? WHERE user = ?;''', (prefs, user))
                        if password != "":
                            c.execute('''UPDATE dated_table SET password = ? WHERE user = ?;''', (vigenere_cipher(password, user), user))
                        conn.commit() # commit commands
                        conn.close() # close connection to database
                        return "UPDATE SUCCESS"
                    
                conn.commit() # commit commands
                conn.close() # close connection to database
                return "USERNAME NOT FOUND"
            # The only other "account" action available is creating a new account, since this is a new user
            else:
                for us in users:
                    if str(us[0]) == user:
                        return "USERNAME ALREADY EXISTS"
                c.execute('''INSERT into dated_table VALUES (?,?,?,?,?,?);''', (user, "", "", "", "", vigenere_cipher(password, user, False)))
                conn.commit() # commit commands
                conn.close() # close connection to database
                    
                return "INPUT SUCCESS"