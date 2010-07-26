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
    typedef std::map <const std::string, std::string> _row_;
    typedef std::pair<const std::string, std::string> _row_pair_;
    typedef std::vector<_row_>                        _rows_;
    typedef std::vector<std::string>                  _string_v_;

    // properties
    sqlite3*    __db;
    std::string __db_filename;

    // methods
    void               _init_db();
    void               _upgrade_db();
    void               _exec(const std::string&);
    void               _insert(const std::string&, const _row_&);
    void               _update(const std::string&, const _row_&, const _row_&);
    void               _delete(const std::string&, const _row_&);
    void               _exec_prepared(const std::string&, const _string_v_&);
    int                _str_to_int(const std::string&);
    int                _get_int(const std::string&);
    _row_              _get_row(const std::string&);
    _rows_             _get_rows(const std::string&);
    std::string        _get_val(const std::string&);
    std::string        _join(const _string_v_&, const std::string&);
    std::string        _make_qs(const _row_&, _string_v_*);
    const std::string& _db_filename();
};
};

#endif  // SRC_LIB_ROTE_DB_H_

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
