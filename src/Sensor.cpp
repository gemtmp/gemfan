/*
 * Sensor.cpp
 *
 *  Created on: May 31, 2012
 *      Author: gem
 */

#include <limits>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "Sensor.hpp"

namespace {
namespace fs = boost::filesystem;

class Scanner {
public:
  Scanner(const fs::path& p, const std::string& prefix, const std::string& suffix)
      : p(p), prefix(prefix), suffix(suffix) {
    index = parseIndex();
  }

  const std::string& path() const { return p.string(); }

  std::string label() const {
    fs::path name = entry("_label");
    if (!fs::is_regular_file(name)) {
      name = p.parent_path() / "name";
    }
    std::string ret;
    if (fs::is_regular_file(name)) {
      std::ifstream f(name.string());
      std::getline(f, ret);
    } else {
      ret = p.parent_path().filename().string();
    }
    return ret;
  }

  int max() const {
    return getEntry<int>("_max");
  }

  int critical() const {
    return getEntry<int>("_crit");
  }

  bool isValid() const {
    return index > 0;
  }

  fs::path entry(const std::string& name) const {
    return fs::path(
        p.parent_path()
            / (prefix + boost::lexical_cast<std::string>(index) + name));
  }

private:

  size_t parseIndex() const {
    std::string name = p.filename().string();
    if (name.size() > prefix.size() + suffix.size()
        && name.compare(0, prefix.size(), prefix) == 0
        && name.compare(name.size() - suffix.size(),
            suffix.size(), suffix) == 0)
      try {
      return boost::lexical_cast<size_t>(
          name.substr(prefix.size(),
              name.size() - (prefix.size() + suffix.size())));
      } catch (boost::bad_lexical_cast&) {}
    return 0;
  }

  template <typename T>
  T getEntry(const std::string& name) const {
    fs::path m = entry(name);
    if (!fs::is_regular_file(m)) {
      return T();
    }
    std::ifstream f(m.string());
    f.exceptions (std::ios_base::badbit|std::ios_base::failbit);
    T ret;
    f >> ret;
    return ret;
  }
private:
  const fs::path& p;
  const std::string& prefix;
  const std::string& suffix;
  size_t index;
};

const std::string tempPrefix = { "temp" };
const std::string tempSuffix = { "_input" };

const std::string fanPrefix = { "pwm" };
const std::string fanSuffix = {  };

const int defaultCritical = 100;

TempSensor createTemp(const Scanner& s) {
  int max = s.max()/1000;
  int critical = s.critical()/1000;
  if (critical == 0) critical = defaultCritical;
  if (max == 0) max = critical;
  int target = std::min(std::max(0, max - 20), std::max(0, critical - 30));

  return {s.path(), s.label(), target, critical};
}

} // namespace

bool System::scanACPI() {
  const static std::string root = "/sys";

  for (fs::recursive_directory_iterator d(root);
      d != fs::recursive_directory_iterator(); ++d) {
    if (!fs::is_regular_file(*d))
      continue;
    {
      Scanner s (d->path(), tempPrefix, tempSuffix);
      if (s.isValid()) {
        temps.push_back(createTemp(s));
        continue;
      }
    }
    {
      Scanner s (d->path(), fanPrefix, fanSuffix);
      if (s.isValid()) {
        fans.push_back({s.path(), s.label(), s.entry("_enable").string() });
        continue;
      }
    }
    if (d->path().filename() == "fan_watchdog") {
      fanWatchDog.reset(new FanWatchDog(d->path().string(), ""));
      continue;
    }
  }

  return !temps.empty() && !fans.empty();
}

System::State System::run(int period) {
  watchdog(period +1);
  auto state = readTemp();
  pid.target(state.temp.target());
  state.speed = pid.step(state.temperature);
  setFanSpeed(state.speed);
  return state;
}

void System::disengage() {
  for (auto r = fans.begin(); r < fans.end(); ++r) {
    r->disengage();
  }
}

System::State System::readTemp() {
  auto ret = std::numeric_limits<TempSensor::value_t>::max();
  auto delta = std::numeric_limits<int>::min();
  auto temp = temps.end();
  for (auto r = temps.begin(); r < temps.end(); ++r) {
    auto val = r->get();
    val /= 1000;
    if (val >= r->critcal() - 3) {
      throw CriticalTemp(val, *r);
    }
    if (delta < val - r->target()) {
      ret = val;
      temp = r;
      delta = val - r->target();
    }
  }
  return {ret, 0, *temp};
}

void System::setFanSpeed(Fan::value_t t) {
  for (auto r = fans.begin(); r < fans.end(); ++r) {
    r->enable();
    r->set(t);
  }
}

void System::watchdog(int period) {
  if (fanWatchDog)
    fanWatchDog->set(period);
}

System::CriticalTemp::CriticalTemp(TempSensor::value_t value, const TempSensor& temp)
    : std::runtime_error("Critical temperature at " + temp.name() + ": "
        + boost::lexical_cast<std::string>(value))
{}
