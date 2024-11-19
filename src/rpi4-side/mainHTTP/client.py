import cv2
import requests
import numpy as np
from ultralytics import YOLO
from collections import deque

detection_model = YOLO('detection.pt')
streak = deque(maxlen=10)

# URL of the MJPEG stream (you need to adjust this to your actual stream URL)
url = 'http://192.168.84.24:8000/stream.mjpg'

# Open a connection to the MJPEG stream
get_response = requests.get(url, stream=True)

# Ensure the connection was successful
if get_response.status_code == 200:
    byte_data = b''  # Buffer to hold the incoming image bytes
    
    for chunk in get_response.iter_content(chunk_size=4096):
        byte_data += chunk

        # Find the start and end of a JPEG frame
        start = byte_data.find(b'\xff\xd8')  # JPEG start 
        end = byte_data.find(b'\xff\xd9')  # JPEG end
        
        if start != -1 and end != -1:
            # Extract the JPEG frame
            jpg_frame = byte_data[start:end+2]
            byte_data = byte_data[end+2:]  # Remove the processed frame from buffer

            # Decode the JPEG frame to an image
            array = np.frombuffer(jpg_frame, dtype=np.uint8)        
            image = cv2.imdecode(array, cv2.IMREAD_COLOR)

            #resized = cv2.resize(image, (224, 224))

            detection = detection_model(image)[0]

            # PLOT
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

            biggest = 0
            predicted_class = 0.0

            for data in detection.boxes.data.tolist():
                class_id = data[5]
                area = (int(data[2]) - int(data[0])) * (int(data[3]) - int(data[1]))

                if area > biggest: 
                    biggest = area
                    predicted_class = class_id + 1
                    best_box = {'xmin': int(data[0]), 'ymin': int(data[1]), 'xmax': int(data[2]), 'ymax': int(data[3])}

                
            if predicted_class != 0:
                streak.append(int(predicted_class))

            if predicted_class != 0 and len(streak) == 1:
                print(streak)
                detection_dict = {'class' : float(streak[0]), 
                'xmin': best_box['xmin'],
                'ymin': best_box['ymin'],
                'xmax': best_box['xmax'],
                'ymax': best_box['ymax'],
                'fast': 1
                }
                post_response = requests.post(url, data=detection_dict)


            if predicted_class != 0 and len(streak) == 10 and all(streak[i] == streak[0] for i in range(len(streak))):
                print(streak)
                detection_dict = {'class' : float(streak[0]), 
                'xmin': best_box['xmin'],
                'ymin': best_box['ymin'],
                'xmax': best_box['xmax'],
                'ymax': best_box['ymax'],
                'fast': 0
                }
                streak.clear()
                post_response = requests.post(url, data=detection_dict)

else:
    print(f"Failed to connect to the stream: {get_response.status_code}")

# Release resources
#cv2.destroyAllWindows()