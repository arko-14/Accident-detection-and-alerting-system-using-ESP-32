from flask import Flask, request, jsonify, render_template

app = Flask(__name__)

# Initialize data storage
data = {
    "accX": 0,
    "accY": 0,
    "accZ": 0,
    "jerkX": 0,
    "jerkY": 0,
    "jerkZ": 0,
    "latitude": "",
    "longitude": "",
    "buzzer_status": "OFF",
    "sms_status": "Not Sent"
}

@app.route('/updateData', methods=['POST'])
def update_data():
    global data
    content = request.json
    data["accX"] = content.get("accX", 0)
    data["accY"] = content.get("accY", 0)
    data["accZ"] = content.get("accZ", 0)
    data["jerkX"] = content.get("jerkX", 0)
    data["jerkY"] = content.get("jerkY", 0)
    data["jerkZ"] = content.get("jerkZ", 0)
    data["latitude"] = content.get("latitude", "")
    data["longitude"] = content.get("longitude", "")

    # Check if jerk exceeds threshold
    total_jerk = (data["jerkX"]**2 + data["jerkY"]**2 + data["jerkZ"]**2)**0.5
    if total_jerk > 10.0:
        data["buzzer_status"] = "ON"
        data["sms_status"] = "Sent"
    else:
        data["buzzer_status"] = "OFF"
        data["sms_status"] = "Not Sent"

    return jsonify({"status": "success"})

@app.route('/')
def index():
    return render_template('index.html', data=data)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
