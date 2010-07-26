// Copyright 2010 Rubix Consulting, Inc.

#include "./note.h"
#include <string>
#include <vector>

namespace rubix {
note::note() {
  _init();
}

note::note(const row& val) {
  _init();
  id(val.find("note_id")->second);
  value(val.find("note")->second);
  _created(val.find("created")->second);
  _modified(val.find("modified")->second);
}

boost::posix_time::ptime note::_now() const {
  return boost::posix_time::second_clock::universal_time();
}

void note::_init() {
  id(0);
  value("");
  _created(_now());
  _modified(_now());
  __tags.clear();
}

void note::clear() {
  _init();
}

const uint32_t& note::id() const {
  return __id;
}

const uint32_t& note::id(const uint32_t& val) {
  __id = val;
  return id();
}

const uint32_t& note::id(const std::string& val) {
  std::istringstream iss(val);
  int ival;
  if (iss >> ival) {
    return id(ival);
  }
  throw std::runtime_error("could not convert string to int: "+val);
}

const std::string& note::value() const {
  return __value;
}

const std::string& note::value(const std::string& val) {
  __value = val;
  // TODO(jrubin) parse tags
  _modified(_now());
  return value();
}

const boost::posix_time::ptime& note::created() const {
  return __created;
}

const boost::posix_time::ptime& note::_created(
    const boost::posix_time::ptime& val) {
  __created = val;
  return created();
}

const boost::posix_time::ptime& note::_created(const std::string& val) {
  return _created(boost::posix_time::time_from_string(val));
}

const boost::posix_time::ptime& note::modified() const {
  return __modified;
}

const boost::posix_time::ptime& note::_modified(
    const boost::posix_time::ptime& val) {
  __modified = val;
  return modified();
}

const boost::posix_time::ptime& note::_modified(const std::string& val) {
  return _modified(boost::posix_time::time_from_string(val));
}

const rubix::tags& note::tags() const {
  return __tags;
}
};

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
