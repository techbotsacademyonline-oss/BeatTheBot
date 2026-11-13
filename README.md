# BeatTheBot

A browser-based ping-pong game prototype that combines:

- Human paddle control via webcam + MediaPipe hand tracking
- AI paddle control via TensorFlow.js prediction of ball trajectory
- Optional ESP32 Wi-Fi arm output for a 4-DOF robotic arm using normalized movement data

## How to run

1. Open `index.html` in a browser.
2. Click `Start Camera & Game`.
3. Allow webcam access.
4. Move your hand in front of the camera to control the left paddle.
5. Optionally connect an ESP32 by entering its URL (for example `http://192.168.1.50`) and clicking `Connect to ESP32`.

## Notes

- The AI uses TensorFlow.js and is trained on synthetic ball trajectories in the browser.
- The ESP32 side is expected to expose a health endpoint at `/health` and an arm control endpoint at `/arm` that accepts JSON payloads. Example payload:

```json
{
  "paddleY": 52.5,
  "ballX": 41.1,
  "ballY": 65.2,
  "targetAngle": 120,
  "timestamp": 1731457900000
}
```

- The game is a prototype and is designed for demo purposes and local testing.

## ESP32 firmware idea

Your ESP32 firmware should:

- listen on Wi-Fi and serve HTTP endpoints or WebSocket messages
- accept a normalized arm target (base/shoulder/elbow/wrist values or equivalent)
- convert those values into servo or motor commands for a 4-DOF arm

This client sends the current AI target and ball prediction so the robot can mirror the right-side gameplay decision.
