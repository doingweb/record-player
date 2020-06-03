#include <Arduino.h>
#include <CircularBuffer.h>

namespace logger {
  extern CircularBuffer<String, 100> logLines;

  void log(String message);
}
