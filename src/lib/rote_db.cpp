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

void rote_db::_exec(const std::string& sql) {
  char *err_msg   = 0;
  if (sqlite3_exec(__db, sql.c_str(), NULL, NULL, &err_msg) == SQLITE_OK) {
    return;
  }
  std::stringstream ss;
  ss << "SQL error: " << err_msg;
  sqlite3_free(err_msg);
  throw std::runtime_error(ss.str());
}

std::string rote_db::_get_val(const std::string& sql) {
  return _get_rows(sql)[0].begin()->second;
}

int rote_db::_get_int(const std::string& sql) {
  return _str_to_int(_get_val(sql));
}

rote_db::_row_ rote_db::_get_row(const std::string& sql) {
  return _get_rows(sql)[0];
}

rote_db::_rows_ rote_db::_get_rows(const std::string& sql) {
  char **result;
  int num_rows    = 0;
  int num_columns = 0;
  char *err_msg   = 0;
  _rows_ results;

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
    _row_ row;
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

std::string rote_db::_join(const _string_v_& s, const std::string& glue) {
  std::string ret;
  for (_string_v_::size_type i = 0; i < s.size(); ++i) {
    ret += s[i];
    if (i+1 < s.size()) {
      ret += glue;
    }
  }
  return ret;
}

void rote_db::_exec_prepared(const std::string& sql, const _string_v_& vs) {
  sqlite3_stmt *stmt;
  if (sqlite3_prepare(__db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
    std::stringstream ss;
    ss << "could not prepare statement: " << sql;
    throw std::runtime_error(ss.str());
  }

  for (_string_v_::size_type i = 0; i < vs.size(); ++i) {
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

  if (sqlite3_step(stmt) != SQLITE_DONE) {
      throw std::runtime_error("could not execute statement");
  }
}

void rote_db::_insert(const std::string& table, const _row_& values) {
  _string_v_ cols, qs, vs;
  for (_row_::const_iterator it = values.begin(); it != values.end(); ++it) {
    const _row_pair_& pair = *it;
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
}

void rote_db::_update(const std::string& table,
                      const _row_& values,
                      const _row_& conditions) {
  std::string sql;
  _string_v_ vs;

  sql  = "UPDATE "+table;
  sql += "  SET "+_make_qs(values, &vs);
  sql += "  WHERE "+_make_qs(conditions, &vs);

  _exec_prepared(sql, vs);
}

void rote_db::_delete(const std::string& table, const _row_& conditions) {
  std::string sql;
  _string_v_ vs;

  sql  = "DELETE FROM "+table;
  sql += "  WHERE "+_make_qs(conditions, &vs);

  _exec_prepared(sql, vs);
}

std::string rote_db::_make_qs(const _row_& values, _string_v_* vs) {
  if (!vs) {
    throw std::runtime_error("invalid vs");
  }
  std::string ret;
  _row_::size_type i = 0;
  for (_row_::const_iterator it = values.begin(); it != values.end(); ++it) {
    const _row_pair_& pair = *it;
    ret += pair.first + " = ?";
    vs->push_back(pair.second);
    if (i+1 < values.size()) {
      ret += ", ";
    }
    ++i;
  }
  return ret;
}

void rote_db::_init_db() {
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

  _row_ values;
  std::stringstream schema_version;
  schema_version << SCHEMA_VERSION;
  values["version"] = schema_version.str();
  _insert("schema_version", values);
}

void rote_db::_upgrade_db() {
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

int rote_db::_str_to_int(const std::string& value) {
  std::istringstream iss(value);
  int ret;
  if (iss >> ret) {
    return ret;
  }
  throw std::runtime_error("could not convert string to int: "+value);
}

const std::string& rote_db::_db_filename() {
  return __db_filename;
}

const std::string& rote_db::_db_filename(const std::string& value) {
  __db_filename = value;
  return _db_filename();
}
};

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
