import io
import picamera
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from threading import Condition
import json
import serial
from time import sleep

"""
FrameBuffer is a synchronized buffer which gets each frame and notifies to all waiting clients.
It implements write() method to be used in picamera.start_recording()
"""
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

"""
StreamingHandler extent http.server.SimpleHTTPRequestHandler class to handle mjpg file for live stream
"""
class StreamingHandler(SimpleHTTPRequestHandler):
    def __init__(self, frames_buffer, serial, *args):
        self.frames_buffer = frames_buffer
        self.serial = serial
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
                # Endless stream
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
        global class_predicted
        # Finds length of client's data
        length = int(self.headers['Content-Length'])
        # Reads the data
        field_data = self.rfile.read(length)
        
        # Extracts the class from the JSON sent by POST request
        self.received = json.loads(field_data)
        print('Dati ricevuti dal client: ', self.received)
        data = self.received[11:14]
        # data = self.received.get('predicted_class', '0.0')
        
        # Saves the predicted class
        if data != '0.0':
            class_predicted = int(float(data))
            print('CLASS PREDICTED: ' + str(class_predicted))
            self.serial.write((str(class_predicted) + '\n').encode())

            # this clears the buffer (?)
            # read_value = self.serial.read(self.serial.in_waiting).decode('ascii')
            while True:
                print('ENTERED WHILE TRUE')
                if self.serial.in_waiting > 0:
                    print('ENTERED CONDITION')
                    received_data = self.serial.readline().decode('utf-8').strip()
                    print(f"Received: {received_data}")
                    if received_data == "42": break

                '''
                if self.serial.in_waiting > 0:
                    print('ENTERED CONDITION')
                    read_value = self.serial.read(self.serial.in_waiting).decode('ascii')
                if read_value == "42": break
                '''

        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        response = {'status': 'success'}
        self.wfile.write(json.dumps(response).encode('utf-8'))
        

    

''' 
ThreadingServer extents ThreadingHTTPServer in order to manage the class prediction sent by the client
'''
class ThreadingServer(ThreadingHTTPServer):
    def __init__(self, *args):
        super().__init__(*args)

    def serve_forever(self):
        global class_predicted
        while True:
            self.handle_request()
            print('class' + str(class_predicted))
                
                

def stream():
    global class_predicted
    class_predicted = 0
    ser = serial.Serial('/dev/ttyACM1', 115200, timeout=1.0)
    sleep(2)
    ser.reset_input_buffer()
    
    if ser.isOpen():
        with picamera.PiCamera(framerate=20) as camera:
            camera.resolution = (512, 512)

            # Create and stream buffer
            frame_buffer = FrameBuffer()
            camera.start_recording(frame_buffer, format='mjpeg')

            # Run server
            try:
                address = ('', 8000)
                handler = lambda *args: StreamingHandler(frame_buffer, ser, *args)

                server = ThreadingServer(address, handler)
                server.serve_forever()
                
            finally:
                camera.stop_recording()

if __name__ == "__main__":
    stream()