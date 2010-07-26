// Copyright 2010 Rubix Consulting, Inc.

#include "./rote_db.h"
#include <stdlib.h>
#include <stdexcept>
#include <sstream>
#include <string>

namespace rubix {
rote_db::rote_db() {
  if (!sqlite3_open_v2(_db_filename().c_str(),
                       &__db,
                       SQLITE_OPEN_READWRITE,
                       NULL)) {
    _upgrade_db();
    return;
  } else if (!sqlite3_open_v2(_db_filename().c_str(),
                              &__db,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                              NULL)) {
    _init_db();
    return;
  }

  throw std::runtime_error("could not open database: "+_db_filename());
}

rote_db::~rote_db() {
  sqlite3_close(__db);
}

rote_db::_rows_ rote_db::_exec(const std::string& sql) {
  char **result;
  int num_rows    = 0;
  int num_columns = 0;
  char *err_msg   = 0;
  _rows_ results;

  int rc = sqlite3_get_table(__db,
                             sql.c_str(),
                             &result,
                             &num_rows,
                             &num_columns,
                             &err_msg);

  if (rc != SQLITE_OK) {
    std::stringstream ss;
    ss << "SQL error: " << err_msg;
    sqlite3_free(err_msg);
    sqlite3_free_table(result);
    throw std::runtime_error(ss.str());
  }

  for (int i = 0; i < num_rows; ++i) {
    _row_ row;
    for (int j = 0; j < num_columns; ++j) {
      row[result[j]] = result[num_columns+(num_columns*i)+j];
    }
    results.push_back(row);
  }

  sqlite3_free(err_msg);
  sqlite3_free_table(result);

  return results;
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

  sql.str("");
  sql << "INSERT INTO schema_version (version) VALUES(";
  sql <<   SCHEMA_VERSION;
  sql << ")";
  _exec(sql.str());
}

void rote_db::_upgrade_db() {
  _rows_ results = _exec("SELECT MAX(version) AS version FROM schema_version");

  const int version = _str_to_int(results[0]["version"]);

  switch (version) {
    case 1:
      return;
    // NOTE: add upgrade cases here
    default:
      return;
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
  if (!__db_filename.empty()) {
    return __db_filename;
  }
  char *home = getenv("HOME");
  std::stringstream ss;
  ss << home << "/." << DB_NAME;
  __db_filename = ss.str();
  return __db_filename;
}
};

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
