/**
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include "Bruinbase.h"
#include "SqlEngine.h"
#include "BTreeIndex.h"

using namespace std;

// external functions and variables for load file and sql command parsing 
extern FILE* sqlin;
int sqlparse(void);


RC SqlEngine::run(FILE* commandline)
{
  fprintf(stdout, "Bruinbase> ");

  // set the command line input and start parsing user input
  sqlin = commandline;
  sqlparse();  // sqlparse() is defined in SqlParser.tab.c generated from
               // SqlParser.y by bison (bison is GNU equivalent of yacc)

  return 0;
}

RC SqlEngine::select(int attr, const string& table, const vector<SelCond>& cond)
{
  RecordFile rf;   // RecordFile containing the table
  RecordId   rid;  // record cursor for table scanning

  RC     rc;
  int    key;     
  string value;
  int    count;
  int    diff;

  // open the table file
  if ((rc = rf.open(table + ".tbl", 'r')) < 0) {
    fprintf(stderr, "Error: table %s does not exist\n", table.c_str());
    return rc;
  }

  bool using_index = false; // flag for index searching
  // open index file, if it exists
  for (int i = 0; i < cond.size(); ++i) {
    if (cond[i].attr == 1 && cond[i].comp != SelCond::NE) {
      // we can use the index
      BTreeIndex bti;
      if (rc = bti.open(table + ".idx", 'r')) { // error opening
        // error opening
        fprintf(stderr, "Error opening index file\n");
      } else { // open successful
        fprintf(stderr, "open index file successful\n");
        using_index = true;
      }
      break;
    }
  }

  // if we're using index, then get key range from conditions
  int startkey = -1, endkey = -1, condval;
  if (using_index) {
    for (int i = 0; i < cond.size(); ++i) {
      // skip conditions not on key
      if (cond[i].attr != 1)
        continue;

      condval = atoi(cond[i].value);
      switch (cond[i].comp) {
      case SelCond::EQ:
        startkey = endkey = condval;
        break;
      case SelCond::GT: // >n is equiv to >=n+1
        condval++;
      case SelCond::GE:
        startkey = condval > startkey ? condval : startkey;
        break;
      case SelCond::LT: // <n is equiv to <=n-1
        condval--;
      case SelCond::LE:
        startkey = condval < startkey ? condval : startkey;
        break;
      }
    }
  }
  if (startkey == -1)
    startkey = 0;

  // scan the table file from the beginning
  int k; // key traversal
  rid.pid = rid.sid = 0; // rid traversal
  count = 0;
  while (1) {
    // 0. check exit conditions
    if (!(rid < rf.endRid()))
      break;
    if (!(k <= endkey))
      break;

    // 1. TODO: fetch tuple, by key or by rid depending on `using_index`


    // read the tuple
    // TODO: only read tuple if we need to
    if ((rc = rf.read(rid, key, value)) < 0) {
      fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
      goto exit_select;
    }

    // 2. check the conditions on the tuple
    for (unsigned i = 0; i < cond.size(); i++) {
      // compute the difference between the tuple value and the condition value
      switch (cond[i].attr) {
      case 1:
        diff = key - atoi(cond[i].value);
        break;
      case 2:
        diff = strcmp(value.c_str(), cond[i].value);
        break;
      }

      // skip the tuple if any condition is not met
      switch (cond[i].comp) {
      case SelCond::EQ:
        if (diff != 0) goto next_tuple;
        break;
      case SelCond::NE:
        if (diff == 0) goto next_tuple;
        break;
      case SelCond::GT:
        if (diff <= 0) goto next_tuple;
        break;
      case SelCond::LT:
        if (diff >= 0) goto next_tuple;
          break;
      case SelCond::GE:
        if (diff < 0) goto next_tuple;
        break;
      case SelCond::LE:
        if (diff > 0) goto next_tuple;
        break;
      }
    }

    // the condition is met for the tuple. 

    // 3. increase matching tuple counter
    count++;

    // 4. print the tuple
    switch (attr) {
    case 1:  // SELECT key
      fprintf(stdout, "%d\n", key);
      break;
    case 2:  // SELECT value
      fprintf(stdout, "%s\n", value.c_str());
      break;
    case 3:  // SELECT *
      fprintf(stdout, "%d '%s'\n", key, value.c_str());
      break;
    }

    // 5. move to the next tuple
next_tuple:
    ++rid;
    ++key;
  }

  // print matching tuple count if "select count(*)"
  if (attr == 4) {
    fprintf(stdout, "%d\n", count);
  }
  rc = 0;

  // close the table file and return
exit_select:
  rf.close();
  return rc;
}

RC SqlEngine::load(const string& table, const string& loadfile, bool index)
{
  RC ret;
  RecordId rid;
  int k;
  string v;

  // open table file
  RecordFile rf;
  if (ret = rf.open(table + ".tbl", 'w')) { // error
    fprintf(stderr, "rf.open() failed to open\n");
    return RC_FILE_OPEN_FAILED;
  }

  // open loadfile
  ifstream ifs;
  ifs.open(loadfile.c_str());
  if (!ifs.is_open()) { // error
    fprintf(stderr, "ifs failed to open %s\n", loadfile.c_str());
    return RC_FILE_OPEN_FAILED;
  }

  BTreeIndex bti;
  if (index) { // build index
    if (ret = bti.open(table + ".idx", 'w')) { // error
      fprintf(stderr, "bti.open() failed to open index file\n");
      return ret;
    }
  }

  string line;
  // read lines
  while (getline(ifs, line)) {

    // parse for key-value pair
    if (ret = SqlEngine::parseLoadLine(line, k, v)) { // parsing failed
      // parse failed
      cout << ret;
      fprintf(stderr, "parseLoadLine returned nonzero\n");
      return ret;
    }

    // append key-value pair to rf
    if (ret = rf.append(k, v, rid)) { // append failed
      fprintf(stderr, "append returned nonzero\n");
      return ret;
    }

    if (index) {
      // insert rid into index
      if (ret = bti.insert(k, rid)) { // insert failed
        fprintf(stderr, "bit.insert returned nonzero\n");
        return ret;
      }
    }
  }

  fprintf(stderr, "load successful\n");
  return 0;
}

RC SqlEngine::parseLoadLine(const string& line, int& key, string& value)
{
  const char *s;
  char        c;
  string::size_type loc;

  // ignore beginning white spaces
  c = *(s = line.c_str());
  while (c == ' ' || c == '\t') { c = *++s; }

  // get the integer key value
  key = atoi(s);

  // look for comma
  s = strchr(s, ',');
  if (s == NULL) { return RC_INVALID_FILE_FORMAT; }

  // ignore white spaces
  do { c = *++s; } while (c == ' ' || c == '\t');

  // if there is nothing left, set the value to empty string
  if (c == 0) {
    value.erase();
    return 0;
  }

  // is the value field delimited by ' or "?
  if (c == '\'' || c == '"') {
    s++;
  } else {
    c = '\n';
  }

  // get the value string
  value.assign(s);
  loc = value.find(c, 0);
  if (loc != string::npos) { value.erase(loc); }

  return 0;
}
