// Copyright 2010 Rubix Consulting, Inc.

#include "./note.h"
#include <fstream>
#include <string>
#include <vector>
#include <boost/regex.hpp>

using ::std::string;
using ::std::istringstream;
using ::std::runtime_error;
using ::std::invalid_argument;
using ::std::ostream;
using ::std::istream;
using ::std::ifstream;
using ::std::ofstream;
using ::boost::regex;
using ::boost::regex_match;
using ::boost::regex_constants::icase;

namespace rubix {
#define USERCHARS "-[:alnum:]"
#define USERCHARS_CLASS "[" USERCHARS "]"
#define PASSCHARS_CLASS "[-[:alnum:]\\Q,?;.:/!%$^*&~\"#'\\E]"
#define HOSTCHARS_CLASS "[-[:alnum:]]"
#define HOST HOSTCHARS_CLASS "+(\\." HOSTCHARS_CLASS "+)*"
#define PORT "(?:\\:[[:digit:]]{1,5})?"
#define PATHCHARS_CLASS "[-[:alnum:]\\Q_$.+!*,;@&=?/~#%\\E]"
#define SCHEME "(?:news:|telnet:|nntp:|file:\\/|https?:|ftps?:|webcal:)"
#define USERPASS USERCHARS_CLASS "+(?:" PASSCHARS_CLASS "+)?"
#define URLPATH   "(?:(/"PATHCHARS_CLASS"+(?:[(]"PATHCHARS_CLASS"*[)])*"PATHCHARS_CLASS"*)*)?"

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
  string val_part, title, body;
  text_type type;

  while ((type = part(&iter, &val_part)) != TEXT_INVALID) {
    switch (type) {
      case TEXT_TITLE:
        title += val_part;
        break;
      case TEXT_SEPARATOR_TITLE:
        title += val_part;
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
  _title(title);

  _modified(_now());
  return value();
}

text_type note::part(string::size_type *iter, string *out) const {
  if (!iter || !out) {
    throw invalid_argument("note::part iter or val is null");
  }

  string str = value();

  text_type default_type      = TEXT_PLAIN;
  text_type default_separator = TEXT_SEPARATOR_PLAIN;

  // find the first newline
  string::size_type title_end = str.find_first_of("\n");

  if ((title_end == string::npos) || (*iter < title_end)) {
    // there was no newline, so the whole thing is the title
    // or the iterator was before the first newline
    default_type      = TEXT_TITLE;
    default_separator = TEXT_SEPARATOR_TITLE;
  }

  text_type type = _token(str, default_type, default_separator, iter, out);

  return type;
}

text_type note::_token(const string& str,
                       const text_type& default_type,
                       const text_type& default_separator,
                       string::size_type *iter,
                       string *out) const {
  if (!iter || !out) {
    throw invalid_argument("note::_token iter or val is null");
  }

  out->clear();
  const string::size_type begin = *iter;

  if (str.empty() || (*iter >= str.size())) {
    *iter = str.size();
    return TEXT_INVALID;
  }

  bool is_separator = _is_separator(str.substr(begin));

  if (str.size() == 1) {
    *out = str;
    if (is_separator) {
      return default_separator;
    }
    return default_type;
  }

  ++(*iter);

  while (*iter < str.size()) {
    if (is_separator && !_is_separator(str.substr(*iter))) {
      *out = str.substr(begin, (*iter-begin));
      return default_separator;
    } else if (!is_separator && _is_separator(str.substr(*iter))) {
      break;
    }
    ++(*iter);
  }

  *out = str.substr(begin, (*iter-begin));

  static regex link_regex(SCHEME "//(?:" USERPASS "\\@)?" HOST PORT URLPATH, icase);
  static regex link_default_http_regex("(?:www|ftp)" HOSTCHARS_CLASS "*\\." HOST PORT URLPATH, icase);
  static regex email_regex("(?:mailto:)?" USERCHARS_CLASS "[" USERCHARS ".]*\\@" HOSTCHARS_CLASS "+\\." HOST, icase);

  if (out->empty()) {
    return TEXT_INVALID;
  } else if ((*out)[0] == TAG_DELIM) {
    return TEXT_TAG;
  } else if ((*out)[0] == TWITTER_DELIM) {
    return TEXT_TWITTER;
  } else if (((*out)[0] == BOLD_DELIM) && ((*out)[out->size()-1] == BOLD_DELIM)) {
    return TEXT_BOLD;
  } else if (regex_match(*out, link_regex)) {
    return TEXT_LINK;
  } else if (regex_match(*out, link_default_http_regex)) {
    return TEXT_LINK_DEFAULT_HTTP;
  } else if (regex_match(*out, email_regex)) {
    return TEXT_EMAIL;
  }

  return default_type;
}

bool note::_is_separator(const string& str) const {
  if (str.empty()) {
    return false;
  }

  const unsigned char& j = str[0];

  switch (j) {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
    case '\f':
      return true;
    case '.':
      if (str.size() == 1) {
        return true;
      }
      return _is_separator(str.substr(1));
  }

  return false;
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
