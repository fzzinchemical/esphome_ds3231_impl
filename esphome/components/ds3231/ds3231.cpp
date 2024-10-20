// Used the Code from the DS1307 component as a base for the DS3231 component.
// This works since both have a singular difference when reading the time from the RTC, the CH-Bit.
// Flags differ, and more functions are available in the DS3231 component, thus need to be implemented instead.

// TODO Add the rest of the DS3231 functions.
// TODO EOSC needs to be read to determine if the Clock is running or not. Otherwise does NOT run.

#include "ds3231.h"
#include "esphome/core/log.h"

// Datasheet:
// - https://datasheets.maximintegrated.com/en/ds/DS3231.pdf

namespace esphome {
namespace ds3231 {

static const char *const TAG = "ds3231";

void DS3231Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up DS3231...");
  time::RealTimeClock::set_timezone(this->timezone_);

  // initializing unused bits
  ds3231_.special.a1le = 0;
  ds3231_.special.a2le = 0;
  ds3231_.special.intcn = 1;
  ds3231_.special.rs1 = 0;
  ds3231_.special.rs2 = 0;
  ds3231_.special.conv = 0;
  ds3231_.special.bbsqw = 0;
  ds3231_.special.eosc = 0;

  if (!this->setup_()) {
    this->mark_failed();
  }
  if (!this->read_rtc_()) {
    this->mark_failed();
  }
}

void DS3231Component::update() { this->read_time(); }

void DS3231Component::dump_config() {
  ESP_LOGCONFIG(TAG, "DS3231:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with DS3231 failed!");
  }
  ESP_LOGCONFIG(TAG, "  Timezone: '%s'", this->timezone_.c_str());
}

float DS3231Component::get_setup_priority() const { return setup_priority::DATA; }

void DS3231Component::read_time() {
  if (!this->read_rtc_()) {
    return;
  }
  if (ds3231_.special.eosc) {
    ESP_LOGW(TAG, "RTC halted, not syncing to system clock. Please set EOSC to 0.");
    return;
  }
  ESPTime rtc_time{
      .second = uint8_t(ds3231_.rtc.second + 10 * ds3231_.rtc.second_10),
      .minute = uint8_t(ds3231_.rtc.minute + 10u * ds3231_.rtc.minute_10),
      .hour = uint8_t(ds3231_.rtc.hour + 10u * ds3231_.rtc.hour_10),
      .day_of_week = uint8_t(ds3231_.rtc.weekday),
      .day_of_month = uint8_t(ds3231_.rtc.day + 10u * ds3231_.rtc.day_10),
      .day_of_year = 1,  // ignored by recalc_timestamp_utc(false)
      .month = uint8_t(ds3231_.rtc.month + 10u * ds3231_.rtc.month_10),
      .year = uint16_t(ds3231_.rtc.year + 10u * ds3231_.rtc.year_10 + 2000),
      .is_dst = false,  // not used
      .timestamp = 0    // overwritten by recalc_timestamp_utc(false)
  };
}

void DS3231Component::write_time() {
  auto now = time::RealTimeClock::now();
  ;  // Wieso wird hier die UTC Zeit verwendet und nicht die durch eine Eingabe?
  if (!now.is_valid()) {
    ESP_LOGE(TAG, "Invalid system time, not syncing to RTC.");
    return;
  }
  ds3231_.rtc.year = (now.year - 2000) % 10;
  ds3231_.rtc.year_10 = ((now.year - 2000) / 10) % 10;
  ds3231_.rtc.century = 0;  // 0 for 2000s

  ds3231_.rtc.month = now.month % 10;
  ds3231_.rtc.month_10 = now.month / 10;

  ds3231_.rtc.day = now.day_of_month % 10;
  ds3231_.rtc.day_10 = now.day_of_month / 10;

  ds3231_.rtc.weekday = now.day_of_week;

  ds3231_.rtc.hour = now.hour % 10;
  ds3231_.rtc.hour_10 = now.hour / 10;
  ds3231_.rtc.hour_format_12_24 = 1;  // 24-hour format
  ds3231_.rtc.minute = now.minute % 10;
  ds3231_.rtc.minute_10 = now.minute / 10;
  ds3231_.rtc.second = now.second % 10;
  ds3231_.rtc.second_10 = now.second / 10;

  // initializing unused bits to be safe
  ds3231_.rtc.unused_0 = 0;
  ds3231_.rtc.unused_1 = 0;
  ds3231_.rtc.unused_2 = 0;
  ds3231_.rtc.unused_3 = 0;
  ds3231_.rtc.unused_4 = 0;
  ds3231_.rtc.unused_5 = 0;

  this->write_rtc_();
}

void DS3231Component::read_temperature() {
  if (!this->read_temperature_()) {
    ESP_LOGE(TAG, "Can't read temperature.");
    return;
  }
  struct {
    int8_t integer;
    uint8_t decimal;
  } temperature;
  temperature.integer = ds3231_.raw_temperature[0];
  temperature.decimal = (ds3231_.raw_temperature[1] >> 6) * 25;
  ESP_LOGD(TAG, "Temperature: %d.%d °C", temperature.integer, temperature.decimal);
}

bool DS3231Component::setup_() {
  if (!this->write_bytes(0x0E, this->ds3231_.raw_special, sizeof(this->ds3231_.raw_special))) {
    ESP_LOGE(TAG, "Can't write I2C data.");
    return false;
  }
  return true;
}

// TODO implement eosc read
bool DS3231Component::read_rtc_() {
  if (!this->read_bytes(0, this->ds3231_.raw_rtc, sizeof(this->ds3231_.raw_rtc))) {
    ESP_LOGE(TAG, "Can't read I2C data.");
    return false;
  }
  // TODO: Replace the DS1307 flags with those used in the DS3231. And replace the ds1307_ with ds3231_.
  //  "Read %0u%0u:%0u%0u:%0u%0u 20%0u%0u-%0u%0u-%0u%0u  CH:%s RS:%0u SQWE:%s OUT:%s",
  ESP_LOGD(TAG, "Read  %0u%0u:%0u%0u:%0u%0u 20%0u%0u-%0u%0u-%0u%0u", ds3231_.rtc.hour_10, ds3231_.rtc.hour,
           ds3231_.rtc.minute_10, ds3231_.rtc.minute, ds3231_.rtc.second_10, ds3231_.rtc.second, ds3231_.rtc.year_10,
           ds3231_.rtc.year, ds3231_.rtc.month_10, ds3231_.rtc.month, ds3231_.rtc.day_10,
           ds3231_.rtc.day /*, ONOFF(ds1307_.ch), ds1307_.rs, ONOFF(ds1307_.sqwe), ONOFF(ds1307_.out)*/);

  return true;
}

bool DS3231Component::write_rtc_() {
  ESP_LOGD(TAG, "Size of raw_rtc: %d", sizeof(this->ds3231_.raw_rtc));
  if (!this->write_bytes(0x0, this->ds3231_.raw_rtc, sizeof(this->ds3231_.raw_rtc))) {
    ESP_LOGE(TAG, "Can't write I2C data.");
    return false;
    `
  }
  // TODO: Replace the DS1307 flags with those used in the DS3231.
  ESP_LOGD(TAG, "Write %0u%0u:%0u%0u:%0u%0u 20%0u%0u-%0u%0u-%0u%0u  CH:%s RS:%0u SQWE:%s OUT:%s", ds3231_.rtc.hour_10,
           ds3231_.rtc.hour, ds3231_.rtc.minute_10, ds3231_.rtc.minute, ds3231_.rtc.second_10, ds3231_.rtc.second,
           ds3231_.rtc.year_10, ds3231_.rtc.year, ds3231_.rtc.month_10, ds3231_.rtc.month, ds3231_.rtc.day_10,
           ds3231_.rtc.day /*, ONOFF(ds3231_.ch), ds3231_.rs, ONOFF(ds3231_.sqwe), ONOFF(ds3231_.out)*/);
  return true;
}

bool DS3231Component::read_temperature_() {
  if (!this->read_bytes(0x11, this->ds3231_.raw_temperature, sizeof(this->ds3231_.raw_temperature))) {
    ESP_LOGE(TAG, "Can't read I2C data, when reading temperature.");
    return false;
  }
  return true;
}

bool DS3231Component::write_alarm_(bool alarm_select) {
  uint8_t alarm_address = alarm_select ? 0x0B : 0x07;  // 0x07 for alarm 1 (false), 0x0B for alarm 2 (true)
  uint8_t *alarm_raw = alarm_select ? this->ds3231_.raw_alarm2 : this->ds3231_.raw_alarm1;

  if (!this->write_bytes(alarm_address, alarm_raw, sizeof(alarm_raw))) {
    ESP_LOGE(TAG, "Can't write I2C data.");
    return false;
  }
  // TODO: Replace the DS1307 flags with those used in the DS3231.
  ESP_LOGD(TAG, "Write %0u%0u:%0u%0u:%0u%0u 20%0u%0u-%0u%0u-%0u%0u", ds3231_.rtc.hour_10, ds3231_.rtc.hour,
           ds3231_.rtc.minute_10, ds3231_.rtc.minute, ds3231_.rtc.second_10, ds3231_.rtc.second, ds3231_.rtc.year_10,
           ds3231_.rtc.year, ds3231_.rtc.month_10, ds3231_.rtc.month, ds3231_.rtc.day_10, ds3231_.rtc.day);
  return true;
}

}  // namespace ds3231
}  // namespace esphome
