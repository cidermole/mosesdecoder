/****************************************************
 * Moses - factored phrase-based language decoder   *
 * Copyright (C) 2015 University of Edinburgh       *
 * Licensed under GNU LGPL Version 2.1, see COPYING *
 ****************************************************/

#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

#include "LMTestArpa.h"


namespace bfs = boost::filesystem;

bfs::path arpaTmpFile;
bool haveArpa = false;

void cleanupArpa() {
  bfs::remove(arpaTmpFile);
}

/**
 * Creates a temp file and fills it with a tiny bigram LM. File is removed after main() exits.
 *
 * @return ARPA file name
 */
std::string setupArpa() {
  if(haveArpa)
    return arpaTmpFile.string();

  arpaTmpFile = bfs::unique_path(bfs::temp_directory_path() / "%%%%-%%%%-%%%%-%%%%.arpa");
  // make sure to "always" clean up ARPA file (should work in most cases)
  atexit(cleanupArpa);
  std::ofstream ofs(arpaTmpFile.string());

  /*
   * Add-one smoothing bigram LM from the following corpus of 3 sentences,
   * using const -0.1 as backoff (except for score_backoff(mary) = -0.2):
   *
   * NOTE: ARPA file stores log_10 probabilities, while Moses internally uses log_e scores.
   *
   * "<s> john read moby dick . </s>"
   * "<s> mary read a different book . </s>"
   * "<s> she read a book by cher . </s>"
   *
   * <s> were explicitly included in the estimation, out of laziness.
   * Really, we only care that these same numbers end up being spit out by the LM again.
   *
   * Maybe we could have assigned whole primes as scores, to make more obvious what is going on.
   */

  // TODO: put this into extra arpa file, generate include
  // see https://cmake.org/Wiki/CMake_FAQ#How_can_I_generate_a_source_file_during_the_build.3F

  // this is the same stuff as in LMTest.arpa
  ofs <<
  "\\data\\\n"
      "ngram 1=15\n"
      "ngram 2=18\n"
      "\n"
      "\\1-grams:\n"
      "-1.431\tshe\t-0.100\n"
      "-1.431\tdifferent\t-0.100\n"
      "-1.255\tbook\t-0.100\n"
      "-1.130\tread\t-0.100\n"
      "-1.130\t.\t-0.100\n"
      "-1.431\tmary\t-0.200\n"
      "-1.431\tdick\t-0.100\n"
      "-1.255\ta\t-0.100\n"
      "-1.431\tmoby\t-0.100\n"
      "-1.431\tcher\t-0.100\n"
      "-1.130\t<s>\t-0.100\n"
      "-1.130\t</s>\t-0.100\n"
      "-1.431\tjohn\t-0.100\n"
      "-1.431\tby\t-0.100\n"
      "-1.732\t<UNK>\t-0.100\n"
      "\n"
      "\\2-grams:\n"
      "-1.243\ta different\n"
      "-1.230\tjohn read\n"
      "-1.243\tbook .\n"
      "-1.230\tcher .\n"
      "-1.255\t<s> she\n"
      "-0.954\t. </s>\n"
      "-1.243\tbook by\n"
      "-1.230\tdifferent book\n"
      "-1.230\tmoby dick\n"
      "-1.243\ta book\n"
      "-1.230\tdick .\n"
      "-1.230\tshe read\n"
      "-1.230\tmary read\n"
      "-1.255\t<s> john\n"
      "-1.255\tread moby\n"
      "-1.079\tread a\n"
      "-1.230\tby cher\n"
      "-1.255\t<s> mary\n"
      "\n"
      "\\end\\"
  << std::endl;

  haveArpa = true;
  return arpaTmpFile.string();
}
