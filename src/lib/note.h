// Copyright 2010 Rubix Consulting, Inc.

#ifndef SRC_LIB_NOTE_H_
#define SRC_LIB_NOTE_H_

#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>
#include <vector>
#include <set>
#include "./types.h"

namespace rubix {
class note {
  public:
    // constructors
    note();
    explicit note(const row&);

    // methods
    void                            clear();
    const uint32_t&                 id() const;
    const uint32_t&                 id(const uint32_t&);
    const uint32_t&                 id(const std::string&);
    const rubix::tags&              tags() const;
    const std::string&              title() const;
    const std::string&              body() const;
    const std::string&              value() const;
    const std::string&              value(const std::string&);
    const boost::posix_time::ptime& created() const;
    const boost::posix_time::ptime& modified() const;

  private:
    // properties
    uint32_t                 __id;
    rubix::tags              __tags;
    std::string              __title;
    std::string              __body;
    std::string              __value;
    boost::posix_time::ptime __created;
    boost::posix_time::ptime __modified;

    // methods
    void                            _init();
    const std::string&              _title(const std::string&);
    const std::string&              _body(const std::string&);
    boost::posix_time::ptime        _now() const;
    const boost::posix_time::ptime& _created(const std::string&);
    const boost::posix_time::ptime& _created(const boost::posix_time::ptime&);
    const boost::posix_time::ptime& _modified(const std::string&);
    const boost::posix_time::ptime& _modified(const boost::posix_time::ptime&);
};
typedef std::vector<note> notes;
};

#endif  // SRC_LIB_NOTE_H_

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
