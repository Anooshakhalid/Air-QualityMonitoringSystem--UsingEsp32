from flask import Flask, render_template, jsonify
import requests

app = Flask(__name__)

# ThingSpeak Configuration
CHANNEL_ID = '2934087'  # Replace with your channel ID
READ_API_KEY = ''  # If public, set as ''
NUMBER_OF_RESULTS = 20


def fetch_thingspeak_data():
    if READ_API_KEY:
        url = f"https://api.thingspeak.com/channels/{CHANNEL_ID}/feeds.json?api_key={READ_API_KEY}&results={NUMBER_OF_RESULTS}"
    else:
        url = f"https://api.thingspeak.com/channels/{CHANNEL_ID}/feeds.json?results={NUMBER_OF_RESULTS}"

    response = requests.get(url)
    if response.status_code == 200:
        return response.json()
    else:
        print("Error fetching data:", response.status_code)
        return None


@app.route('/')
def home():
    return render_template('home.html')


@app.route('/dashboard')
def index():
    return render_template('index.html')


@app.route('/data')
def data():
    thingspeak_data = fetch_thingspeak_data()
    if thingspeak_data:
        feeds = thingspeak_data.get('feeds', [])
        data = [{
            'created_at': feed['created_at'],
            'entry_id': feed['entry_id'],
            'temp': feed['field1'],
            'humidity': feed['field2'],
            'dust': feed['field3'],
            'light': feed['field4']
        } for feed in feeds]
        return jsonify({'data': data})
    else:
        return jsonify({'data': []})


if __name__ == '__main__':
    app.run(debug=True)
