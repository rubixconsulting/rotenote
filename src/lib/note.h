// Copyright 2010 Rubix Consulting, Inc.

#ifndef SRC_LIB_NOTE_H_
#define SRC_LIB_NOTE_H_

#include <boost/date_time/gregorian/gregorian.hpp>
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
    void                          clear();
    const uint32_t&               id() const;
    const uint32_t&               id(const uint32_t&);
    const uint32_t&               id(const std::string&);
    const rubix::tags&            tags() const;
    const std::string&            value() const;
    const std::string&            value(const std::string&);
    const boost::gregorian::date& created() const;
    const boost::gregorian::date& modified() const;

  private:
    // properties
    uint32_t                 __id;
    rubix::tags              __tags;
    std::string              __value;
    boost::gregorian::date   __created;
    boost::gregorian::date   __modified;

    // methods
    void                          _init();
    const boost::gregorian::date& _created(const std::string&);
    const boost::gregorian::date& _created(const boost::gregorian::date&);
    const boost::gregorian::date& _modified(const std::string&);
    const boost::gregorian::date& _modified(const boost::gregorian::date&);
};
typedef std::vector<note> notes;
};

#endif  // SRC_LIB_NOTE_H_

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
