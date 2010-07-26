// Copyright 2010 Rubix Consulting, Inc.

#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <string>
#include "./rote.h"
#include "../lib/note.h"
#include "../lib/rote_db.h"

using ::rubix::rote_db;
using ::rubix::note;
using ::std::cout;

int main(int argc, char **argv) {
  rote_db db(dbfile());

  cout << "number of notes: " << db.num_notes() << "\n\n";

  note test;
  //sleep(2);
  test.value("this is the title\nthis is the body of the first note");
  db.save_note(&test);

  cout << "test:\n";
  cout << "  id: " << test.id() << "\n";
  cout << "  created:  " << boost::posix_time::to_simple_string(test.created())  << "\n";
  cout << "  modified: " << boost::posix_time::to_simple_string(test.modified()) << "\n";
  cout << test.value() << "\n\n";

  cout << "number of notes: " << db.num_notes() << "\n";

  db.delete_note(&test);

  cout << "number of notes: " << db.num_notes() << "\n";

  return EXIT_SUCCESS;
}

std::string dbfile() {
  char *home = getenv("HOME");
  std::stringstream ss;
  ss << home << "/." << DB_NAME;
  return ss.str();
}

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
