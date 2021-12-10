// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include <algorithm>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include "CsvWriter.h"
#include "dtoa_milo.h"

using ad::util::CsvWriter;

// cached first 10 powers of 10
static int pow10cache[10] = {
    1,           10,          100,           1000,          10000,
    100000,      1000000,     10000000,      100000000,     1000000000};

// _____________________________________________________________________________
CsvWriter::CsvWriter(std::ostream* str, const HeaderList& headers)
    : _stream(str),
      _headers(headers),
      _hWritten(false),
      _first(true),
      _delim(',') {}

// _____________________________________________________________________________
int CsvWriter::pow10(int i) const {
  if (i < 10) return pow10cache[i];
  return pow(10, i);
}

// _____________________________________________________________________________
void CsvWriter::writeDouble(double d, size_t digits) {
  double p = pow10(digits);
  d = std::round(d * p) / p;
  writeDouble(d);
}

// _____________________________________________________________________________
void CsvWriter::writeDouble(double d) {
  ad::util::dtoa_milo(d, _dblBuf);
  if (!_first) _stream->write(&_delim, 1);
  _first = false;
  *_stream <<_dblBuf;
}

// _____________________________________________________________________________
void CsvWriter::skip() {
  if (!_first) _stream->write(&_delim, 1);
  _first = false;
}

// _____________________________________________________________________________
void CsvWriter::writeString(const std::string& str) {
  if (!_first) _stream->write(&_delim, 1);
  _first = false;

  if (str.find(_delim) != std::string::npos) {
    if (str.find('"') != std::string::npos) {
      (*_stream) << "\"" << escStr(str) << "\"";
    } else {
      (*_stream) << "\"" << str << "\"";
    }
  } else {
    _stream->write(str.c_str(), str.size());
  }
}

// _____________________________________________________________________________
std::string CsvWriter::escStr(const std::string& str) const {
  std::stringstream ret;
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '\"')
      ret << "\"\"";
    else
      ret << str[i];
  }

  return ret.str();
}

// _____________________________________________________________________________
void CsvWriter::writeRawString(const std::string& str) {
  if (!_first) _stream->write(&_delim, 1);
  _first = false;
  _stream->write(str.c_str(), str.size());
}

// _____________________________________________________________________________
void CsvWriter::writeInt(int i) {
  if (!_first) _stream->write(&_delim, 1);
  _first = false;
  if (i == 0) _stream->write("0", 1);
  else if (i == 1) _stream->write("1", 1);
  else if (i == 2) _stream->write("2", 1);
  else (*_stream) << i;
}

// _____________________________________________________________________________
void CsvWriter::flushLine() {
  if (!_hWritten) writeHeader();
  (*_stream) << "\n";
  _first = true;
}

// _____________________________________________________________________________
void CsvWriter::writeHeader() {
  _hWritten = true;
  writeStrArr(_headers);
}

// _____________________________________________________________________________
void CsvWriter::writeStrArr(const std::vector<std::string>& arr) {
  if (!arr.size()) return;
  bool first = false;
  for (const auto& str : arr) {
    if (first) (*_stream) << _delim;
    first = true;
    (*_stream) << str.c_str();
  }
}
