#include "uptime_text_sensor.h"

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace uptime {

static const char *const TAG = "uptime.sensor";

void UptimeTextSensor::setup() {
  this->last_ms_ = millis();
  if (this->last_ms_ < 60 * 1000)
    this->last_ms_ = 0;
  this->update();
}

void UptimeTextSensor::update() {
  auto now = millis();
  uint32_t delta = now - this->last_ms_;
  this->last_ms_ = now - delta % 1000;  // save remainder for next update
  delta /= 1000;
  this->uptime_ += delta;

  auto uptime = this->uptime_;
  unsigned interval = this->get_update_interval() / 1000;

  unsigned seconds = uptime % 60;
  uptime /= 60;
  unsigned minutes = uptime % 60;
  uptime /= 60;
  unsigned hours = uptime % 24;
  uptime /= 24;
  unsigned days = uptime;

  // Format string as "0g, 00h 01m e 48s"
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%ug, %02uh %02um e %02us", days, hours, minutes, seconds);

  this->publish_state(std::string(buffer));
}

float UptimeTextSensor::get_setup_priority() const { return setup_priority::HARDWARE; }
void UptimeTextSensor::dump_config() { LOG_TEXT_SENSOR("", "Uptime Text Sensor", this); }

}  // namespace uptime
}  // namespace esphome
