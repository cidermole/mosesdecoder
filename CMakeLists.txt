cmake_minimum_required(VERSION 2.8.7)
project(mosesdecoder-tests)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS system thread filesystem unit_test_framework)

# MAX_NUM_FACTORS

add_definitions(-DMAX_NUM_FACTORS=4 -DKENLM_MAX_ORDER=6 -DWITH_THREADS -DBOOST_TEST_DYN_LINK)
include_directories(${Boost_INCLUDE_DIRS} .)

#set(SOURCES src/mainapp.cpp src/Student.cpp)
# set(SOURCE_FILES ...)

set(KENLM_SRC
    lm/bhiksha.cc
    lm/bhiksha.hh
    lm/binary_format.cc
    lm/binary_format.hh
    lm/blank.hh
    lm/config.cc
    lm/config.hh
    lm/enumerate_vocab.hh
    lm/facade.hh
    lm/left.hh
    lm/lm_exception.cc
    lm/lm_exception.hh
    lm/max_order.hh
    lm/model.cc
    lm/model.hh
    lm/model_type.hh
    lm/ngram_query.hh
    lm/partial.hh
    lm/quantize.cc
    lm/quantize.hh
    lm/read_arpa.cc
    lm/read_arpa.hh
    lm/return.hh
    lm/search_hashed.cc
    lm/search_hashed.hh
    lm/search_trie.cc
    lm/search_trie.hh
    lm/sizes.cc
    lm/sizes.hh
    lm/state.hh
    lm/trie.cc
    lm/trie.hh
    lm/trie_sort.cc
    lm/trie_sort.hh
    lm/value_build.cc
    lm/value_build.hh
    lm/value.hh
    lm/virtual_interface.cc
    lm/virtual_interface.hh
    lm/vocab.cc
    lm/vocab.hh
    lm/weights.hh
    lm/word_index.hh
    util/bit_packing.cc
    util/bit_packing.hh
    util/double-conversion/bignum.cc
    util/double-conversion/bignum-dtoa.cc
    util/double-conversion/bignum-dtoa.h
    util/double-conversion/bignum.h
    util/double-conversion/cached-powers.cc
    util/double-conversion/cached-powers.h
    util/double-conversion/diy-fp.cc
    util/double-conversion/diy-fp.h
    util/double-conversion/double-conversion.cc
    util/double-conversion/double-conversion.h
    util/double-conversion/fast-dtoa.cc
    util/double-conversion/fast-dtoa.h
    util/double-conversion/fixed-dtoa.cc
    util/double-conversion/fixed-dtoa.h
    util/double-conversion/ieee.h
    util/double-conversion/strtod.cc
    util/double-conversion/strtod.h
    util/double-conversion/utils.h
    util/ersatz_progress.cc
    util/ersatz_progress.hh
    util/exception.cc
    util/exception.hh
    util/fake_ostream.hh
    util/file.cc
    util/file.hh
    util/file_piece.cc
    util/file_piece.hh
    util/file_stream.hh
    util/fixed_array.hh
    util/float_to_string.cc
    util/float_to_string.hh
    util/generator.hh
    util/getopt.hh
    util/have.hh
    util/integer_to_string.cc
    util/integer_to_string.hh
    util/joint_sort.hh
    util/mmap.cc
    util/mmap.hh
    util/multi_intersection.hh
    util/murmur_hash.cc
    util/murmur_hash.hh
    util/parallel_read.cc
    util/parallel_read.hh
    util/pcqueue.hh
    util/pool.cc
    util/pool.hh
    util/probing_hash_table.hh
    util/proxy_iterator.hh
    util/random.cc
    util/random.hh
    util/read_compressed.cc
    util/read_compressed.hh
    util/scoped.cc
    util/scoped.hh
    util/sized_iterator.hh
    util/sorted_uniform.hh
    util/stream/block.hh
    util/stream/chain.cc
    util/stream/chain.hh
    util/stream/config.hh
    util/stream/count_records.cc
    util/stream/count_records.hh
    util/stream/io.cc
    util/stream/io.hh
    util/stream/line_input.cc
    util/stream/line_input.hh
    util/stream/multi_progress.cc
    util/stream/multi_progress.hh
    util/stream/multi_stream.hh
    util/stream/rewindable_stream.cc
    util/stream/rewindable_stream.hh
    util/stream/sort.hh
    util/stream/stream.hh
    util/stream/timer.hh
    util/string_piece.cc
    util/string_piece_hash.hh
    util/string_piece.hh
    util/string_stream.hh
    util/tempfile.hh
    util/thread_pool.hh
    util/tokenize.hh
    util/tokenize_piece.hh
    util/usage.cc
    util/usage.hh
)

set(LMTEST_COMMON_SRC
# in LMTEST_SRC
#    contrib/unit-test/LMTest.h
#    contrib/unit-test/LMTest.cpp
    
        contrib/unit-test/LMTestArpa.h
        contrib/unit-test/LMTestArpa.cpp
    
        # for CalcScore(), we build a Phrase
        moses/Phrase.h
        moses/Phrase.cpp
        
            moses/Word.h
            moses/Word.cpp
    
    # for Manager::LMCallback() - UGLY
    contrib/unit-test/MockIncremental.h
    contrib/unit-test/MockIncremental.cpp
    
    moses/LM/Ken.h
    moses/LM/Ken.cpp
    
    moses/LM/Base.h
    moses/LM/Base.cpp
    
        moses/FF/StatefulFeatureFunction.h
        moses/FF/StatefulFeatureFunction.cpp
    
            moses/FF/FeatureFunction.h
            moses/FF/FeatureFunction.cpp
    
    moses/FF/FFState.h
    moses/FF/FFState.cpp
    
        # Scan<bool>(str) for FeatureFunction
        moses/Util.h
        moses/Util.cpp
    
    # for word IDs within KenLM. Could just use different MappingBuilder
    moses/FactorCollection.h
    moses/FactorCollection.cpp
        
        moses/Factor.h
        moses/Factor.cpp
    
    moses/ScoreComponentCollection.h
    moses/ScoreComponentCollection.cpp
        
        moses/FF/StatelessFeatureFunction.h
        moses/FF/StatelessFeatureFunction.cpp
        
        contrib/unit-test/MockStaticData.h
        contrib/unit-test/MockStaticData.cpp
        
        # UGLY
        moses/FeatureVector.h
        moses/FeatureVector.cpp
    
#    moses/Hypothesis.h
#    moses/Hypothesis.cpp
#        moses/Phrase.h
#        moses/Phrase.cpp
#        moses/Word.h
#        moses/Word.cpp
#        ...
    
    
    ${KENLM_SRC}
)

set(LMTEST_SRC
    contrib/unit-test/LMTest.h
    contrib/unit-test/LMTest.cpp
    
    contrib/unit-test/MockHypothesis.h
    contrib/unit-test/MockHypothesis.cpp
    
    ${LMTEST_COMMON_SRC}
)

set(MOSES_OPTIONS_SRC
    moses/parameters/AllOptions.cpp                                                           
    moses/parameters/AllOptions.h                                                             
    moses/parameters/BeamSearchOptions.h                                                      
    moses/parameters/BookkeepingOptions.cpp                                                   
    moses/parameters/BookkeepingOptions.h                                                     
    moses/parameters/CubePruningOptions.cpp                                                   
    moses/parameters/CubePruningOptions.h                                                     
    moses/parameters/InputOptions.cpp                                                         
    moses/parameters/InputOptions.h                                                           
    moses/parameters/LMBR_Options.cpp                                                         
    moses/parameters/LMBR_Options.h                                                           
    moses/parameters/LookupOptions.h                                                          
    moses/parameters/MBR_Options.cpp                                                          
    moses/parameters/MBR_Options.h                                                            
    moses/parameters/NBestOptions.cpp                                                         
    moses/parameters/NBestOptions.h                                                           
    moses/parameters/OptionsBaseClass.cpp                                                     
    moses/parameters/OptionsBaseClass.h                                                       
    moses/parameters/OOVHandlingOptions.cpp
    moses/parameters/OOVHandlingOptions.h
    moses/parameters/ReorderingOptions.cpp                                                    
    moses/parameters/ReorderingOptions.h                                                      
    moses/parameters/ReportingOptions.cpp                                                     
    moses/parameters/ReportingOptions.h
    moses/parameters/SearchOptions.cpp
    moses/parameters/SearchOptions.h
    moses/parameters/ServerOptions.cpp
    moses/parameters/ServerOptions.h
    
    # Parameter for *Options, e.g. InputOptions
    moses/Parameter.h
    moses/Parameter.cpp
    
        # Scan<bool>(str)
        moses/Util.h
        moses/Util.cpp
        
        # FeatureRegistry
        moses/FF/Factory.h
        moses/FF/Factory.cpp
        
        moses/InputFileStream.h
        moses/InputFileStream.cpp
    
    # ... gzopen() ... 
    
    # does not link currently.
)

set(LMTEST_HYPO_SRC
    #contrib/unit-test/LMTestHypothesis.h
    contrib/unit-test/LMTestHypothesis.cpp
    
    moses/Hypothesis.h
    moses/Hypothesis.cpp
    
        contrib/unit-test/MockManager.h
        contrib/unit-test/MockManager.cpp
        
#        moses/Sentence.h
#        moses/Sentence.cpp

        contrib/unit-test/MockSentence.h
        contrib/unit-test/MockSentence.cpp

        
        moses/InputType.h
        moses/InputType.cpp
    
        moses/Bitmap.h
        moses/Bitmap.cpp
        
        moses/TranslationOption.h
        moses/TranslationOption.cpp
        
#            moses/TargetPhrase.h
#            moses/TargetPhrase.cpp
            
#                moses/AlignmentInfoCollection.h
#                moses/AlignmentInfoCollection.cpp
                
#                moses/TranslationModel/PhraseDictionary.h
#                moses/TranslationModel/PhraseDictionary.cpp
                
#                moses/PP/Factory.h
#                moses/PP/Factory.cpp

            # does not link currently.
            
            contrib/unit-test/MockTargetPhrase.h
            contrib/unit-test/MockTargetPhrase.cpp
        
            
        #${MOSES_OPTIONS_SRC}
        # does not link currently.
        
        moses/AlignmentInfo.h
        moses/AlignmentInfo.cpp
        
    moses/Bitmaps.h
    moses/Bitmaps.cpp
        
    ${LMTEST_COMMON_SRC}
)

add_executable(lmtest ${LMTEST_SRC})
target_link_libraries(lmtest ${Boost_LIBRARIES} pthread)

add_executable(lmtesthypo ${LMTEST_HYPO_SRC})
target_link_libraries(lmtesthypo ${Boost_LIBRARIES} pthread)

# wait, this builds two sets of source files?
