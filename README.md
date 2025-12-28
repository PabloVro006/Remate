# Remate - Don't waste your time 🌍♻️
Remate (Robotic Ecological Multi-waste Automatic Trashcan) is an innovative smart sorting system designed to revolutionize urban waste management. Unlike existing solutions that process one item at a time, Remate is capable of identifying and sorting multiple pieces of waste simultaneously.

### 🏆 Achievements & Recognition
* 1st place at the Italian Robotics Championship 2024.
* Gold Medal (5th place out of 48 teams) at the World Robot Olympiad Gloabl Final 2024, in the Future Innovators Senior category

### 🚀 Key Innovations
The core of Remate lies in its mechanical division system, which allows it to accept an entire bag of waste and sort it autonomously.
* **Efficiency**: While competitors take several seconds per item, Remate optimizes flow through mechanical separation and high-speed AI inference
* **Multi-Object Recognition**: Powered by an AI model based on Object Detection (trained on a custom dataset), the system identifies different materials like plastics and metals in a single frame.

### 🛠️ System Architecture
The project integrates hardware and software components:
**Hardware**
* Main Controllers: Raspberry Pi 4 and Arduino Nano Every (Motor Control).
* Vision: PiCamera for real-time image acquisition.
* Mechanics: DC Motors (12 and 20 RPM), Hall effect sensors for precise positioning, and magnets.
* Power: 12V lead-acid battery and 5V power bank.
* AI Server: An external computer that acts as a server and makes the Object Detection inference in real time.

**Software**
* AI Server (Python): Handles image processing and waste classification via HTTP requests.
* Raspberry Pi Script (Python): Coordinates the camera and communicates with the Arduino via Serial.
* Arduino Firmware (C++): Manages the movement of the rotating paddle, sorting disk, and separators.

### 👥 The Team
* Paolo Vairo: Hardware Engineering and Arduino Firmware Development.
* Giulio Gismondi: Project Manager, AI Development, and Python scripting.
* Andrea Tarasca: Mechanical Design, 3D Modeling, and Communications.
