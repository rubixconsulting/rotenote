// Copyright 2010 Rubix Consulting, Inc.

#ifndef SRC_LIB_TYPES_H_
#define SRC_LIB_TYPES_H_

#include <string>
#include <vector>
#include <set>
#include <map>

namespace rubix {
#define SCHEMA_VERSION 1

typedef std::set<std::string> tags;
typedef std::map <const std::string, std::string> row;
typedef std::pair<const std::string, std::string> row_pair;
typedef std::vector<row>                          rows;
typedef std::vector<std::string>                  string_v;

enum sort {
  CREATION,
  CREATION_DESC,
  MODIFIED,
  MODIFIED_DESC
};
};

#endif  // SRC_LIB_TYPES_H_

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
