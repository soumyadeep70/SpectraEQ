<div align="center">
  <h3>
    <img src="src/ui/assets/app_icon.png" alt="SpectraEQ Logo" height="50" align="middle" />
    <b>SpectraEQ</b>
  </h3>
</div>

<div align="center">
  <a href="https://github.com/soumyadeep70/SpectraEQ/actions">
    <img src="https://img.shields.io/github/actions/workflow/status/soumyadeep70/SpectraEQ/tests.yml?branch=main&label=tests&style=flat-square" alt="Test Status" />
  </a>&nbsp;&nbsp;
  <a href="https://github.com/soumyadeep70/SpectraEQ/blob/main/LICENSE">
    <img src="https://img.shields.io/github/license/soumyadeep70/SpectraEQ?style=flat-square" alt="License" />
  </a>&nbsp;&nbsp;
  <img src="https://img.shields.io/github/last-commit/soumyadeep70/SpectraEQ?style=flat-square" alt="Last Commit" />
</div>

SpectraEQ is a modern real-time audio spectrum analyzer and visualizer built with C++23, Qt 6 QML, and Miniaudio. It captures live system audio from selectable input devices, performs high-performance FFT analysis using a custom optimized FFT implementation, and renders a responsive frequency spectrum with logarithmic frequency scaling and perceptual dB mapping.

---

# Screenshots

<div align="center">
  <img src="src/ui/assets/app_screenshot.png" alt="SpectraEQ Dashboard" width="800" />
</div>

---

# Future Work/ Improvements

- Add sample rate in the ui
- Fix the bar overflows in ui
- Check the return values of miniaudio functions and log them
- Add SIMD optimized versions of the FFT