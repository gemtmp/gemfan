/*
 * PID.hpp
 *
 *  Created on: Mar 3, 2012
 *      Author: gem
 */

#ifndef PID_HPP_
#define PID_HPP_

template <typename value_t, typename action_t, action_t minAction, action_t maxAction>
class PID {
public:
  PID(const value_t& target)
    : target_(target)
      , previos(target), accumulator(0), action_value(0) {
    p = 1.0;
    i = 0.02;
    d_pos = 6;
    d_neg = 4;
    accumulator_max = 2;
  }

  void target(const value_t& t) { target_ = t; }

  action_t step(value_t current) {

    double delta = (current - target_) * p + getI(current) + getD(current);

    if (action_value + delta > maxAction)
      action_value = maxAction;
    else if (action_value + delta < minAction)
      action_value = minAction;
    else
      action_value += delta;
    previos = current;
    return action_value;
  }

  value_t value() {
    return previos;
  }
  action_t action() {
      return action_value;
    }

private:
  double getI(value_t current) {
    accumulator += i * (current - target_);
    if (accumulator > accumulator_max)
      accumulator = accumulator_max;
    else if (accumulator < -accumulator_max)
      accumulator = -accumulator_max;
    return accumulator;
  }
  double getD(value_t current) {
    value_t d = current - previos;
    return d * (d > 0 ? d_pos : d_neg);
  }
  value_t target_;
  value_t previos;
  double accumulator;
  double accumulator_max;
  action_t action_value;
  double p;
  double i;
  double d_pos;
  double d_neg;
};

#endif /* PID_HPP_ */
