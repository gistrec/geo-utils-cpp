// Great-circle distance and initial heading between two points, printed once.
// Needs a C++17-capable core (ESP32 3.x, ESP8266 3.x, arduino-pico, STM32duino,
// UNO R4); older cores stop with a single #error from <GeoUtils.h>.
#include <GeoUtils.h>

void setup() {
    Serial.begin(115200);
    delay(1000);  // let USB-CDC boards enumerate before the first print

    const geo::LatLng newYork{40.7128, -74.0060};
    const geo::LatLng london{51.5074, -0.1278};

    Serial.print("Distance: ");
    Serial.print(geo::distance_between(newYork, london) / 1000.0);
    Serial.println(" km");
    Serial.print("Heading:  ");
    Serial.print(geo::heading(newYork, london));
    Serial.println(" deg");
}

void loop() {}
