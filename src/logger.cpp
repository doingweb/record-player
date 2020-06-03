#include "logger.h"

namespace logger {
  CircularBuffer<String, 100> logLines;

  void log(String message) {
    String logMessage = "[" + String(time(nullptr), DEC) + "] " + message;
    Serial.println(logMessage);
    Serial.flush();
    logLines.push(logMessage);
  }
}
