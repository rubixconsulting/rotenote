// Copyright 2010 Rubix Consulting, Inc.

#include "./note.h"
#include <fstream>
#include <string>
#include <vector>

using ::std::string;
using ::std::istringstream;
using ::std::runtime_error;
using ::std::invalid_argument;
using ::std::ostream;
using ::std::istream;
using ::std::ifstream;
using ::std::ofstream;

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
  _title("");
  _body("");
  __tags.clear();
  __tags.insert(TAG_ALL);
}

const string& note::title() const {
  return __title;
}

const string& note::_title(const string& val) {
  __title = val;
  return title();
}

const string& note::body() const {
  return __body;
}

const string& note::_body(const string& val) {
  __body = val;
  return body();
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

const uint32_t& note::id(const string& val) {
  istringstream iss(val);
  int ival;
  if (iss >> ival) {
    return id(ival);
  }
  throw runtime_error("could not convert string to int: "+val);
}

const string& note::value() const {
  return __value;
}

const string& note::value(const string& val) {
  __value = val;

  __tags.clear();
  __tags.insert(TAG_ALL);
  string::size_type iter = 0;
  string val_part, body;
  text_type type;

  while ((type = part(&iter, &val_part)) != TEXT_INVALID) {
    switch (type) {
      case TEXT_TITLE:
        _title(val_part);
        break;
      case TEXT_TAG:
        body += val_part;
        __tags.insert(val_part);
        break;
      default:
        body += val_part;
    }
  }

  _body(body);

  _modified(_now());
  return value();
}

text_type note::part(string::size_type *iter, string *out) const {
  if (!iter || !out) {
    throw invalid_argument("note::part iter or val is null");
  }

  out->clear();
  string cur = value();

  if (*iter >= cur.size()) {
    *iter = cur.size();
    return TEXT_INVALID;
  }

  string::size_type title_end = cur.find_first_of("\n");

  if (*iter < title_end) {
    *out  = cur.substr(*iter, title_end);
    *iter = title_end;
    return TEXT_TITLE;
  }

  text_type type = TEXT_PLAIN;

  if (cur[*iter] == TAG_DELIM) {
    type = TEXT_TAG;
    *out += TAG_DELIM;
    ++(*iter);
  }

  while (*iter < cur.size()) {
    const unsigned char& j = cur[*iter];
    if (type == TEXT_TAG) {
      switch (j) {
        case '.':
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\f':
          return type;
      }
    } else if (j == TAG_DELIM) {
      break;
    }
    *out += j;
    ++(*iter);
  }

  return type;
}

const boost::posix_time::ptime& note::created() const {
  return __created;
}

const boost::posix_time::ptime& note::_created(
    const boost::posix_time::ptime& val) {
  __created = val;
  return created();
}

const boost::posix_time::ptime& note::_created(const string& val) {
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

const boost::posix_time::ptime& note::_modified(const string& val) {
  return _modified(boost::posix_time::time_from_string(val));
}

const rubix::tags& note::tags() const {
  return __tags;
}

note& note::operator<<(const string& s) {
  string old = value();
  value(old+s);
  return *this;
}

ostream& operator<<(ostream& os, const note& obj) {
  os << obj.value();
  return os;
}

istream& operator>>(istream& is, note& obj) {
  string val;
  is >> val;
  obj.value(val);
  return is;
}

const string& note::load_from_file(const string& fn) {
  ifstream f(fn.c_str());
  if (!f.is_open()) {
    throw runtime_error("could not open file: "+fn);
  }

  string new_value, line;
  while (getline(f, line)) {
    new_value += line;
    if (!(f.rdstate() & ifstream::eofbit)) {
      new_value += "\n";
    }
  }
  f.close();
  return value(new_value);
}

void note::write_to_file(const string& fn) const {
  ofstream f(fn.c_str());
  if (!f.is_open()) {
    throw runtime_error("could not open file: "+fn);
  }

  f << value();
  f.close();
}
};

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
