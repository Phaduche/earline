# Third-party notices

## Dorso (formerly Posturr)

Earline's forward-head rule is adapted from the camera posture detector in Dorso
(https://github.com/tldev/dorso, `Sources/Detectors/CameraPostureDetector.swift`):
a baseline taken from upright calibration samples, a dead zone around it, a
forward-head check based on face size growing past its upright size (+5% starts,
+15% more is full severity), a slouch check based on the nose dropping, hysteresis,
and progressive on-screen nudges (glow / blur). Earline re-implements these ideas in
JavaScript and combines them with its own personal model; no Swift code is copied.

```
MIT License

Copyright (c) 2025 Posturr Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## MediaPipe Tasks Vision

Face and pose landmarks come from Google MediaPipe (`@mediapipe/tasks-vision`),
Apache License 2.0, loaded at runtime from jsDelivr and Google Cloud Storage.
