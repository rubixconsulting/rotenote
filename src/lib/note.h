// Copyright 2010 Rubix Consulting, Inc.

#ifndef SRC_LIB_NOTE_H_
#define SRC_LIB_NOTE_H_

#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>
#include <vector>
#include <set>
#include "./types.h"

namespace rubix {

#define TAG_ALL         "#all"
#define TAG_DELIM       '#'
#define BOLD_DELIM      '*'
#define ITALIC_DELIM    '/'
#define UNDERLINE_DELIM '_'
#define TWITTER_DELIM   '@'

enum text_type {
  TEXT_INVALID,
  TEXT_PLAIN,
  TEXT_TITLE,
  TEXT_BOLD,
  TEXT_ITALIC,
  TEXT_UNDERLINE,
  TEXT_TAG,
  TEXT_LINK,
  TEXT_LINK_DEFAULT_HTTP,
  TEXT_EMAIL,
  TEXT_TWITTER
};

enum special_state {
  SPECIAL_UNKNOWN,
  BEGIN_SPECIAL,
  END_SPECIAL,
  BEGIN_END_SPECIAL
};

class note {
  public:
    // constructors
    note();
    explicit note(const row&);

    // operators
    note& operator<<(const std::string&);

    // methods
    void                            clear();
    void                            write_to_file(const std::string&) const;
    text_type                       part(std::string::size_type*, std::string*) const;
    const uint32_t&                 id() const;
    const uint32_t&                 id(const uint32_t&);
    const uint32_t&                 id(const std::string&);
    const rubix::tags&              tags() const;
    const std::string&              title() const;
    const std::string&              body() const;
    const std::string&              value() const;
    const std::string&              value(const std::string&);
    const std::string&              load_from_file(const std::string&);
    const boost::posix_time::ptime& created_at() const;
    const boost::posix_time::ptime& updated_at() const;

  private:
    // properties
    uint32_t                 __id;
    rubix::tags              __tags;
    std::string              __title;
    std::string              __body;
    std::string              __value;
    boost::posix_time::ptime __created_at;
    boost::posix_time::ptime __updated_at;

    // methods
    void                            _init();
    bool                            _is_separator(const std::string&) const;
    std::string                     _token(const std::string&,
                                           std::string::size_type*) const;
    special_state                   _is_special(const std::string&, text_type*) const;
    const std::string&              _title(const std::string&);
    const std::string&              _body(const std::string&);
    boost::posix_time::ptime        _now() const;
    const boost::posix_time::ptime& _created_at(const std::string&);
    const boost::posix_time::ptime& _created_at(const boost::posix_time::ptime&);
    const boost::posix_time::ptime& _updated_at(const std::string&);
    const boost::posix_time::ptime& _updated_at(const boost::posix_time::ptime&);
};
typedef std::vector<note> notes;
std::ostream& operator<<(std::ostream&, const note&);
std::istream& operator>>(std::istream&, note&);
};

#endif  // SRC_LIB_NOTE_H_

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
