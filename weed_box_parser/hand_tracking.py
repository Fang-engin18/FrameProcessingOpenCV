import socket

import cv2
import mediapipe as mp

CAMERA_ID = 0
FRAME_WIDTH = 1280
FRAME_HEIGHT = 720
SERVER = ("127.0.0.1", 5005)

# Camera + hand detection only. ALL sector/zone decisions are made by C++.
mp_hands = mp.solutions.hands
hands = mp_hands.Hands(
    static_image_mode=False,
    max_num_hands=2,
    model_complexity=1,
    min_detection_confidence=0.6,
    min_tracking_confidence=0.5,
)

cap = cv2.VideoCapture(CAMERA_ID, cv2.CAP_DSHOW)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, FRAME_WIDTH)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT)
if not cap.isOpened():
    print("ERROR: Cannot open camera")
    hands.close()
    raise SystemExit(1)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.settimeout(0.05)
print("Camera started. Start go.bat in a second terminal. Press Q to exit.")

try:
    while True:
        ok, frame = cap.read()
        if not ok:
            print("ERROR: Camera frame not available")
            break
        frame = cv2.flip(frame, 1)
        height, width = frame.shape[:2]
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = hands.process(rgb)

        boxes = []
        if results.multi_hand_landmarks:
            for landmarks in results.multi_hand_landmarks:
                xs = [int(p.x * width) for p in landmarks.landmark]
                ys = [int(p.y * height) for p in landmarks.landmark]
                x1 = max(0, min(xs))
                y1 = max(0, min(ys))
                x2 = min(width - 1, max(xs))
                y2 = min(height - 1, max(ys))
                if x2 > x1 and y2 > y1:
                    boxes.append((x1, y1, x2 - x1, y2 - y1))

        # One UDP datagram contains the complete snapshot of this frame,
        # including count=0 when no hands are visible.
        parts = ["FRAME", str(width), str(height), str(len(boxes))]
        for x, y, w, h in boxes:
            parts += [str(x), str(y), str(w), str(h)]

        state = None
        try:
            sock.sendto(" ".join(parts).encode("ascii"), SERVER)
            data, _ = sock.recvfrom(1024)
            fields = data.decode("ascii").split()
            if len(fields) == 6 and fields[0] == "STATE":
                count = int(fields[1])
                top = float(fields[2])
                bottom = float(fields[3])
                overlap = float(fields[4])
                mask = int(fields[5])
                if 1 <= count <= 16 and 0 <= mask < (1 << count):
                    state = (count, top, bottom, overlap, mask)
        except (socket.timeout, OSError, ValueError):
            pass

        # Only detection boxes are created in Python.
        for x, y, w, h in boxes:
            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)
            cv2.putText(frame, f"Hand bbox: {x},{y},{w},{h}",
                        (x, max(20, y - 8)), cv2.FONT_HERSHEY_SIMPLEX,
                        0.5, (0, 255, 0), 2)

        # Read-only visualization of parameters and mask RECEIVED from C++.
        if state is not None:
            count, top, bottom, overlap, mask = state
            for index in range(1, count):
                xx = round(width * index / count)
                cv2.line(frame, (xx, 0), (xx, height - 1), (255, 255, 0), 2)
            for ratio in (top, bottom):
                yy = round(height * ratio / 100)
                cv2.line(frame, (0, yy), (width - 1, yy), (0, 165, 255), 2)
            for index in range(count):
                if mask & (1 << index):
                    xx0 = round(width * index / count)
                    xx1 = round(width * (index + 1) / count)
                    overlay = frame.copy()
                    cv2.rectangle(overlay, (xx0, 0), (xx1, height), (0, 220, 0), -1)
                    cv2.addWeighted(overlay, 0.13, frame, 0.87, 0, frame)
            binary = format(mask, f"0{count}b")
            status = f"C++ ONLINE | mask={binary} ({mask})"
            info = f"sectors={count}, area>={overlap:g}%, zone={top:g}-{bottom:g}%"
            color = (0, 255, 0)
        else:
            status = "C++ OFFLINE | mask UNKNOWN (no actuation)"
            info = "Start go.bat to receive sectors and spray zone"
            color = (0, 0, 255)

        cv2.rectangle(frame, (0, 0), (min(width, 750), 83), (25, 25, 25), -1)
        cv2.putText(frame, status, (10, 29), cv2.FONT_HERSHEY_SIMPLEX,
                    0.7, color, 2)
        cv2.putText(frame, info, (10, 57), cv2.FONT_HERSHEY_SIMPLEX,
                    0.53, (255, 255, 255), 1)
        cv2.putText(frame, f"Detected hands: {len(boxes)}", (10, 78),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)

        cv2.imshow("Python detector + C++ sector controller", frame)
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break
finally:
    cap.release()
    hands.close()
    sock.close()
    cv2.destroyAllWindows()
