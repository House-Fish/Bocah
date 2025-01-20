from flask import Flask, request, jsonify
from datetime import datetime
import uuid
import json
import os

app = Flask(__name__)

# Constants
DATABASE_FILE = 'events_db.json'

# Initialize empty database structure
initial_db = {
    'transportation': {},  # device_id -> {mode -> duration}
    'air_conditioner': {}  # device_id -> duration
}

def load_db():
    if os.path.exists(DATABASE_FILE):
        with open(DATABASE_FILE, 'r') as f:
            return json.load(f)
    return initial_db

def save_db(db):
    with open(DATABASE_FILE, 'w') as f:
        json.dump(db, f, indent=2)

def validate_uuid(device_id):
    try:
        uuid.UUID(device_id)
        return True
    except ValueError:
        return False

def validate_transportation_mode(mode):
    valid_modes = ["car", "bike", "walk", "bus", "train"]
    return mode in valid_modes

@app.route('/events', methods=['POST'])
def record_event():
    data = request.get_json()
    
    # Validate common fields
    if not all(k in data for k in ['type', 'device_id', 'duration', 'message']):
        return jsonify({'error': 'Missing required fields'}), 400
    
    if not validate_uuid(data['device_id']):
        return jsonify({'error': 'Invalid device_id format'}), 400
    
    if not isinstance(data['duration'], (int, float)):
        return jsonify({'error': 'Duration must be a number'}), 400
    
    event_type = data['type']
    device_id = data['device_id']
    duration = int(data['duration'])

    try:
        db = load_db()

        if event_type == 'transportation':
            # Validate transportation-specific fields
            if 'mode' not in data:
                return jsonify({'error': 'Missing mode for transportation event'}), 400
            
            mode = data['mode']
            if not validate_transportation_mode(mode):
                return jsonify({'error': 'Invalid transportation mode'}), 400
            
            # Initialize nested dictionaries if they don't exist
            if device_id not in db['transportation']:
                db['transportation'][device_id] = {}
            if mode not in db['transportation'][device_id]:
                db['transportation'][device_id][mode] = 0
            
            # Update duration
            db['transportation'][device_id][mode] += duration

        elif event_type == 'air-conditioner':
            # Initialize if doesn't exist
            if device_id not in db['air_conditioner']:
                db['air_conditioner'][device_id] = 0
            
            # Update duration
            db['air_conditioner'][device_id] += duration

        else:
            return jsonify({'error': 'Invalid event type'}), 400

        save_db(db)
        
        return jsonify({
            'success': True,
            'eventId': str(uuid.uuid4()),
            'deviceId': device_id,
            'timestamp': datetime.utcnow().isoformat()
        })

    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/stats/<device_id>', methods=['GET'])
def get_stats(device_id):
    if not validate_uuid(device_id):
        return jsonify({'error': 'Invalid device_id format'}), 400

    db = load_db()
    
    # Get transportation stats
    transport_data = db['transportation'].get(device_id, {})
    
    # Get AC stats
    ac_duration = db['air_conditioner'].get(device_id, 0)

    return jsonify({
        'device_id': device_id,
        'transportation': transport_data,
        'air_conditioner': ac_duration
    })

@app.route('/reset', methods=['POST'])
def reset_database():
    try:
        save_db(initial_db)
        return jsonify({
            'success': True,
            'message': 'Database reset successfully'
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    # Ensure database file exists
    if not os.path.exists(DATABASE_FILE):
        save_db(initial_db)
    app.run(host="0.0.0.0", port=5000)