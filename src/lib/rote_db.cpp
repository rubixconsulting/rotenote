// Copyright 2010 Rubix Consulting, Inc.

#include "./rote_db.h"
#include <stdexcept>
#include <sstream>
#include <string>

namespace rubix {
rote_db::rote_db(const std::string& dbfile) {
  _init(dbfile);
}

void rote_db::_init(const std::string& value) {
  __db = 0;

  if (value.empty()) {
    throw std::invalid_argument("missing database filename");
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

  throw std::runtime_error("could not open database: "+value);
}

rote_db::~rote_db() {
  sqlite3_close(__db);
}

void rote_db::_exec(const std::string& sql) const {
  char *err_msg   = 0;
  if (sqlite3_exec(__db, sql.c_str(), NULL, NULL, &err_msg) == SQLITE_OK) {
    return;
  }
  std::stringstream ss;
  ss << "SQL error: " << err_msg;
  sqlite3_free(err_msg);
  throw std::runtime_error(ss.str());
}

std::string rote_db::_get_val(const std::string& sql) const {
  const string_v vs;
  return _get_val(sql, vs);
}

std::string rote_db::_get_val(const std::string& sql, const string_v& vs) const {
  return _get_rows(sql, vs)[0].begin()->second;
}

int rote_db::_get_int(const std::string& sql) const {
  const string_v vs;
  return _get_int(sql, vs);
}

int rote_db::_get_int(const std::string& sql, const string_v& vs) const {
  return _str_to_int(_get_val(sql, vs));
}

row rote_db::_get_row(const std::string& sql) const {
  const string_v vs;
  return _get_row(sql, vs);
}

row rote_db::_get_row(const std::string& sql, const string_v& vs) const {
  return _get_rows(sql, vs)[0];
}

string_v rote_db::_get_col(const std::string& sql) const {
  const string_v vs;
  return _get_col(sql, vs);
}

string_v rote_db::_get_col(const std::string& sql, const string_v& vs) const {
  string_v ret;
  rubix::rows rows = _get_rows(sql, vs);
  for (rubix::rows::const_iterator it = rows.begin(); it != rows.end(); ++it) {
    const row& row = *it;
    ret.push_back(row.begin()->second);
  }
  return ret;
}

rows rote_db::_get_rows(const std::string& sql) const {
  const string_v vs;
  return _get_rows(sql, vs);
}

rows rote_db::_get_rows(const std::string& sql, const string_v& vs) const {
  return _exec_prepared(sql, vs);
}

std::string rote_db::_join(const string_v& s, const std::string& glue) const {
  std::string ret;
  for (string_v::size_type i = 0; i < s.size(); ++i) {
    ret += s[i];
    if (i+1 < s.size()) {
      ret += glue;
    }
  }
  return ret;
}

rows rote_db::_exec_prepared(const std::string& sql,
                             const string_v& vs) const {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(__db, sql.c_str(), -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    std::stringstream ss;
    ss << "could not prepare statement \""<< sql << "\" " << sqlite3_errmsg(__db) << " (" << rc << ")";
    throw std::runtime_error(ss.str());
  }

  for (string_v::size_type i = 0; i < vs.size(); ++i) {
    const std::string& val = vs[i];
    if (sqlite3_bind_text(stmt,
                          i+1,
                          val.c_str(),
                          -1,
                          SQLITE_TRANSIENT) != SQLITE_OK) {
      std::stringstream ss;
      ss << "could not bind text parameter: " << val;
      throw std::runtime_error(ss.str());
    }
  }

  rows ret;

  int num_columns = sqlite3_column_count(stmt);
  rc = sqlite3_step(stmt);
  while (rc == SQLITE_ROW) {
    rubix::row row;
    for (int i = 0; i < num_columns; ++i) {
      row[sqlite3_column_name(stmt, i)] = (const char*)sqlite3_column_text(stmt, i);
    }
    ret.push_back(row);
    rc = sqlite3_step(stmt);
  }

  if (rc != SQLITE_DONE) {
    std::stringstream ss;
    ss << "could not execute statement \""<< sql << "\" " << sqlite3_errmsg(__db) << " (" << rc << ")";
    throw std::runtime_error(ss.str());
  }

  return ret;
}

int rote_db::_insert(const std::string& table, const row& values) const {
  string_v cols, qs, vs;
  for (row::const_iterator it = values.begin(); it != values.end(); ++it) {
    const row_pair& pair = *it;
    cols.push_back(pair.first);
    qs.push_back("?");
    vs.push_back(pair.second);
  }

  std::string sql;
  sql  = "INSERT INTO "+table+" (";
  sql +=   _join(cols, ",");
  sql += ") VALUES(";
  sql +=   _join(qs, ",");
  sql += ")";

  _exec_prepared(sql, vs);

  return sqlite3_last_insert_rowid(__db);
}

void rote_db::_update(const std::string& table,
                      const row& values,
                      const row& conditions) const {
  std::string sql;
  string_v vs;

  sql  = "UPDATE "+table;
  sql += "  SET "+_make_qs(values, &vs);
  sql += "  WHERE "+_make_qs(conditions, &vs);

  _exec_prepared(sql, vs);
}

void rote_db::_delete(const std::string& table, const row& conditions) const {
  std::string sql;
  string_v vs;

  sql  = "DELETE FROM "+table;
  sql += "  WHERE "+_make_qs(conditions, &vs);

  _exec_prepared(sql, vs);
}

std::string rote_db::_make_qs(const row& values, string_v* vs) const {
  if (!vs) {
    throw std::runtime_error("invalid vs");
  }
  std::string ret;
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
  std::stringstream sql;

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
}

void rote_db::_upgrade_db() const {
  std::string sql;
  sql  = "SELECT MAX(version) AS version";
  sql += "  FROM schema_version";
  const int version = _get_int(sql);

  switch (version) {
    case 1:
      return;
    // NOTE: add upgrade cases here
    default:
      throw std::runtime_error("unknown schema version");
  }
}

std::string rote_db::_int_to_str(const int& value) const {
  std::stringstream ss;
  ss << value;
  return ss.str();
}

int rote_db::_str_to_int(const std::string& value) const {
  std::istringstream iss(value);
  int ret;
  if (iss >> ret) {
    return ret;
  }
  throw std::runtime_error("could not convert string to int: "+value);
}

const std::string& rote_db::_db_filename() const {
  return __db_filename;
}

const std::string& rote_db::_db_filename(const std::string& value) {
  __db_filename = value;
  return _db_filename();
}

int rote_db::save_note(note *value) const {
  if (!value) {
    throw std::invalid_argument("can not save NULL note");
  } else if (!value->id()) {
    return _insert_note(value);
  }
  return _update_note(value);
}

int rote_db::_insert_note(note *value) const {
  row values;
  values["note"]     = value->value();
  values["created"]  = boost::posix_time::to_simple_string(value->created());
  values["modified"] = boost::posix_time::to_simple_string(value->modified());
  const int id = _insert("notes", values);
  value->id(id);
  return _save_tags(value);
}

int rote_db::_update_note(const note *value) const {
  row values, conditions;
  values["note"]     = value->value();
  values["modified"] = boost::posix_time::to_simple_string(value->modified());
  conditions["note_id"] = value->id();
  _update("notes", values, conditions);
  return _save_tags(value);
}

int rote_db::_save_tags(const note *value) const {
  row conditions;
  conditions["note_id"] = _int_to_str(value->id());
  _delete("notes_tags", conditions);

  std::string sql;
  sql = "DELETE FROM tags WHERE tag NOT IN (SELECT tag FROM notes_tags)";
  _exec(sql);

  tags t = value->tags();
  for (tags::const_iterator it = t.begin(); it != t.end(); ++it) {
    const std::string& tag = *it;
    _save_tag(tag, value);
  }
  return value->id();
}

void rote_db::_save_tag(const std::string& tag, const note *value) const {
  std::string sql;
  string_v conditions;

  sql  = "INSERT INTO tags (tag) VALUES(?)";
  conditions.push_back(tag);
  sql += "  WHERE NOT EXISTS(";
  sql += "    SELECT * FROM tags WHERE tag = ?";
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
    const std::string& val = *it;
    ret.insert(val);
  }
  return ret;
}

notes rote_db::list_notes(const sort& value) const {
  string_v conditions;
  return search(conditions, value);
}

std::string rote_db::_order_by(const sort& value) const {
  std::string ret;
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

notes rote_db::search(const string_v& conditions, const sort& value) const {
  std::string sql;
  sql  = "SELECT *";
  sql += "  FROM note";
  if (!conditions.empty()) {
    sql += "  WHERE LOWER(note) LIKE LOWER(?)";
  }
  sql += _order_by(value);

  rubix::rows rows;
  if (!conditions.empty()) {
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

notes rote_db::by_tag(const std::string& tag, const sort& value) const {
  std::string sql;
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

void rote_db::delete_note(note* value) const {
  if (!value) {
    throw std::invalid_argument("can not delete NULL note");
  } else if (!value->id()) {
    throw std::invalid_argument("can not delete note without note_id");
  }

  row conditions;
  conditions["note_id"] = _int_to_str(value->id());
  _delete("notes", conditions);
  value->clear();
}
};

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
