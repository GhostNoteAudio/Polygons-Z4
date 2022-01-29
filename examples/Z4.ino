#include "Z4.h"

void setup()
{
    Polygons::init();
    Z4::start();
}

int i = 0;
void loop()
{
    delay(50);
    if (i++ % 100 == 0)
        Serial.println(Timers::GetCpuLoad() * 100);
    Z4::loop();
}