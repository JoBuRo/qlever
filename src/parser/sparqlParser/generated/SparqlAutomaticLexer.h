
// Generated from SparqlAutomatic.g4 by ANTLR 4.9.2

#pragma once


#include "antlr4-runtime.h"




class  SparqlAutomaticLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, T__18 = 19, T__19 = 20, 
    T__20 = 21, T__21 = 22, T__22 = 23, T__23 = 24, T__24 = 25, T__25 = 26, 
    T__26 = 27, T__27 = 28, T__28 = 29, T__29 = 30, T__30 = 31, BASE = 32, 
    PREFIX = 33, SELECT = 34, DISTINCT = 35, REDUCED = 36, AS = 37, CONSTRUCT = 38, 
    WHERE = 39, DESCRIBE = 40, ASK = 41, FROM = 42, NAMED = 43, GROUPBY = 44, 
    GROUP_CONCAT = 45, HAVING = 46, ORDERBY = 47, ASC = 48, DESC = 49, LIMIT = 50, 
    OFFSET = 51, VALUES = 52, LOAD = 53, SILENT = 54, CLEAR = 55, DROP = 56, 
    CREATE = 57, ADD = 58, DATA = 59, MOVE = 60, COPY = 61, INSERT = 62, 
    DELETE = 63, WITH = 64, USING = 65, DEFAULT = 66, GRAPH = 67, ALL = 68, 
    OPTIONAL = 69, SERVICE = 70, BIND = 71, UNDEF = 72, MINUS = 73, UNION = 74, 
    FILTER = 75, NOT = 76, IN = 77, STR = 78, LANG = 79, LANGMATCHES = 80, 
    DATATYPE = 81, BOUND = 82, IRI = 83, URI = 84, BNODE = 85, RAND = 86, 
    ABS = 87, CEIL = 88, FLOOR = 89, ROUND = 90, CONCAT = 91, STRLEN = 92, 
    UCASE = 93, LCASE = 94, ENCODE = 95, FOR = 96, CONTAINS = 97, SQR = 98, 
    STRSTARTS = 99, STRENDS = 100, STRBEFORE = 101, STRAFTER = 102, YEAR = 103, 
    MONTH = 104, DAY = 105, HOURS = 106, MINUTES = 107, SECONDS = 108, TIMEZONE = 109, 
    TZ = 110, NOW = 111, UUID = 112, STRUUID = 113, SHA1 = 114, SHA256 = 115, 
    SHA384 = 116, SHA512 = 117, MD5 = 118, COALESCE = 119, IF = 120, STRLANG = 121, 
    STRDT = 122, SAMETERM = 123, ISIRI = 124, ISURI = 125, ISBLANK = 126, 
    ISLITERAL = 127, ISNUMERIC = 128, REGEX = 129, SUBSTR = 130, REPLACE = 131, 
    EXISTS = 132, COUNT = 133, SUM = 134, MIN = 135, MAX = 136, AVG = 137, 
    SAMPLE = 138, SEPARATOR = 139, IRI_REF = 140, PNAME_NS = 141, PNAME_LN = 142, 
    BLANK_NODE_LABEL = 143, VAR1 = 144, VAR2 = 145, LANGTAG = 146, INTEGER = 147, 
    DECIMAL = 148, DOUBLE = 149, INTEGER_POSITIVE = 150, DECIMAL_POSITIVE = 151, 
    DOUBLE_POSITIVE = 152, INTEGER_NEGATIVE = 153, DECIMAL_NEGATIVE = 154, 
    DOUBLE_NEGATIVE = 155, EXPONENT = 156, STRING_LITERAL1 = 157, STRING_LITERAL2 = 158, 
    STRING_LITERAL_LONG1 = 159, STRING_LITERAL_LONG2 = 160, ECHAR = 161, 
    NIL = 162, ANON = 163, PN_CHARS_U = 164, VARNAME = 165, PN_PREFIX = 166, 
    PN_LOCAL = 167, PLX = 168, PERCENT = 169, HEX = 170, PN_LOCAL_ESC = 171, 
    WS = 172, COMMENTS = 173
  };

  explicit SparqlAutomaticLexer(antlr4::CharStream *input);
  ~SparqlAutomaticLexer();

  virtual std::string getGrammarFileName() const override;
  virtual const std::vector<std::string>& getRuleNames() const override;

  virtual const std::vector<std::string>& getChannelNames() const override;
  virtual const std::vector<std::string>& getModeNames() const override;
  virtual const std::vector<std::string>& getTokenNames() const override; // deprecated, use vocabulary instead
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;

  virtual const std::vector<uint16_t> getSerializedATN() const override;
  virtual const antlr4::atn::ATN& getATN() const override;

private:
  static std::vector<antlr4::dfa::DFA> _decisionToDFA;
  static antlr4::atn::PredictionContextCache _sharedContextCache;
  static std::vector<std::string> _ruleNames;
  static std::vector<std::string> _tokenNames;
  static std::vector<std::string> _channelNames;
  static std::vector<std::string> _modeNames;

  static std::vector<std::string> _literalNames;
  static std::vector<std::string> _symbolicNames;
  static antlr4::dfa::Vocabulary _vocabulary;
  static antlr4::atn::ATN _atn;
  static std::vector<uint16_t> _serializedATN;


  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

  struct Initializer {
    Initializer();
  };
  static Initializer _init;
};

