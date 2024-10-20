#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace ds3231 {

class DS3231Component : public time::RealTimeClock, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override;

  void read_time();
  void write_time();

  void write_alarm();

  void read_temperature();

  // void read_alarm_1();
  // void write_alarm_2();
  // void read_alarm_2();

  // void write_square_wave_output_frequency();
  // void read_square_wave_output_frequency();

  // void enable_32khz_output();

 protected:
  bool setup_();
  bool read_rtc_();
  bool write_rtc_();

  bool read_alarm_(bool alarm_select);  // false for alarm 1, true for alarm 2
  bool write_alarm_(bool alarm_select);

  bool read_temperature_();

  union DS3231Reg {
    struct RTC {
      // 00h (Seconds)
      uint8_t second : 4;
      uint8_t second_10 : 3;
      bool unused_0 : 1;

      // 01h (Minutes)
      uint8_t minute : 4;
      uint8_t minute_10 : 3;
      bool unused_1 : 1;

      // 02h (Hours)
      uint8_t hour : 4;
      uint8_t hour_10 : 1;
      uint8_t hour_20 : 1;
      uint8_t hour_format_12_24 : 1;  // 0: 24-hour, 1: 12-hour format
      bool unused_2 : 1;

      // 03h (Day)
      uint8_t weekday : 3;
      uint8_t unused_3 : 5;

      // 04h (Date)
      uint8_t day : 4;
      uint8_t day_10 : 2;
      uint8_t unused_4 : 2;

      // 05h (Month/Century)
      uint8_t month : 4;
      bool month_10 : 1;
      uint8_t unused_5 : 2;
      bool century : 1;

      // 06h (Year)
      uint8_t year : 4;
      uint8_t year_10 : 4;
    } rtc;

    // Alarm Registers 0x7 and 0xB IF ALARM2 is used, there will be no seconds!

    struct Alarm {
      union {
        struct {
          // 07h (Alarm 1 Seconds)
          uint8_t a_second : 4;
          uint8_t a_second_10 : 3;
          bool a_m1 : 1;

          // 08h (Alarm 1 Minutes)
          uint8_t a_minute : 4;
          uint8_t a_minute_10 : 3;
          bool a_m2 : 1;

          // 09h (Alarm 1 Hours)
          uint8_t a_hour : 4;
          uint8_t a_hour_10 : 1;
          bool a_hour_20 : 1;
          uint8_t a_hour_format_12_24 : 1;
          bool a_m3 : 1;

          // 0Ah (Alarm 1 Day/Date)
          uint8_t a_date : 4;
          uint8_t a_date_10 : 2;
          bool a_dy_dt : 1;
          bool a_m4 : 1;
        } alarm1;
        struct {
          // 08h (Alarm 1 Minutes)
          uint8_t a_minute : 4;
          uint8_t a_minute_10 : 3;
          bool a_m2 : 1;

          // 09h (Alarm 1 Hours)
          uint8_t a_hour : 4;
          uint8_t a_hour_10 : 1;
          bool a_hour_20 : 1;
          uint8_t a_hour_format_12_24 : 1;
          bool a_m3 : 1;

          // 0Ah (Alarm 1 Day/Date)
          uint8_t a_date : 4;
          uint8_t a_date_10 : 2;
          bool a_dy_dt : 1;
          bool a_m4 : 1;
        } alarm2;
      };
    };

    // Special-Purpose Registers
    // 0Eh (Control Register)
    struct Special {
      bool a1le : 1;
      bool a2le : 1;
      bool intcn : 1;
      bool rs1 : 1;
      bool rs2 : 1;
      bool conv : 1;
      bool bbsqw : 1;
      bool eosc : 1;

      // 0Fh (Status Register)
      bool a1f : 1;
      bool a2f : 1;
      bool bsy : 1;
      bool en32khz : 1;
      uint8_t unused_6 : 3;
      bool osf : 1;
    } special;

    // 10h (Aging Offset)
    int8_t aging_offset;

    struct Temperature {
      // 11h (Temperature MSB)
      int8_t temperature_msb;

      // 12h (Temperature LSB)
      uint8_t unused_7 : 6;
      uint8_t temperature_lsb : 2;
    } temperature;
    mutable uint8_t raw_rtc[sizeof(rtc)];
    mutable uint8_t raw_alarm1[sizeof(Alarm::alarm1)];
    mutable uint8_t raw_alarm2[sizeof(Alarm::alarm2)];
    mutable uint8_t raw_special[sizeof(special)];
    mutable uint8_t raw_temperature[sizeof(temperature)];
  } ds3231_;
};
template<typename... Ts> class WriteAction : public Action<Ts...>, public Parented<DS3231Component> {
 public:
  void play(Ts... x) override { this->parent_->write_time(); }
};

template<typename... Ts> class ReadAction : public Action<Ts...>, public Parented<DS3231Component> {
 public:
  void play(Ts... x) override { this->parent_->read_time(); }
};
}  // namespace ds3231
}  // namespace esphome