// Copyright 2010 Rubix Consulting, Inc.

#ifndef SRC_LIB_ROTE_DB_H_
#define SRC_LIB_ROTE_DB_H_

#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>
#include "./note.h"

namespace rubix {

#define SCHEMA_VERSION 1

enum sort {
  TITLE,
  TITLE_DESC,
  CREATION,
  CREATION_DESC,
  MODIFIED,
  MODIFIED_DESC
};

class rote_db {
  public:
    // constructors
    explicit rote_db(const std::string&);
    ~rote_db();

    // methods
    bool  save_note(const note*) const;
    tags  list_tags() const;
    notes list_notes() const;
    notes list_notes(const sort&) const;
    notes search(const std::string&) const;
    notes by_tag(const std::string&) const;

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
    void               _init(const std::string&);
    void               _init_db() const;
    void               _upgrade_db() const;
    void               _exec(const std::string&) const;
    void               _insert(const std::string&, const _row_&) const;
    void               _update(const std::string&,
                               const _row_&,
                               const _row_&) const;
    void               _delete(const std::string&, const _row_&) const;
    void               _exec_prepared(const std::string&,
                                      const _string_v_&) const;
    bool               _insert_note(const note*) const;
    bool               _update_note(const note*) const;
    int                _str_to_int(const std::string&) const;
    int                _get_int(const std::string&) const;
    _row_              _get_row(const std::string&) const;
    _rows_             _get_rows(const std::string&) const;
    std::string        _get_val(const std::string&) const;
    std::string        _join(const _string_v_&, const std::string&) const;
    std::string        _make_qs(const _row_&, _string_v_*) const;
    const std::string& _db_filename() const;
    const std::string& _db_filename(const std::string&);
};
};

#endif  // SRC_LIB_ROTE_DB_H_

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
