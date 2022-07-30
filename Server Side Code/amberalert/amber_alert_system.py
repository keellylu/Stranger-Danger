import sqlite3
import datetime
import numpy
from math import radians, cos, sin, asin, sqrt, pi
ht_db = '/var/jail/home/team63/amberalert/amberalert.db'
now = datetime.datetime.now()
        
def find_distance(lat_a, lon_a, lat_b, lon_b):
    """
    Uses the Haversine formula the find the distance in miles between two
    latitude/longitude coordinates
    """
    lat_a *= pi / 180.0
    lat_b *= pi / 180.0
    lon_a *= pi / 180.0
    lon_b *= pi / 180.0
      
    # Haversine formula
    adj_lon = lon_b - lon_a
    adj_lat = lat_b - lat_a
    a = sin(adj_lat / 2)**2 + cos(lat_a) * cos(lat_b) * sin(adj_lon / 2)**2
 
    val = 2 * asin(sqrt(a))
    
    #multiply by radius of Earth in miles
    return(val * 3956)
  
def find_bearing(lat_a, lon_a, lat_b, lon_b):
    """
    Use the formula to calculate the bearing angle between two latitude/longitude 
    coordinates
    """
    lat_a *= pi / 180.0
    lat_b *= pi / 180.0
    lon_a *= pi / 180.0
    lon_b *= pi / 180.0
    
    adj_lon = lon_b - lon_a
    
    x = cos(lat_b) * sin(adj_lon)
    y = cos(lat_a) * sin(lat_b) - sin(lat_a) * cos(lat_b) * cos(adj_lon)
    
    bearing = numpy.arctan2(x,y)
    bearing = numpy.degrees(bearing)

    return bearing

def parse_latlon(lat_lon):
    NS = 0
    EW = 0
    dot1 = 0
    dot2 = 0
    
    #parse DMS lat/lon string from the GPS
    for i in range(len(lat_lon)):
        if lat_lon[i] == ".":
            if dot1 == 0:
                dot1 = i
            else:
                dot2 = i
        elif lat_lon[i] in {"N", "S"}:
            NS = i
        elif lat_lon[i] in {"E", "W"}:
            EW = i
    
    #extract degree/minutes
    lat_deg = int(lat_lon[0 : dot1 - 2])
    lat_min = float(lat_lon[dot1 - 2 : NS])
    lon_deg = int(lat_lon[NS + 1 : dot2 - 2])
    lon_min = float(lat_lon[dot2 - 2 : EW])
    
    #convert DMS format to decimal lat/lon values
    lat = lat_deg + (lat_min / 60.0)
    lon = lon_deg + (lon_min / 60.0)
    if lat_lon[NS] == "S":
        lat = lat * (0 - 1.0)
    if lat_lon[EW] == "W":
        lon = lon * (0 - 1.0)
    
    return lat, lon
  
def request_handler(request):
    if request["method"] == "GET":
        HEIGHT = 50 #half the height in px of LCD display
        WIDTH = 50 #half the widthin px of LCD display
        SCALE = 200
        SHIFT = 14 
        
        lat_lon = str(request['values']['location'])
        user = str(request['values']['user'])
        lat, lon = parse_latlon(lat_lon)
        
        users_in_danger = []
        with sqlite3.connect(ht_db) as c:
            c.execute("""CREATE TABLE IF NOT EXISTS amber_data (time_ timestamp, user text, lat real, lon real);""")
            rows = c.execute('''SELECT * FROM amber_data;''').fetchall()
            
            for row in rows:
                lat2 = float(row[2])
                lon2 = float(row[3])
                dist = find_distance(lat, lon, lat2, lon2)
                if dist < 0.25: #if user in danger is within a quarter mile
                    bear = find_bearing(lat, lon, lat2, lon2)
                    angle = 90.0 - bear
                    
                    #Approximate the distance and angle as polar coordinates
                    #to be used for the circular LCD display
                    #Then convert to cartesian coordinates
                    x = SCALE*(dist * cos(angle)) 
                    y = SCALE*(dist * sin(angle))
                    
                    if lon2 < lon:
                        if x > 0:
                            x = 0.0 - x
                    if lon2 > lon:
                        if x < 0:
                            x = 0.0 - x
                    if lat2 < lat:
                        if y > 0:
                            y = 0.0 - y
                    if lat2 > lat:
                        if y < 0:
                            y = 0.0 - y
                    
                    #Adjust for LCD display centered at (64, 64)
                    x = round(WIDTH + SHIFT + x, 6)
                    y = round(HEIGHT + SHIFT - y, 6)
                    
                    if user != str(row[1]):
                        users_in_danger.append([str(row[1]), x, y, dist, angle, round(lat2, 6), round(lon2, 6)])
                    
        
        to_plot = "" #generate return string

        if "view" in request["values"].keys(): 
            to_plot = "Your location: (" + str(round(lat, 6)) + ', ' + str(round(lon, 6)) + ') <br> <br>'
        elif "action" in request["values"].keys():
            to_plot = str(round(lat, 6)) + ', ' + str(round(lon, 6)) + ';'
        for u in users_in_danger:
            if "view" in request["values"].keys():
                to_plot += u[0] + ': plotted point at (' + str(u[1]) + ', ' + str(u[2]) + ') for lon, lat (' + str(u[5]) + ', ' + str(u[6]) + ')'
                to_plot += "<br>"
            elif "action" in request["values"].keys():
                to_plot += str(u[5]) + ', ' + str(u[6]) + ';'
            else: #default for GET requests from the embedded side code
                to_plot += str(u[1]) + ',' + str(u[2]) + ';'
                
        return to_plot
              
    if request["method"]=="POST": #handle TIER 3 response for user in danger
        user = str(request['form']['user'])
        lat_lon = str(request['form']['location'])
        lat, lon = parse_latlon(lat_lon)

        with sqlite3.connect(ht_db) as c:
            c.execute("""CREATE TABLE IF NOT EXISTS amber_data (time_ timestamp, user text, lat real, lon real);""")
            
            rows = c.execute('''SELECT * FROM amber_data;''').fetchall()
            
            names = []
            for r in rows:
                names.append(r[1])

            if user in names: #if user is already in the database
                c.execute('''DELETE FROM amber_data WHERE user=?;''',(user,))
                c.execute('''INSERT into amber_data VALUES (?, ?, ?, ?);''',(datetime.datetime.now(), user, lat, lon))
            else: #if new entry
                c.execute('''INSERT into amber_data VALUES (?, ?, ?, ?);''',(datetime.datetime.now(), user, lat, lon))
                
        return "AMBER ALERT POST SUCCESS"
    else:
        return "Invalid HTTP method for this url."

