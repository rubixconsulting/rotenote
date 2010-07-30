// Copyright 2010 Rubix Consulting, Inc.

#ifndef SRC_LIB_ROTE_DB_H_
#define SRC_LIB_ROTE_DB_H_

#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>
#include "./types.h"
#include "./note.h"

namespace rubix {
class rote_db {
  public:
    // constructors
    explicit rote_db(const std::string&);
    ~rote_db();

    // methods
    void  delete_note(note* value) const;
    int   save_note(note* value) const;
    int   num_notes() const;
    tags  list_tags() const;
    note  by_id(const uint32_t&) const;
    notes list_notes(const sort&) const;
    notes search(const std::string&, const sort&) const;
    notes by_tag(const std::string&, const sort&) const;

  private:
    // properties
    sqlite3*    __db;
    std::string __db_filename;

    // methods
    void               _init(const std::string&);
    void               _init_db() const;
    void               _upgrade_db() const;
    void               _exec(const std::string&) const;
    void               _update(const std::string&,
                               const row&,
                               const row&) const;
    void               _delete(const std::string&, const row&) const;
    void               _save_tag(const std::string&, const note* value) const;
    void               _begin() const;
    void               _commit() const;
    void               _rollback() const;
    int                _insert(const std::string&, const row&) const;
    int                _insert_note(note* value) const;
    int                _update_note(const note*) const;
    int                _save_tags(const note*) const;
    int                _str_to_int(const std::string&) const;
    int                _get_int(const std::string&) const;
    int                _get_int(const std::string&, const string_v&) const;
    row                _get_row(const std::string&) const;
    row                _get_row(const std::string&, const string_v&) const;
    rows               _get_rows(const std::string&) const;
    rows               _get_rows(const std::string&, const string_v&) const;
    string_v           _get_col(const std::string&) const;
    string_v           _get_col(const std::string&, const string_v&) const;
    rows               _exec_prepared(const std::string&,
                                      const string_v&) const;
    std::string        _int_to_str(const int&) const;
    std::string        _get_val(const std::string&) const;
    std::string        _get_val(const std::string&, const string_v&) const;
    std::string        _join(const string_v&, const std::string&) const;
    std::string        _make_qs(const row&, string_v*) const;
    std::string        _order_by(const sort&) const;
    const std::string& _db_filename() const;
    const std::string& _db_filename(const std::string&);
};
};

#endif  // SRC_LIB_ROTE_DB_H_

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
