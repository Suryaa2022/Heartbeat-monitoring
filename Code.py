import time
import random

def measure_heartbeat():
    # Simulating heartbeat measurement
    heartbeat = random.randint(60, 100)
    return heartbeat

def monitor_heartbeat():
    while True:
        heartbeat = measure_heartbeat()
        print("Heartbeat: {} bpm".format(heartbeat))
        time.sleep(1)  # Wait for 1 second

if __name__ == "__main__":
    monitor_heartbeat()
