// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Johannes Kalmbach(joka921) <johannes.kalmbach@gmail.com>

#include <getopt.h>
#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include "./parser/NTriplesParser.h"
#include "./parser/TsvParser.h"
#include "./parser/TurtleParser.h"
#include "./util/Log.h"

/**
 * @brief Instantiate a Parser that parses filename and writes the resulting
 * triples to argument out.
 *
 * @tparam Parser A Parser that supports a call to getline that yields a triple
 * @param out the parsed triples are written to this file
 * @param filename the filename from which the triples are parsed, can be
 * "/dev/stdin"
 */
template <class Parser>
void writeNTImpl(std::ostream& out, const std::string& filename) {
  Parser p(filename);
  std::array<std::string, 3> triple;
  // this call by reference is necesary because of the TSV-Parsers interface
  while (p.getLine(triple)) {
    out << triple[0] << " " << triple[1] << " " << triple[2] << " .\n";
  }
}

/**
 * @brief Decide according to arg fileFormat which parser to use.
 * Then call writeNTImpl with the appropriate parser
 * @param out Parsed triples will be written here.
 * @param fileFormat One of [tsv|ttl|tsv|mmap]
 * @param filename Will read from this file, might be /dev/stdin
 */
void writeNT(std::ostream& out, const string& fileFormat,
             const std::string& filename) {
  if (fileFormat == "ttl") {
    writeNTImpl<TurtleStreamParser>(out, filename);
  } else if (fileFormat == "tsv") {
    writeNTImpl<TsvParser>(out, filename);
  } else if (fileFormat == "nt") {
    writeNTImpl<NTriplesParser>(out, filename);
  } else if (fileFormat == "mmap") {
    writeNTImpl<TurtleMmapParser>(out, filename);
  } else {
    LOG(ERROR) << "writeNT was called with unknown file format " << fileFormat
               << ". This should never happen, terminating" << std::endl;
    LOG(ERROR) << "Please specify a valid file format" << std::endl;
    exit(1);
  }
}

// _______________________________________________________________________________________________________________
void printUsage(char* execName) {
  std::ios coutState(nullptr);
  coutState.copyfmt(cout);
  cout << std::setfill(' ') << std::left;

  cout << "Usage: " << execName << " -i <index> [OPTIONS]" << endl << endl;
  cout << "Options" << endl;
  cout << "  " << std::setw(20) << "F, file-format" << std::setw(1) << "    "
       << " Specify format of the input file. Must be one of "
          "[tsv|nt|ttl|mmap]."
       << " " << std::setw(36)
       << "If not set, we will try to deduce from the filename" << endl
       << " " << std::setw(36)
       << "(mmap assumes an on-disk turtle file that can be mmapped to memory)"
       << endl;
  cout << "  " << std::setw(20) << "i, input-file" << std::setw(1) << "    "
       << " The file to be parsed from. If omitted, we will read from stdin"
       << endl;
  cout << "  " << std::setw(20) << "o, output-file" << std::setw(1) << "    "
       << " The NTriples file to be Written to. If omitted, we will write to "
          "stdout"
       << endl;
  cout.copyfmt(coutState);
}

// ________________________________________________________________________
int main(int argc, char** argv) {
  // we possibly write to stdout to pipe it somewhere else, so redirect all
  // logging output to std::err
  ad_utility::setGlobalLogginStream(&std::cerr);
  struct option options[] = {{"help", no_argument, NULL, 'h'},
                             {"file-format", required_argument, NULL, 'F'},
                             {"input-file", required_argument, NULL, 'i'},
                             {"output-file", required_argument, NULL, 'o'},
                             {NULL, 0, NULL, 0}};

  string inputFile, outputFile, fileFormat;
  while (true) {
    int c = getopt_long(argc, argv, "F:i:o:h", options, nullptr);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'h':
        printUsage(argv[0]);
        return 0;
        break;
      case 'i':
        inputFile = optarg;
        break;
      case 'o':
        outputFile = optarg;
        break;
      case 'F':
        fileFormat = optarg;
        break;
      default:
        cout << endl
             << "! ERROR in processing options (getopt returned '" << c
             << "' = 0x" << std::setbase(16) << c << ")" << endl
             << endl;
        printUsage(argv[0]);
        exit(1);
    }
  }

  if (fileFormat.empty()) {
    bool filetypeDeduced = false;
    if (ad_utility::endsWith(inputFile, ".tsv")) {
      fileFormat = "tsv";
      filetypeDeduced = true;
    } else if (ad_utility::endsWith(inputFile, ".nt")) {
      fileFormat = "nt";
      filetypeDeduced = true;
    } else if (ad_utility::endsWith(inputFile, ".ttl")) {
      fileFormat = "ttl";
      filetypeDeduced = true;
    } else {
      LOG(WARN)
          << " Could not deduce the type of the input knowledge-base-file by "
             "its extension. Assuming the input to be turtle. Please specify "
             "--file-format (-F)\n";
      LOG(WARN) << "In case this is not correct \n";
    }
    if (filetypeDeduced) {
      LOG(INFO) << "Assuming input file format to be " << fileFormat
                << " due to the input file's extension.\n";
      LOG(INFO)
          << "If this is wrong, please manually specify the --file-format "
             "(-F) flag.\n";
    }
  }

  if (inputFile.empty()) {
    LOG(INFO) << "No input file was specified, parsing from stdin\n";
    inputFile = "/dev/stdin";
  } else if (inputFile == "-") {
    LOG(INFO) << "Parsing from stdin\n";
    inputFile = "/dev/stdin";
  }

  LOG(INFO) << "Trying to parse from input file " << argv[1] << std::endl;

  if (!outputFile.empty()) {
    std::ofstream of(outputFile);
    if (!of) {
      LOG(ERROR) << "Error opening '" << outputFile << "'" << std::endl;
      printUsage(argv[0]);
      exit(1);
    }
    LOG(INFO) << "Writing to file " << outputFile << std::endl;
    writeNT(of, fileFormat, inputFile);
    of.close();
  } else {
    LOG(INFO) << "Writing to stdout" << std::endl;
    writeNT(std::cout, fileFormat, inputFile);
  }
}
