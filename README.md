### 📦 `CameraToROSPublisher` for UE5 → ROS Image Streaming

This Unreal Engine 5 C++ component captures images from a `SceneCaptureComponent2D` camera and sends them over a TCP socket to a ROS-compatible receiver (like a Python node) using the `sensor_msgs/msg/Image` layout (`bgr8` format).

---

### 🛠️ Setup Instructions

#### 1. **Add Component to Your Actor**

* Attach `UCameraToROSPublisher` to an Actor (like a vehicle).
* Assign in the Details Panel:

  * `CaptureComponent`: Your `SceneCaptureComponent2D`
  * `RenderTarget`: The `TextureRenderTarget2D` assigned to your capture component

#### 2. **Configure ROS Connection**

* Set the following in the Details Panel:

  * `RosIPAddress`: IP of the ROS machine (e.g. `192.168.137.53`)
  * `RosPort`: Port (e.g. `9999`)
  * `SendRate`: Frame rate (images/sec) to stream

#### 3. **Build the Project**

* Build with Visual Studio 2022 after adding the component.
* Make sure sockets and async tasks are enabled.

#### 4. **Launch Unreal Simulation**

* When the simulation runs:

  * Images will be captured at the defined rate.
  * The component sends formatted binary data over TCP.

---

### 📤 Data Format (matches `struct.pack('!I', ...)` in Python)

Each image is sent in the following serialized binary format:

| Field       | Size      | Description                    |
| ----------- | --------- | ------------------------------ |
| `Width`     | 4 bytes   | Image width                    |
| `Height`    | 4 bytes   | Image height                   |
| `EncLen`    | 4 bytes   | Length of encoding string      |
| `Encoding`  | `EncLen`  | Image encoding (e.g. `"bgr8"`) |
| `DataLen`   | 4 bytes   | Total size of raw pixel data   |
| `PixelData` | `DataLen` | Raw image in BGR format        |

---

### 🧪 Test Client (Python)

```python
import socket
import struct
import numpy as np
import cv2

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind(('0.0.0.0', 9999))
sock.listen(1)
conn, addr = sock.accept()

while True:
    def recv_all(count):
        buf = b''
        while count:
            newbuf = conn.recv(count)
            if not newbuf: return None
            buf += newbuf
            count -= len(newbuf)
        return buf

    width = struct.unpack('!I', recv_all(4))[0]
    height = struct.unpack('!I', recv_all(4))[0]
    enc_len = struct.unpack('!I', recv_all(4))[0]
    encoding = recv_all(enc_len).decode()
    data_len = struct.unpack('!I', recv_all(4))[0]
    img_data = recv_all(data_len)

    img = np.frombuffer(img_data, dtype=np.uint8).reshape((height, width, 3))
    cv2.imshow("ROS Image", img)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break
```

---

### ✅ Notes

* `bgr8` encoding is used by default to match OpenCV expectations.
* Image sending is done on a **separate thread** to avoid blocking the game thread.
* UE5 logging will display each image sent, with its size and byte count.

