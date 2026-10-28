// Copyright 2026 Aleksandr Kovalko
// Licensed under the Apache License, Version 2.0

#pragma once

// Arduino entry point. The Library Manager puts only src/ on the include
// path, so this shim forwards to the real headers in include/geo/. Cores
// that still compile as gnu++11 (AVR, SAMD, ESP32 core 2.x) would otherwise
// fail deep inside <optional>; fail here with one readable message instead.
#if __cplusplus < 201703L
#error "GeoUtils needs C++17: use ESP32 core 3.x, ESP8266 3.x, arduino-pico, STM32duino or UNO R4"
#endif

#include "../include/geo/geo.hpp"
