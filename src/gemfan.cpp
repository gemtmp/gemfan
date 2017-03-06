
#include <iostream>
#include <signal.h>

#include "Sensor.hpp"

volatile bool isTerminated = false;
static void int_handler(int) {
  isTerminated = true;
}

int main() {
  signal(SIGINT, int_handler);

  System s;
  if (!s.scanACPI()) {
    std::cerr << "Can not detect temperature sensors and fan control" << std::endl;
    std::cerr << s;
    return 1;
  }
  std::cout << s;

  static const int period = 1;

  int counter = 0;
  while(!isTerminated) {
    try {
      auto state = s.run(period);

      std::cout << "Fan " << state.speed
          << " Temp " << state.temp.name() << " " << state.temperature
          << " " << state.temp.target() << std::endl;
    } catch (const System::CriticalTemp& ex) {
      s.disengage();
      std::cout << ex.what() << std::endl;
    }
    counter++;
    if (counter > 15) {
      std::cout << s;
      counter = 0;
    }
    sleep(period);
  }
  return 0;
}
