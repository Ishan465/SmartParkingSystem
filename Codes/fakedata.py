import datetime
import random
import firebase_admin
from firebase_admin import credentials, db

# Initialize Firebase with your credentials
cred = credentials.Certificate(r"C:\Users\ISHAN\Downloads\smart-parking-b5283-firebase-adminsdk-fbsvc-d55ff837f3.json")  # Replace with your service account key file path
firebase_admin.initialize_app(cred, {
    'databaseURL': "https://smart-parking-b5283-default-rtdb.firebaseio.com/"  # Replace with your Firebase database URL
})

def generate_fake_data():
    """Generate fake parking data for the past 30 days."""
    # Define the date range: 30 days ago to yesterday
    today = datetime.datetime.now()
    start_date = today - datetime.timedelta(days=30)
    end_date = today - datetime.timedelta(days=1)
    
    current_date = start_date
    while current_date <= end_date:
        for slot in range(1, 5):  # For slots 1 to 4
            # Start of the day at 00:00, slot is empty
            day_start = current_date.replace(hour=0, minute=0, second=0, microsecond=0)
            timestamp = day_start
            occupied = False
            write_history_entry(slot, timestamp, occupied)
            
            # Simulate a few parking sessions per day
            while timestamp < day_start + datetime.timedelta(days=1):
                if not occupied:
                    # Car arrives: choose a random time within the next 0-4 hours
                    arrival_time = timestamp + datetime.timedelta(hours=random.uniform(0, 4))
                    # Stop if arrival is after 8pm to limit late-night activity
                    if arrival_time > day_start + datetime.timedelta(hours=20):
                        break
                    timestamp = arrival_time
                    occupied = True
                else:
                    # Car leaves: choose a random duration of 1-8 hours
                    duration = datetime.timedelta(hours=random.uniform(1, 8))
                    leave_time = timestamp + duration
                    # Cap the leave time at the end of the day (11:59:59)
                    if leave_time > day_start + datetime.timedelta(days=1):
                        leave_time = day_start + datetime.timedelta(days=1) - datetime.timedelta(seconds=1)
                    timestamp = leave_time
                    occupied = False
                # Log the change in occupancy
                write_history_entry(slot, timestamp, occupied)
        
        # Move to the next day
        current_date += datetime.timedelta(days=1)

def write_history_entry(slot, timestamp, occupied):
    """Write a history entry to Firebase for the given slot and timestamp."""
    # Generate timestamp in milliseconds since epoch as the key
    timestamp_ms = int(timestamp.timestamp() * 1000)
    # Format timestamp as a human-readable string (matching Arduino format)
    timestamp_str = timestamp.strftime("%Y-%m-%d %H:%M:%S")
    
    # Reference to the history path for the slot
    ref = db.reference('/parking_history/slot' + str(slot))
    ref.child(str(timestamp_ms)).set({
        'occupied': occupied,
        'timestamp': timestamp_str
    })
    print(f"Slot {slot} at {timestamp_str}: occupied={occupied}")

# Run the data generation
if __name__ == "__main__":
    generate_fake_data()