import io
import picamera
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from threading import Condition
import json
import serial
from time import sleep
import RPi.GPIO as GPIO

GPIO.setmode(GPIO.BOARD)
GPIO.setwarnings(False)
GPIO.setup(7, GPIO.OUT)
GPIO.output(7, GPIO.LOW)

'''
FrameBuffer is a synchronized buffer which gets each frame and notifies to all waiting clients.
It implements write() method to be used in picamera.start_recording()
'''
class FrameBuffer(object):
    def __init__(self):
        self.frame = None
        self.buffer = io.BytesIO()
        self.condition = Condition()

    def write(self, buf):
        if buf.startswith(b'\xff\xd8'):
            # New frame
            with self.condition:
                # Write to buffer
                self.buffer.seek(0)
                self.buffer.write(buf)
                # Crop buffer to exact size
                self.buffer.truncate()
                # Save the frame
                self.frame = self.buffer.getvalue()
                # Notify all other threads
                self.condition.notify_all()

'''
StreamingHandler extent http.server.SimpleHTTPRequestHandler class to handle mjpg file for live stream
'''
class StreamingHandler(SimpleHTTPRequestHandler):
    def __init__(self, frames_buffer, *args):
        self.frames_buffer = frames_buffer
        super().__init__(*args)

    def do_GET(self):
        if self.path == '/stream.mjpg':
            self.send_response(200)
            self.send_header('Age', 0)
            self.send_header('Cache-Control', 'no-cache, private')
            self.send_header('Pragma', 'no-cache')
            self.send_header('Content-Type', 'multipart/x-mixed-replace; boundary=FRAME')
            self.end_headers()
            try:
                # Run the endless stream
                while True:
                    with self.frames_buffer.condition:
                        # Wait for a new frame
                        self.frames_buffer.condition.wait()
                        # It's available, pick it up
                        frame = self.frames_buffer.frame
                        # Send it
                        self.wfile.write(b'--FRAME\r\n')
                        self.send_header('Content-Type', 'image/jpeg')
                        self.send_header('Content-Length', len(frame))
                        self.end_headers()
                        self.wfile.write(frame)
                        self.wfile.write(b'\r\n')
            except Exception as e:
                print(f'Removed streaming client {self.client_address}, {str(e)}')
        else:
            # Fallback to default handler
            super().do_GET()

    def do_POST(self):
        # Calls global variables
        global class_predicted, stop_condition, fast_stop
        # Finds length of client's data
        length = int(self.headers['Content-Length'])
        # Reads the data
        field_data = self.rfile.read(length).decode('utf-8')
        data = [int(float(i.split('=')[1])) for i in field_data.split('&')]
        fast_stop = int(data[1])

        # Saves the predicted class
        if data[0] != '0.0':
            class_predicted = int(float(data[0]))

        # Responds
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        response = {'status': 'success', 'stop' : stop_condition}
        self.wfile.write(json.dumps(response).encode('utf-8'))

'''
ThreadingServer extents ThreadingHTTPServer in order to manage the class prediction sent by the client
'''
class ThreadingServer(ThreadingHTTPServer):
    def __init__(self, serial, *args):
        self.serial = serial
        super().__init__(*args)

    def serve_forever(self):
        # Calls and initializes global variables
        global class_predicted, stop_condition, fast_stop
        class_predicted = 0
        fast_stop = 0

        while True:
            # Handles single request
            self.handle_request()
            sleep(0.01)

            # Checks if the paddle needs to be stopped
            if fast_stop == 1:
                # Send fast stop command
                self.serial.write(('9\n').encode())
                print('PADDLE BLOCKED')

            else:
                # Checks if new predictions should be received and a class is predicted
                if stop_condition == 0 and class_predicted != 0:
                    # Sends the class recognized
                    self.serial.write((str(class_predicted) + '\n').encode())
                    print('DATA SENT')
                    # Stops new predictions
                    stop_condition = 1

            sleep(0.01)

            if self.serial.in_waiting > 0:
                # Decodes the received data
                received_data = self.serial.readline().decode('utf-8').strip()

                # If the Arduino said so, start getting new predictions
                if received_data == "42": stop_condition = 0


'''
Function to handle streaming from a picamera while setting up a server for video trasmission.
It also manages serial USB communication with an Arduino
'''
def stream():
    # Calls and initializes global variables
    global class_predicted, stop_condition
    stop_condition = 0

    # Initialize serial communication with the Arduino
    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1.0) # controlla il timeout
    sleep(2)
    # Clear any leftover data in the input buffer
    ser.reset_input_buffer()

    # Check if the serial communication is open
    if ser.isOpen():
        # Initialize picamera
        with picamera.PiCamera(framerate=5) as camera:
            camera.resolution = (704, 512)
            # Create and stream buffer
            frame_buffer = FrameBuffer()
            camera.start_recording(frame_buffer, format='mjpeg')

            try:
                address = ('', 8000)
                # Define a handler to manage incoming streaming requests
                handler = lambda *args: StreamingHandler(frame_buffer, *args)

                # Start a multithreaded server to handle requests and serial communication
                server = ThreadingServer(ser, address, handler)
                # Keep the server running indefinitely
                server.serve_forever()

            finally:
                GPIO.output(4, GPIO.LOW)
                camera.stop_recording()

if __name__ == "__main__":
#    sleep(120)
    GPIO.output(7, GPIO.HIGH)
    stream()
