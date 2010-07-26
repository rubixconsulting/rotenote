// Copyright 2010 Rubix Consulting, Inc.

#ifndef SRC_LIB_NOTE_H_
#define SRC_LIB_NOTE_H_

#include <boost/date_time/gregorian/gregorian.hpp>
#include <string>
#include <vector>

namespace rubix {
class note {
  public:
    // constructors
    note();
    note(const int&, const std::string&, const boost::gregorian::date&,
         const boost::gregorian::date&);

    // methods
    void                            clear();
    const uint32_t&                 id() const;
    const uint32_t&                 id(const uint32_t&);
    const std::string&              value() const;
    const std::string&              value(const std::string&);
    const boost::gregorian::date&   created() const;
    const boost::gregorian::date&   modified() const;
    const std::vector<std::string>& tags() const;

  private:
    // properties
    uint32_t                 __id;
    std::string              __value;
    boost::gregorian::date   __created;
    boost::gregorian::date   __modified;
    std::vector<std::string> __tags;

    // methods
    void                          _init();
    const boost::gregorian::date& _created(const boost::gregorian::date&);
    const boost::gregorian::date& _modified(const boost::gregorian::date&);
};
};

#endif  // SRC_LIB_NOTE_H_

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
