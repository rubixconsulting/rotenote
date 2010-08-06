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

  out->clear();
  string str = value();
  const string::size_type begin = *iter;

  // find the first newline
  string::size_type title_end = str.find_first_of("\n");

  if ((str.empty()) || (*iter >= str.size())) {
    *iter = str.size();
    return TEXT_INVALID;
  } else if (title_end == string::npos) {
    // there was no newline, so the whole thing is the title
    *iter = str.size();
    *out = str;
    return TEXT_TITLE;
  } else if (*iter < title_end) {
    // the iterator was before the first newline
    *iter = title_end;
    *out = str.substr(begin, *iter-begin);
    return TEXT_TITLE;
  }

  string token;
  text_type type = TEXT_INVALID, special_type = TEXT_PLAIN;
  special_state state;
  string::size_type tmp_iter;

  while (*iter < str.size()) {
    tmp_iter = *iter;
    text_type last_type = type;

    token = _token(str, &tmp_iter);
    state = _is_special(token, &type);

    if (!out->empty() &&
        ((state == BEGIN_SPECIAL) || (state == BEGIN_END_SPECIAL))) {
      // there is stuff in the buffer and we encountered the beginning of a
      // special type, don't advance the iterator or type and return the output
      // buffer
      return last_type;
    }

    // append to the output buffer and advance the iterator
    *out += token;
    *iter = tmp_iter;

    if (state == BEGIN_SPECIAL) {
      // keep track of the type of special part we have, but default to
      // TEXT_PLAIN
      special_type = type;
    } else if ((state == END_SPECIAL) || (state == BEGIN_END_SPECIAL)) {
      // we found the end of the special part
      if (state == END_SPECIAL) {
        // this is a multi part buffer, so it should be set to the type we found
        // at the beginning and reset the special type to TEXT_PLAIN
        type = special_type;
        special_type = TEXT_PLAIN;
      }

      return type;
    }
  }

  return type;
}

special_state note::_is_special(const std::string& str, text_type *type) const {
  static regex link_regex(SCHEME "//(?:" USERPASS "\\@)?" HOST PORT URLPATH, icase);
  static regex link_default_http_regex("(?:www|ftp)" HOSTCHARS_CLASS "*\\." HOST PORT URLPATH, icase);
  static regex email_regex("(?:mailto:)?" USERCHARS_CLASS "[" USERCHARS ".]*\\@" HOSTCHARS_CLASS "+\\." HOST, icase);

  if (str.empty()) {
    *type = TEXT_INVALID;
    return BEGIN_END_SPECIAL;

  } else if (str[0] == TAG_DELIM) {
    *type = TEXT_TAG;
    return BEGIN_END_SPECIAL;

  } else if (str[0] == TWITTER_DELIM) {
    *type = TEXT_TWITTER;
    return BEGIN_END_SPECIAL;

  // begin BOLD
  } else if (str[0] == BOLD_DELIM) {
    *type = TEXT_BOLD;
    if (str[str.size()-1] == BOLD_DELIM) {
      // end BOLD
      return BEGIN_END_SPECIAL;
    }
    return BEGIN_SPECIAL;

  // end BOLD
  } else if (str[str.size()-1] == BOLD_DELIM) {
    *type = TEXT_BOLD;
    return END_SPECIAL;

  // begin ITALIC
  } else if (str[0] == ITALIC_DELIM) {
    *type = TEXT_ITALIC;
    if (str[str.size()-1] == ITALIC_DELIM) {
      // end ITALIC
      return BEGIN_END_SPECIAL;
    }
    return BEGIN_SPECIAL;

  // end ITALIC
  } else if (str[str.size()-1] == ITALIC_DELIM) {
    *type = TEXT_ITALIC;
    return END_SPECIAL;

  // begin UNDERLINE
  } else if (str[0] == UNDERLINE_DELIM) {
    *type = TEXT_UNDERLINE;
    if (str[str.size()-1] == UNDERLINE_DELIM) {
      // end UNDERLINE
      return BEGIN_END_SPECIAL;
    }
    return BEGIN_SPECIAL;

  // end UNDERLINE
  } else if (str[str.size()-1] == UNDERLINE_DELIM) {
    *type = TEXT_UNDERLINE;
    return END_SPECIAL;

  } else if (regex_match(str, link_regex)) {
    *type = TEXT_LINK;
    return BEGIN_END_SPECIAL;

  } else if (regex_match(str, link_default_http_regex)) {
    *type = TEXT_LINK_DEFAULT_HTTP;
    return BEGIN_END_SPECIAL;

  } else if (regex_match(str, email_regex)) {
    *type = TEXT_EMAIL;
    return BEGIN_END_SPECIAL;
  }

  *type = TEXT_PLAIN;
  return SPECIAL_UNKNOWN;
}

string note::_token(const std::string& str,
                    string::size_type *iter) const {
  const string::size_type begin = *iter;
  string test = str.substr(begin);

  if (test.empty()) {
    *iter = str.size();
    return false;
  } else if (test.size() == 1) {
    *iter = str.size();
    return test;
  }

  bool is_separator = _is_separator(test);
  ++(*iter);

  while (*iter < str.size()) {
    if (is_separator && !_is_separator(str.substr(*iter))) {
      break;
    } else if (!is_separator && _is_separator(str.substr(*iter))) {
      break;
    }
    ++(*iter);
  }

  return str.substr(begin, (*iter-begin));
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
