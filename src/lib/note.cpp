// Copyright 2010 Rubix Consulting, Inc.

#include "./note.h"
#include <string>
#include <vector>

namespace rubix {
note::note() {
  _init();
}

note::note(const int& idval, const std::string& val,
           const boost::gregorian::date& create,
           const boost::gregorian::date& modify) {
  _init();
  id(idval);
  value(val);
  _created(create);
  _modified(modify);
}

void note::_init() {
  id(0);
  value("");
  _created(boost::gregorian::date());
  _modified(boost::gregorian::date());
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

const std::string& note::value() const {
  return __value;
}

const std::string& note::value(const std::string& val) {
  __value = val;
  // TODO(jrubin) parse tags
  _modified(boost::gregorian::date());
  return value();
}

const boost::gregorian::date& note::created() const {
  return __created;
}

const boost::gregorian::date& note::_created(
    const boost::gregorian::date& val) {
  __created = val;
  return created();
}

const boost::gregorian::date& note::modified() const {
  return __modified;
}

const boost::gregorian::date& note::_modified(
    const boost::gregorian::date& val) {
  __modified = val;
  return modified();
}

const std::vector<std::string>& note::tags() const {
  return __tags;
}
};

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
