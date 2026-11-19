
import cv2
import mediapipe as mp

# Настройки камеры
CAMERA_ID = 0
FRAME_WIDTH = 1280
FRAME_HEIGHT = 720

# Инициализация MediaPipe
mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils

hands = mp_hands.Hands(
    static_image_mode=False,
    max_num_hands=2,
    model_complexity=1,
    min_detection_confidence=0.6,
    min_tracking_confidence=0.5
)

# Открываем камеру
cap = cv2.VideoCapture(CAMERA_ID, cv2.CAP_DSHOW)

cap.set(cv2.CAP_PROP_FRAME_WIDTH, FRAME_WIDTH)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT)

if not cap.isOpened():
    print("ERROR: Cannot open camera")
    raise SystemExit(1)

print("Camera started.")
print("Press Q to exit.")

while True:
    ret, frame = cap.read()

    if not ret:
        print("ERROR: Cannot read frame")
        break

    # Зеркальное отображение для удобства
    frame = cv2.flip(frame, 1)

    frame_height, frame_width = frame.shape[:2]

    # MediaPipe ожидает RGB
    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

    results = hands.process(rgb)

    detections = []

    if results.multi_hand_landmarks:
        for hand_landmarks in results.multi_hand_landmarks:

            # Рисуем скелет руки
            mp_draw.draw_landmarks(
                frame,
                hand_landmarks,
                mp_hands.HAND_CONNECTIONS
            )

            # Получаем координаты всех точек
            xs = [
                int(point.x * frame_width)
                for point in hand_landmarks.landmark
            ]

            ys = [
                int(point.y * frame_height)
                for point in hand_landmarks.landmark
            ]

            # Bounding box
            x_min = max(0, min(xs))
            y_min = max(0, min(ys))
            x_max = min(frame_width - 1, max(xs))
            y_max = min(frame_height - 1, max(ys))

            width = x_max - x_min
            height = y_max - y_min

            if width <= 0 or height <= 0:
                continue

            bbox = {
                "x": x_min,
                "y": y_min,
                "width": width,
                "height": height
            }

            detections.append(bbox)

            # Рисуем рамку
            cv2.rectangle(
                frame,
                (x_min, y_min),
                (x_max, y_max),
                (0, 255, 0),
                2
            )

            # Нижняя точка рамки
            bottom_point = (x_min + width // 2, y_max)

            cv2.circle(
                frame,
                bottom_point,
                7,
                (0, 0, 255),
                -1
            )

            cv2.putText(
                frame,
                f"Hand: {x_min},{y_min},{width},{height}",
                (x_min, max(25, y_min - 10)),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.55,
                (0, 255, 0),
                2
            )

    # Границы трех вертикальных секторов
    for i in range(1, 3):
        x = int(frame_width * i / 3)

        cv2.line(
            frame,
            (x, 0),
            (x, frame_height),
            (255, 255, 0),
            2
        )

    # Полоса зоны распыления: 40-60% высоты
    zone_top = int(frame_height * 0.40)
    zone_bottom = int(frame_height * 0.60)

    cv2.line(
        frame,
        (0, zone_top),
        (frame_width, zone_top),
        (0, 165, 255),
        2
    )

    cv2.line(
        frame,
        (0, zone_bottom),
        (frame_width, zone_bottom),
        (0, 165, 255),
        2
    )

    cv2.putText(
        frame,
        f"Frame: {frame_width}x{frame_height}",
        (10, 30),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.7,
        (255, 255, 255),
        2
    )

    cv2.putText(
        frame,
        f"Detected hands: {len(detections)}",
        (10, 60),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.7,
        (255, 255, 255),
        2
    )

    cv2.imshow("Hand Tracking - Weed Box Parser", frame)

    # Q — выход
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

cap.release()
hands.close()
cv2.destroyAllWindows()

print("Camera stopped.")
