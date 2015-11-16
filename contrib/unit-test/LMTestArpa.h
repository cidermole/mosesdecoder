/****************************************************
 * Moses - factored phrase-based language decoder   *
 * Copyright (C) 2015 University of Edinburgh       *
 * Licensed under GNU LGPL Version 2.1, see COPYING *
 ****************************************************/

#ifndef MMT_SRC_NOSYNC_TESTARPA_H
#define MMT_SRC_NOSYNC_TESTARPA_H

/**
 * Creates a temp file and fills it with a tiny bigram LM. File is removed after main() exits.
 *
 * @return ARPA file name
 */
std::string setupArpa();

#endif //MMT_SRC_NOSYNC_TESTARPA_H
