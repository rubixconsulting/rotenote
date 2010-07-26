// Copyright 2010 Rubix Consulting, Inc.

#ifndef SRC_LIB_ROTE_DB_H_
#define SRC_LIB_ROTE_DB_H_

#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>

namespace rubix {

#define DB_NAME        "rotedb"
#define SCHEMA_VERSION 1

class rote_db {
  public:
    // constructors
    rote_db();
    ~rote_db();

    // methods
    // bool                     saveNote(note*)
    // std::vector<std::string> listTags()
    // std::vector<note>        listNotes();
    // std::vector<note>        listNotes(sort)
    // std::vector<note>        search(string)
    // std::vector<note>        byTag(tag)

  private:
    // typedefs
    typedef std::map<const std::string, std::string> _row_;
    typedef std::vector<_row_>                       _rows_;

    // properties
    sqlite3*    __db;
    std::string __db_filename;

    // methods
    void               _init_db();
    void               _upgrade_db();
    int                _str_to_int(const std::string&);
    _rows_             _exec(const std::string&);
    const std::string& _db_filename();
};
};

#endif  // SRC_LIB_ROTE_DB_H_

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
