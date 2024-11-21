#include <iostream>

enum ColumnDataType { TEXT, LONG, DOUBLE };

/////////////////////

class BaseColumnWriter {
private:
  ColumnDataType mColumnType;
  bool mDictEnabled;

public:
  BaseColumnWriter(ColumnDataType columnType): mColumnType(columnType) { }
  virtual ~BaseColumnWriter() {}
};

template<ColumnDataType T, bool dict>
class ColumnWriter;

template<>
class ColumnWriter<LONG, false>: public BaseColumnWriter {
private:
  ColumnDataType mColumnType;

public:
  void Write(const long value, const int row) { /* TODO */ }
};

template<>
class ColumnWriter<DOUBLE, false>: public BaseColumnWriter {
private:
  ColumnDataType mColumnType;

public:
  void Write(const long double, const int row) { /* TODO */}
};

class BaseTextColumnWriter: public BaseColumnWriter { };

template<>
class ColumnWriter<TEXT, false>: public BaseTextColumnWriter {
private:
  ColumnDataType mColumnType;

public:
  void Write(const std::string & value, const int row) { /* TODO */}
  void Write(const char* value, const int row) { /* TODO */}
};

template<>
class ColumnWriter<TEXT, true>: public BaseTextColumnWriter {
private:
  ColumnDataType mColumnType;

public:
  int PutDictinaryKey(const std::string & key) { /* TODO */ return 0; }
  int PutDictinaryKey(const char* key) { /* TODO */ return 0; }
  void PutDictRowData(int offset, int row) { /* TODO */}
};

////////////////////////////////////////////////
class DocsIterator;

class BaseDocValueSegReader {
private:
  ColumnDataType mColumnType;
  bool mDictEnabled;

public:
  BaseDocValueSegReader(ColumnDataType columnType): mColumnType(columnType) { }
  virtual ~BaseDocValueSegReader() {}
};

template<ColumnDataType T, bool dict>
class DocValueSegReader;

template<>
class DocValueSegReader<LONG, false>: public BaseDocValueSegReader {
public:
  template<ColumnDataType _T, bool _D>
  void GetByDocIds(ColumnWriter<_T, _D>* writer, DocsIterator& docit);

  template<>
  void GetByDocIds<TEXT, true>(ColumnWriter<TEXT, true>* writer, DocsIterator& docit);

  template<>
  void GetByDocIds<TEXT, false>(ColumnWriter<TEXT, false>* writer, DocsIterator& docit);
};

template<>
class DocValueSegReader<DOUBLE, false>: public BaseDocValueSegReader {
public:
  template<ColumnDataType _T, bool _D>
  void GetByDocIds(ColumnWriter<_T, _D>* writer, DocsIterator& docit);
};

class BaseTextDocValueSegReader: public BaseDocValueSegReader { };

template<>
class DocValueSegReader<TEXT, false>: public BaseTextDocValueSegReader {
public:
  template<ColumnDataType _T, bool _D>
  void GetByDocIds(ColumnWriter<_T, _D>* writer, DocsIterator& docit);
};

template<>
class DocValueSegReader<TEXT, true>: public BaseTextDocValueSegReader {
public:
  template<ColumnDataType _T, bool _D>
  void GetByDocIds(ColumnWriter<_T, _D>* writer, DocsIterator& docit);
};

int main() {
  return 0;
}