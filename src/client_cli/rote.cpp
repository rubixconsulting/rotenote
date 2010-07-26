// Copyright 2010 Rubix Consulting, Inc.

#include <stdlib.h>
#include <sstream>
#include <string>
#include "./rote.h"
#include "../lib/rote_db.h"

using ::rubix::rote_db;

int main(int argc, char **argv) {
  rote_db db(dbfile());

  return EXIT_SUCCESS;
}

std::string dbfile() {
  char *home = getenv("HOME");
  std::stringstream ss;
  ss << home << "/." << DB_NAME;
  return ss.str();
}

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
