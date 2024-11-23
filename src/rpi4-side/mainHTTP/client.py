import cv2
import requests
import numpy as np
from ultralytics import YOLO
from collections import deque, Counter
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--ip", help="Insert the server's ip address")
args = parser.parse_args()
print(args.ip)

# Model and streak deque initialization
detection_model = YOLO('detection.pt')
streak = deque(maxlen=10)

"""
This function creates and returns a dictionary to be sent as data through an http request
"""
def request_dict(class_id, fast_stop=0):
    detection_dict = {'class' : class_id, # Class predicted by the model
                'fast': fast_stop # Condition to immediately stop the paddle
                }
    
    return detection_dict

# URL of the MJPEG stream
url = f'http://{args.ip}:8000/stream.mjpg'

# Open a connection to the MJPEG stream
get_response = requests.get(url, stream=True)

# Ensure the connection was successful
if get_response.status_code == 200:
    
    # Setup buffer to hold the incoming image bytes
    byte_data = b''  
    
    for chunk in get_response.iter_content(chunk_size=4096):
        byte_data += chunk

        # Find the start and end of a JPEG frame
        start = byte_data.find(b'\xff\xd8')  # JPEG start 
        end = byte_data.find(b'\xff\xd9')  # JPEG end
        
        if start != -1 and end != -1:
            # Extract the JPEG frame
            jpg_frame = byte_data[start:end+2]
            # Remove the processed frame from buffer
            byte_data = byte_data[end+2:]

            # Decode the JPEG frame to a opencv image
            array = np.frombuffer(jpg_frame, dtype=np.uint8)        
            image = cv2.imdecode(array, cv2.IMREAD_COLOR)

            # Run the model inference on the image
            detection = detection_model(image)[0]

            # UNCOMMENT FOR PLOTTING THE PREDICTIONS
            '''
            for data in detection.boxes.data.tolist():
                if float(data[4]) < 0.70:
                    continue

                xmin, ymin, xmax, ymax = int(data[0]), int(data[1]), int(data[2]), int(data[3])
                cv2.rectangle(image, (xmin, ymin) , (xmax, ymax), (0, 255, 0), 2)
            
            cv2.imshow("Image", image)

            if cv2.waitKey(1) == ord("q"):
                break
            '''

            predicted_class = 0.0
            boxes_list = []

            # For each prediction, 
            for data in detection.boxes.data.tolist():
                class_id = data[5]
                boxes_list.append(class_id)
            
            c = Counter(boxes_list)

            if len(c) > 1:
                if  c.most_common(1) != c.most_common(2):
                    predicted_class = c.most_common(1) + 1
                else: predicted_class = 4
            else: predicted_class = c.most_common(1) + 1

            # Appends the prediction to the streak deque    
            if predicted_class != 0:
                streak.append(int(predicted_class))

            # Checks if it is the first prediction
            if predicted_class != 0 and len(streak) == 1:
                # Saves data in a dictionary
                dict = request_dict(class_id=float(streak[0]), fast_stop=1)
                # Sends a post request with the data
                post_response = requests.post(url, data=dict)
                

            # Checks if the last 10 predictions are all the same
            if predicted_class != 0 and len(streak) == 10 and all(streak[i] == streak[0] for i in range(len(streak))):
                # Saves data in a dictionary 
                dict = request_dict(class_id=float(streak[0]))
                # Clears the streak deque
                streak.clear()
                # Sends a post request with the data
                post_response = requests.post(url, data=dict)

else:
    print(f"Failed to connect to the stream: {get_response.status_code}")

# UNCOMMENT FOR PLOTTING THE PREDICTIONS
# cv2.destroyAllWindows()