#include <iostream>
#include <cstring>
#include <string>
#include <cstdlib>

#include "datafiletoolfuncs.h"
#include "../datafilehandlermethod.h"
#include "../filesystem.h"
#include "../path.h"

int usage() {
  std::cout << "datafilecat: decrypts the content of a cbftp data file.\n\n"
            << "Usage: datafilecat [--infile=] [--outfile=]"
            << std::endl;
  return 0;
}

int main(int argc, char ** argv) {
  Path path = Path(getenv("HOME")) / DATAPATH / DATAFILE;
  bool useoutfile = false;
  std::string outfile;
  for (int i = 1; i < argc; i++) {
    if (!strncmp(argv[i], "--infile=", 9)) {
      path = argv[i] + 9;
    }
    else if (!strncmp(argv[i], "--outfile=", 10)) {
      useoutfile = true;
      outfile = argv[i] + 10;
    }
    else {
      return usage();
    }
  }


  if (!checkInputFile(path)) return -1;

  Core::BinaryData key = getPassphrase();

  Core::BinaryData rawdata;
  FileSystem::readFile(path, rawdata);

  Core::BinaryData decryptedtext;
  if (!DataFileHandlerMethod::decrypt(rawdata, key, decryptedtext)) {
    std::cerr << "Error: Either the passphrase is wrong, or the input file is"
              << " not a valid cbftp data file."
              << std::endl;
    return -1;
  }

  if (useoutfile) {
    FileSystem::writeFile(outfile, decryptedtext);
  }
  else {
    std::cout << std::string((const char *)&decryptedtext[0],
                             decryptedtext.size());
    std::cerr << std::endl;
  }
  return 0;
}
