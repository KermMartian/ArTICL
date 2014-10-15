#include <TILP.h>

TILP tilp = TILP(3, 2);

void setup() {
  pinMode(4, INPUT_PULLUP);
  Serial.begin(9600);
  tilp.resetLines();
  delay(1000);
}

void loop() {
  if (!digitalRead(4)) {
    Serial.println("Starting transfer...");
    uint8_t screen[768+2];
    uint8_t msg[4] = {0x73, 0x6D, 0x00, 0x00};
    tilp.send(msg, NULL, 0);
    Serial.println("Sent request");
    int rlen = 0;
    tilp.get(msg, NULL, &rlen, 0);
    Serial.println("Recieved acknowledgement");
    tilp.get(NULL, screen, &rlen, 768+2);
    Serial.println("Recieved screenshot");
    tilp.send(msg, NULL, 0);
    Serial.println("Sent acknowledgement");
    for (int i = 1; i <= 768; i++) {
      for (int j = 7; j > -1; j--) {
        if (screen[i+3] & (1 << j)) {
          Serial.write('#');
        } else {
          Serial.write('.');
        }
      }
      if (i%(12) == 0) {
        Serial.println();
      }
    }
  }
}