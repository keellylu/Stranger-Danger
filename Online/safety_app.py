from flask import Flask,render_template,request,redirect, jsonify
import requests
import json
 
app = Flask(__name__)

coords = {}

@app.route('/location')
def form():
    return render_template('location.html')

@app.route('/index')
def update():
    return render_template('index.html')

@app.route('/create_account')
def account():
    return render_template('create_account.html')
 
@app.route('/verify', methods = ['POST', 'GET'])
def verify():
    if request.method == 'POST':
        latlon = request.form['location']
        user = request.form['user']
        
        return redirect((f"/tracker/{user}/{latlon}"))

@app.route('/tracker/<user>/<latlon>') 
def tracker(user, latlon):
    try:
        db_url = "https://608dev-2.net/sandbox/sc/team63/amberalert/amber_alert_system.py"
      
        parameters = {'location': latlon, 'user': user, 'action': 'online'}
     
        # sending get request 
        r = requests.get(url = db_url, params = parameters)
        locs = r.text
    
        coords = []
        curr = ""
        curr_d = {"lat": None, "lng": None}
        for x in locs: #parse returned string, create list of dicts with lat/lon as keys
            if x == ',':
                curr_d['lat'] = float(curr)
                curr = ""
            elif x == ";":
                curr_d['lng'] = float(curr)
                coords.append(curr_d)
                curr_d = {"lat": None, "lng": None}
                curr = ""
            else:
                curr += x
            
        return render_template("tracker.html", data = json.dumps(coords))
    except:
        return "Error. Please enter your location as specified on your device."

app.run(debug = True, host='localhost', port=5000)

