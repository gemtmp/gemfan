/*
 * Sensor.hpp
 *
 *  Created on: May 31, 2012
 *      Author: gem
 */

#ifndef SENSOR_HPP_
#define SENSOR_HPP_

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>

#include <boost/scoped_ptr.hpp>

#include "PID.hpp"

template <typename T>
class FsEntry {
public:
  typedef T value_t;

  FsEntry(const std::string& path, const std::string& name)
    : path_(path), name_(name) {}

  const std::string& path() const { return path_; }
  const std::string& name() const { return name_; }

  value_t get() const {
    std::ifstream f(path_.c_str(), std::ios_base::in);
    f.exceptions (std::ios_base::badbit|std::ios_base::failbit);
    value_t value;
    try {
	    f >> value;
    } catch(...) {
    }
    return value;
  }
  void set(const value_t& val) const {
    std::ofstream f(path_.c_str(), std::ios_base::out);
    f.exceptions (std::ios_base::badbit|std::ios_base::failbit);
    f << val;
  }
private:
  std::string path_;
  std::string name_;
};

template <typename T>
inline std::ostream& operator<<(std::ostream& s, const FsEntry<T>& d) {
  s << d.name();
  return s;
}

class TempSensor: public FsEntry<int> {
public:
  typedef FsEntry<int> parent_t;
  using parent_t::value_t;

  TempSensor(const std::string& path, const std::string& name, value_t target, value_t critical)
    : parent_t(path, name), target_(target), critical_(critical) {}

  value_t target() const { return target_; }
  value_t critcal() const { return critical_; }

private:
  value_t target_;
  value_t critical_;
};

inline std::ostream& operator<<(std::ostream& s, const TempSensor& d) {
  s << static_cast<TempSensor::parent_t>(d) << " current " << d.get()/1000 << ", target " << d.target() << ", critical " << d.critcal();
  return s;
}

class Fan: public FsEntry<int> {
public:
  typedef FsEntry<int> parent_t;
  using parent_t::value_t;

  Fan(const std::string& path, const std::string& name, const std::string& enablePath)
    : parent_t(path, name), enablePath(enablePath) {}

  void enable() {
    std::ofstream f(enablePath.c_str(), std::ios_base::out);
    f.exceptions (std::ios_base::badbit|std::ios_base::failbit);
    f << "1";
  }
  void disengage() {
    std::ofstream f(enablePath.c_str(), std::ios_base::out);
    f.exceptions (std::ios_base::badbit|std::ios_base::failbit);
    f << "0";
  }

private:
  std::string enablePath;
};

typedef FsEntry<int> FanWatchDog;

class System {
public:
  System() : pid(70) {}

  // return true on success
  bool scanACPI();

  struct State {
    TempSensor::value_t temperature;
    Fan::value_t speed;
    const TempSensor& temp;
  };

  struct CriticalTemp : public std::runtime_error {
    CriticalTemp(TempSensor::value_t value, const TempSensor& temp);
  };

  State run(int period);

  // set fans speed to maximum
  void disengage();

private:
  State readTemp();
  void setFanSpeed(Fan::value_t t);
  void watchdog(int period);

private:
  friend std::ostream& operator<<(std::ostream& s, const System& v);

  PID<TempSensor::value_t, Fan::value_t, 85, 255> pid; // do not stop fan completely to cool video card

  std::vector<TempSensor> temps;
  std::vector<Fan> fans;
  boost::scoped_ptr<FanWatchDog> fanWatchDog;
};

inline std::ostream& operator<<(std::ostream& s, const System& v) {
  s << "Temperature sensors:\n";
  for (auto r = v.temps.begin(); r < v.temps.end(); ++r) {
      s << "\t" <<  *r << "\n";
  }
  s << "Fans:\n";
  for (auto r = v.fans.begin(); r < v.fans.end(); ++r) {
      s << "\t" <<  *r << "\n";
  }
  if (v.fanWatchDog)
    s << "Fan whatchdog is present." << std::endl;

  return s;
}
#endif /* SENSOR_HPP_ */
