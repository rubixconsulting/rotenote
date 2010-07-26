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
  return _get_rows(sql)[0].begin()->second;
}

int rote_db::_get_int(const std::string& sql) const {
  return _str_to_int(_get_val(sql));
}

rubix::row rote_db::_get_row(const std::string& sql) const {
  return _get_rows(sql)[0];
}

rubix::string_v rote_db::_get_col(const std::string& sql) const {
  string_v ret;
  rubix::rows rows = _get_rows(sql);
  for (rubix::rows::const_iterator it = rows.begin(); it != rows.end(); ++it) {
    const row& row = *it;
    ret.push_back(row.begin()->second);
  }
  return ret;
}

rubix::rows rote_db::_get_rows(const std::string& sql) const {
  char **result;
  int num_rows    = 0;
  int num_columns = 0;
  char *err_msg   = 0;
  rows results;

  if (sqlite3_get_table(__db,
                        sql.c_str(),
                        &result,
                        &num_rows,
                        &num_columns,
                        &err_msg) != SQLITE_OK) {
    std::stringstream ss;
    ss << "SQL error: " << err_msg;
    sqlite3_free(err_msg);
    sqlite3_free_table(result);
    throw std::runtime_error(ss.str());
  }

  for (int i = 0; i < num_rows; ++i) {
    row row;
    for (int j = 0; j < num_columns; ++j) {
      char* value = result[num_columns+(num_columns*i)+j];
      if (!value) {
        throw std::runtime_error("invalid index");
      }
      row[result[j]] = value;
    }
    results.push_back(row);
  }

  sqlite3_free(err_msg);
  sqlite3_free_table(result);

  return results;
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

std::string rote_db::_exec_prepared(const std::string& sql,
                             const string_v& vs) const {
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(__db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
    std::stringstream ss;
    ss << "could not prepare statement: " << sql;
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

  std::string ret;

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    ret = (const char*)sqlite3_column_text(stmt, 0);
  } else if (sqlite3_step(stmt) != SQLITE_DONE) {
      throw std::runtime_error("could not execute statement");
  }

  return ret;
}

void rote_db::_insert(const std::string& table, const row& values) const {
  _insert(table, values, "");
}

std::string rote_db::_insert(const std::string& table,
                      const row& values,
                      const std::string& ret) const {
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

  if (!ret.empty()) {
    sql += " RETURNING "+ret;
  }

  return _exec_prepared(sql, vs);
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
  std::stringstream schema_version;
  schema_version << SCHEMA_VERSION;
  values["version"] = schema_version.str();
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
  values["created"]  = boost::gregorian::to_simple_string(value->created());
  values["modified"] = boost::gregorian::to_simple_string(value->modified());
  const int id = _str_to_int(_insert("notes", values, "note_id"));
  value->id(id);
  return id;
}

int rote_db::_update_note(const note *value) const {
  row values, conditions;
  values["note"]     = value->value();
  values["modified"] = boost::gregorian::to_simple_string(value->modified());
  conditions["note_id"] = value->id();
  _update("notes", values, conditions);
  return value->id();
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

notes rote_db::list_notes() const {
  return list_notes(CREATION_DESC);
}

notes rote_db::list_notes(const sort& value) const {
  return search("", value);
}

notes rote_db::search(const std::string& condition, const sort& value) const {
  std::string sql;
  sql  = "SELECT *";
  sql += "  FROM note";
  if (!condition.empty()) {
    sql += "  WHERE LOWER(note) LIKE ?";
  }
  sql += "  ORDER BY ";
  switch (value) {
    case CREATION:
      sql += "CREATED";
      break;
    case CREATION_DESC:
      sql += "CREATED DESC";
      break;
    case MODIFIED:
      sql += "MODIFIED";
      break;
    case MODIFIED_DESC:
      sql += "MODIFIED DESC";
      break;
  }
  rubix::rows rows;
  if (condition.empty()) {
    rows = _get_rows(sql);
  } else {
    // TODO(jrubin)
    // rows = _get_rows(sql, conditions);
  }
  notes ret;
  for (rubix::rows::const_iterator it = rows.begin(); it != rows.end(); ++it) {
    const row& row = *it;
    ret.push_back(note(row));
  }
  return ret;
}

notes rote_db::by_tag(const std::string& value) const {
  notes ret;
  // TODO(jrubin)
  return ret;
}
};

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
