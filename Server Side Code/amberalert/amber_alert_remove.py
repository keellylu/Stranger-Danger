import sqlite3
import datetime
ht_db = '/var/jail/home/team63/amberalert/amberalert.db'
now = datetime.datetime.now()
        

def request_handler(request):
    if request["method"]=="POST":
        name = str(request['form']['user'])

        with sqlite3.connect(ht_db) as c:
            c.execute("""CREATE TABLE IF NOT EXISTS amber_data (time_ timestamp, user text, lat real, long real);""")
            c.execute('''DELETE FROM amber_data WHERE user=?;''',(name,))

            data_list = [] #for list representation
            data_string = "" #for formatted string representation

            rows = c.execute('''SELECT * FROM amber_data;''').fetchall()

            for r in rows: #('2022-04-16 21:12:38.567000', 'joe', 392.102, 4.203)
                data_list.append(r)

            for i in range(len(data_list)):
                r = data_list[(len(data_list)-1)-i]
                data_string += "User <{}> is in danger. Location: Lat {}, Long {}. Posted: {}.\n".format(r[1], str(r[2]), str(r[3]), str(r[0]))
                
        return data_string

    else:
        return "Invalid HTTP method for this url."
