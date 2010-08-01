// Copyright 2010 Rubix Consulting, Inc.

#include "./rote_db.h"
#include <stdexcept>
#include <sstream>
#include <string>

using ::std::string;
using ::std::stringstream;
using ::std::istringstream;
using ::std::invalid_argument;
using ::std::runtime_error;

namespace rubix {
rote_db::rote_db(const string& dbfile) {
  _init(dbfile);
}

void rote_db::_init(const string& value) {
  __db = 0;

  if (value.empty()) {
    throw invalid_argument("missing database filename");
  }

  _db_filename(value);

  if (!sqlite3_open_v2(value.c_str(),
                       &__db,
                       SQLITE_OPEN_READWRITE,
                       NULL)) {
    _upgrade_db();
    return;
  } else if (!sqlite3_open_v2(value.c_str(),
                              &__db,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                              NULL)) {
    _init_db();
    return;
  }

  throw runtime_error("could not open database: "+value);
}

rote_db::~rote_db() {
  sqlite3_close(__db);
}

void rote_db::_exec(const string& sql) const {
  char *err_msg   = 0;
  if (sqlite3_exec(__db, sql.c_str(), NULL, NULL, &err_msg) == SQLITE_OK) {
    return;
  }
  stringstream ss;
  ss << "SQL error: " << err_msg;
  sqlite3_free(err_msg);
  throw runtime_error(ss.str());
}

string rote_db::_get_val(const string& sql) const {
  const string_v vs;
  return _get_val(sql, vs);
}

string rote_db::_get_val(const string& sql,
                              const string_v& vs) const {
  return _get_rows(sql, vs)[0].begin()->second;
}

int rote_db::_get_int(const string& sql) const {
  const string_v vs;
  return _get_int(sql, vs);
}

int rote_db::_get_int(const string& sql, const string_v& vs) const {
  return _str_to_int(_get_val(sql, vs));
}

row rote_db::_get_row(const string& sql) const {
  const string_v vs;
  return _get_row(sql, vs);
}

row rote_db::_get_row(const string& sql, const string_v& vs) const {
  return _get_rows(sql, vs)[0];
}

string_v rote_db::_get_col(const string& sql) const {
  const string_v vs;
  return _get_col(sql, vs);
}

string_v rote_db::_get_col(const string& sql, const string_v& vs) const {
  string_v ret;
  rubix::rows rows = _get_rows(sql, vs);
  for (rubix::rows::const_iterator it = rows.begin(); it != rows.end(); ++it) {
    const row& row = *it;
    ret.push_back(row.begin()->second);
  }
  return ret;
}

rows rote_db::_get_rows(const string& sql) const {
  const string_v vs;
  return _get_rows(sql, vs);
}

rows rote_db::_get_rows(const string& sql, const string_v& vs) const {
  return _exec_prepared(sql, vs);
}

string rote_db::_join(const string_v& s, const string& glue) const {
  string ret;
  for (string_v::size_type i = 0; i < s.size(); ++i) {
    ret += s[i];
    if (i+1 < s.size()) {
      ret += glue;
    }
  }
  return ret;
}

rows rote_db::_exec_prepared(const string& sql,
                             const string_v& vs) const {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(__db, sql.c_str(), -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    stringstream ss;
    ss << "could not prepare statement \""<< sql << "\" ";
    ss << sqlite3_errmsg(__db) << " (" << rc << ")";
    throw runtime_error(ss.str());
  }

  for (string_v::size_type i = 0; i < vs.size(); ++i) {
    const string& val = vs[i];
    if (sqlite3_bind_text(stmt,
                          i+1,
                          val.c_str(),
                          -1,
                          SQLITE_TRANSIENT) != SQLITE_OK) {
      stringstream ss;
      ss << "could not bind text parameter: " << val;
      throw runtime_error(ss.str());
    }
  }

  rows ret;

  int num_columns = sqlite3_column_count(stmt);
  rc = sqlite3_step(stmt);
  while (rc == SQLITE_ROW) {
    rubix::row row;
    for (int i = 0; i < num_columns; ++i) {
      const char* column_name = sqlite3_column_name(stmt, i);
      row[column_name] = (const char*)sqlite3_column_text(stmt, i);
    }
    ret.push_back(row);
    rc = sqlite3_step(stmt);
  }

  if (rc != SQLITE_DONE) {
    stringstream ss;
    ss << "could not execute statement \""<< sql << "\" ";
    ss << sqlite3_errmsg(__db) << " (" << rc << ")";
    throw runtime_error(ss.str());
  }

  return ret;
}

int rote_db::_insert(const string& table, const row& values) const {
  string_v cols, qs, vs;
  for (row::const_iterator it = values.begin(); it != values.end(); ++it) {
    const row_pair& pair = *it;
    cols.push_back(pair.first);
    qs.push_back("?");
    vs.push_back(pair.second);
  }

  string sql;
  sql  = "INSERT INTO "+table+" (";
  sql +=   _join(cols, ",");
  sql += ") VALUES(";
  sql +=   _join(qs, ",");
  sql += ")";

  _exec_prepared(sql, vs);

  return sqlite3_last_insert_rowid(__db);
}

void rote_db::_update(const string& table,
                      const row& values,
                      const row& conditions) const {
  string sql;
  string_v vs;

  sql  = "UPDATE "+table;
  sql += "  SET "+_make_qs(values, &vs);
  sql += "  WHERE "+_make_qs(conditions, &vs);

  _exec_prepared(sql, vs);
}

void rote_db::_delete(const string& table, const row& conditions) const {
  string sql;
  string_v vs;

  sql  = "DELETE FROM "+table;
  sql += "  WHERE "+_make_qs(conditions, &vs);

  _exec_prepared(sql, vs);
}

string rote_db::_make_qs(const row& values, string_v* vs) const {
  if (!vs) {
    throw runtime_error("invalid vs");
  }
  string ret;
  row::size_type i = 0;
  for (row::const_iterator it = values.begin(); it != values.end(); ++it) {
    const row_pair& pair = *it;
    ret += pair.first + " = ?";
    vs->push_back(pair.second);
    if (i+1 < values.size()) {
      ret += ", ";
    }
    ++i;
  }
  return ret;
}

void rote_db::_init_db() const {
  _begin();

  stringstream sql;

  sql << "PRAGMA foreign_keys = ON";
  _exec(sql.str());

  sql.str("");
  sql << "CREATE TABLE schema_version (";
  sql << "  version INTEGER NOT NULL PRIMARY KEY";
  sql << ")";
  _exec(sql.str());

  sql.str("");
  sql << "CREATE TABLE notes (";
  sql << "  note_id  INTEGER NOT NULL PRIMARY KEY,";
  sql << "  note     TEXT    NOT NULL DEFAULT '',";
  sql << "  created  TEXT    NOT NULL DEFAULT (datetime('now')),";
  sql << "  modified TEXT    NOT NULL DEFAULT (datetime('now'))";
  sql << ")";
  _exec(sql.str());

  sql.str("");
  sql << "CREATE TABLE tags (";
  sql << "  tag TEXT NOT NULL PRIMARY KEY";
  sql << ")";
  _exec(sql.str());

  sql.str("");
  sql << "CREATE TABLE notes_tags (";
  sql << "  note_id INTEGER NOT NULL,";
  sql << "  tag     TEXT    NOT NULL,";
  sql << "  PRIMARY KEY (note_id, tag),";
  sql << "  FOREIGN KEY (note_id) REFERENCES notes(note_id)";
  sql << "    ON DELETE CASCADE ON UPDATE CASCADE,";
  sql << "  FOREIGN KEY (tag)     REFERENCES tags(tag)";
  sql << "    ON DELETE CASCADE ON UPDATE CASCADE";
  sql << ")";
  _exec(sql.str());

  row values;
  values["version"] = _int_to_str(SCHEMA_VERSION);
  _insert("schema_version", values);

  _commit();
}

void rote_db::_upgrade_db() const {
  string sql;
  sql  = "SELECT MAX(version) AS version";
  sql += "  FROM schema_version";
  const int version = _get_int(sql);

  switch (version) {
    case 1:
      return;
    // NOTE: add upgrade cases here
    default:
      throw runtime_error("unknown schema version");
  }
}

string rote_db::_int_to_str(const int& value) const {
  stringstream ss;
  ss << value;
  return ss.str();
}

int rote_db::_str_to_int(const string& value) const {
  istringstream iss(value);
  int ret;
  if (iss >> ret) {
    return ret;
  }
  throw runtime_error("could not convert string to int: "+value);
}

const string& rote_db::_db_filename() const {
  return __db_filename;
}

const string& rote_db::_db_filename(const string& value) {
  __db_filename = value;
  return _db_filename();
}

int rote_db::save_note(note *value) const {
  if (!value) {
    throw invalid_argument("can not save NULL note");
  } else if (!value->id()) {
    return _insert_note(value);
  }
  return _update_note(value);
}

int rote_db::_insert_note(note *value) const {
  _begin();
  row values;
  values["note"]     = value->value();
  values["created"]  = boost::posix_time::to_simple_string(value->created());
  values["modified"] = boost::posix_time::to_simple_string(value->modified());
  const int id = _insert("notes", values);
  value->id(id);
  return _save_tags(value);
}

int rote_db::_update_note(const note *value) const {
  _begin();
  row values, conditions;
  values["note"]     = value->value();
  values["modified"] = boost::posix_time::to_simple_string(value->modified());
  conditions["note_id"] = _int_to_str(value->id());
  _update("notes", values, conditions);
  return _save_tags(value);
}

int rote_db::_save_tags(const note *value) const {
  row conditions;
  conditions["note_id"] = _int_to_str(value->id());
  _delete("notes_tags", conditions);

  string sql;
  sql = "DELETE FROM tags WHERE tag NOT IN (SELECT tag FROM notes_tags)";
  _exec(sql);

  tags t = value->tags();
  for (tags::const_iterator it = t.begin(); it != t.end(); ++it) {
    const string& tag = *it;
    _save_tag(tag, value);
  }
  _commit();
  return value->id();
}

void rote_db::_save_tag(const string& tag, const note *value) const {
  string sql;
  string_v conditions;

  sql  = "INSERT INTO tags (tag) SELECT ?";
  conditions.push_back(tag);
  sql += "  WHERE NOT EXISTS(";
  sql += "    SELECT 1 FROM tags WHERE tag = ?";
  conditions.push_back(tag);
  sql += "  )";

  _exec_prepared(sql, conditions);

  row values;
  values["note_id"] = _int_to_str(value->id());
  values["tag"]     = tag;
  _insert("notes_tags", values);
}

tags rote_db::list_tags() const {
  string_v ts = _get_col("SELECT tag FROM tags");
  tags ret;
  for (string_v::const_iterator it = ts.begin(); it != ts.end(); ++it) {
    const string& val = *it;
    ret.insert(val);
  }
  return ret;
}

notes rote_db::list_notes(const sort& value) const {
  string condition;
  return search(condition, value);
}

string rote_db::_order_by(const sort& value) const {
  string ret;
  ret = "  ORDER BY ";
  switch (value) {
    case CREATION:
      ret += "CREATED";
      break;
    case CREATION_DESC:
      ret += "CREATED DESC";
      break;
    case MODIFIED:
      ret += "MODIFIED";
      break;
    case MODIFIED_DESC:
      ret += "MODIFIED DESC";
      break;
  }
  return ret;
}

notes rote_db::search(const string& condition, const sort& value) const {
  string sql;
  sql  = "SELECT *";
  sql += "  FROM notes";
  if (!condition.empty()) {
    sql += "  WHERE LOWER(note) LIKE LOWER(?)";
  }
  sql += _order_by(value);

  rubix::rows rows;
  if (!condition.empty()) {
    string_v conditions;
    conditions.push_back(condition);
    rows = _get_rows(sql, conditions);
  } else {
    rows = _get_rows(sql);
  }

  notes ret;
  for (rubix::rows::const_iterator it = rows.begin(); it != rows.end(); ++it) {
    const row& row = *it;
    ret.push_back(note(row));
  }
  return ret;
}

note rote_db::by_id(const uint32_t& note_id) const {
  string sql;
  sql = "SELECT * FROM notes WHERE note_id = ?";

  string_v conditions;
  conditions.push_back(_int_to_str(note_id));
  rubix::row row = _get_row(sql, conditions);

  return note(row);
}

notes rote_db::by_tag(const string& tag, const sort& value) const {
  string sql;
  sql  = "SELECT *";
  sql += "  FROM notes";
  sql += "  WHERE note_id IN (";
  sql += "    SELECT note_id";
  sql += "      FROM notes_tags";
  sql += "      WHERE LOWER(tag) = LOWER(?)";
  sql += "  )";
  sql += _order_by(value);

  rubix::rows rows;
  string_v conditions;
  conditions.push_back(tag);
  rows = _get_rows(sql, conditions);

  notes ret;
  for (rubix::rows::const_iterator it = rows.begin(); it != rows.end(); ++it) {
    const row& row = *it;
    ret.push_back(note(row));
  }
  return ret;
}

int rote_db::num_notes() const {
  return _get_int("SELECT COUNT(*) FROM notes");
}

void rote_db::delete_note(const note& value) const {
  if (!value.id()) {
    throw invalid_argument("can not delete note without note_id");
  }

  row conditions;
  conditions["note_id"] = _int_to_str(value.id());
  _delete("notes", conditions);
}

void rote_db::_begin() const {
  _exec("BEGIN");
}

void rote_db::_commit() const {
  _exec("COMMIT");
}

void rote_db::_rollback() const {
  _exec("ROLLBACK");
}
};

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
