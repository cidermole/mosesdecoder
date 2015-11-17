#!/bin/bash
#
# Wraps bjam and g++ to produce a list of all source files
# actually compiled in a bjam run. Works via hijacking g++
# through a modified PATH, and grabbing the argument passed.

BJAM=$(dirname $0)/bjam

######

# From the finished source crawl, produce some statistics
# (such as the number of users of header files)
#
function source_stats() {
	# source_stats $LIST_TMP_DIR $ABS_SRC
	LIST_TMP_DIR="$1"
	ABS_SRC="$2"

	find $LIST_TMP_DIR/headers -type f -exec cat {} \; | sed '/^\/usr/d' | awk '{gsub("^'$ABS_SRC'/", ""); print;}' | sort | uniq > $LIST_TMP_DIR/sources-and-headers.txt
	awk '!/(\.cpp|\.cc)/' $LIST_TMP_DIR/sources-and-headers.txt > $LIST_TMP_DIR/headers.txt

	pushd $LIST_TMP_DIR/headers >/dev/null
	while read header; do
		header_users=$LIST_TMP_DIR/header-users/$header
		mkdir -p $(dirname $header_users)

		# find the source files using this header
		grep -rl $header . | cut -c 3- > $header_users
		nusages=$(cat $header_users | wc -l)
		echo "$nusages $header"
	done < $LIST_TMP_DIR/headers.txt > $LIST_TMP_DIR/header-usage.txt
	popd >/dev/null

	# sort descending by usage count
	sort -n -r $LIST_TMP_DIR/header-usage.txt > $LIST_TMP_DIR/header-usage.sorted.txt
	mv $LIST_TMP_DIR/header-usage.sorted.txt $LIST_TMP_DIR/header-usage.txt
}

######

if [ "$(basename $0)" == "g++" ]; then
	# we are individual g++ wrapper

	for lastarg in "$@"; do true; done
	#source_file=$(eval echo $(echo '$'"$#"))
	source_file="$lastarg"

	# cpp source is last argument
	#source_file="$@" # print entire cmdline of g++

	# but bjam calls g++ for all other sorts of funky stuff,
	# so check if it is a source file we are looking for. 
	has_src=$(echo "$source_file" | awk '/(\.cpp|\.cc)/')
	if [ ! -z "$has_src" ]; then
		# make relative paths by substituting away the absolute prefix
		source_file=$(echo $source_file | awk '{gsub("^'$ABS_SRC'/", ""); print;}')
		echo "$source_file" >> $LIST_TMP_DIR/sources.txt
	fi

	# invoke real g++
	$ORIG_CPP "$@"
	result=$?

	# create list of headers...
	if [ ! -z "$has_src" ]; then
		headers=$LIST_TMP_DIR/headers/$source_file
		if [ ! -e $headers ]; then
			mkdir -p $(dirname $headers) || exit 1
			# with -M, gcc outputs a list of dependencies for the given source file.
			#ls -1 2>&- $($ORIG_CPP -M "$@") > $headers

			#print command...
			#echo $ORIG_CPP -M "$@" > $headers

			# get rid of -o output.o
			arglist=""
			while [[ $# > 0 ]] ; do
				if [ "$1" == "-o" ]; then
					# skip -o argument
					shift
				else
					arglist="$arglist \"$1\""
				fi
				shift
			done
			#ls -1 2>&- $($ORIG_CPP -M "$arglist") > $headers
			#echo $ORIG_CPP -M $arglist > $headers
			eval $ORIG_CPP -M $arglist | xargs ls -1 2>&- >> $headers

			# post-process a bit (remove /usr)
			#sed '/^\/usr/d' $headers > ${headers}.nousr

			# note: always has the cpp itself.
		fi
	fi

	exit $result
else
	# we are coordinating bjam wrapper
	export LIST_TMP_DIR=/tmp/list-sources.$$
	mkdir -p $LIST_TMP_DIR/bin || exit 1

	touch $LIST_TMP_DIR/sources.txt
	echo >&2 "Collecting list of source files in $LIST_TMP_DIR/sources.txt ..."

	# backup the path to original g++ (for me, /usr/bin/g++)
	export ORIG_CPP=$(which g++)

	# absolute path to avoid
	this_dir=$(readlink -f $(dirname $0))
	export ABS_SRC=$this_dir

	# symlink g++ to ourselves
	ln -s "$this_dir/$(basename $0)" $LIST_TMP_DIR/bin/g++
	export PATH=$LIST_TMP_DIR/bin:$PATH

	# run bjam compilation
	$BJAM "$@"
	result=$?

	# post-process
	sort $LIST_TMP_DIR/sources.txt | uniq > $LIST_TMP_DIR/sources.uniq.txt
	mv $LIST_TMP_DIR/sources.uniq.txt $LIST_TMP_DIR/sources.txt

	# produce nice stats
	source_stats $LIST_TMP_DIR $ABS_SRC

	echo >&2 "Collected list of cpp/cc source files in $LIST_TMP_DIR/sources.txt"
	exit $result
fi

