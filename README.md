# SFSFSSFSF #
## (Simple File System For Storing Secret Files in Sound Files) ##


This package aims to be a fully-featured steganographic file system. Its use case is those 
who need to evade network detection but have full access to a machine.

SFSFSSFSF has been forked by me (acrefoot) to indicate a new direction of the project. 
SFSFSSFSF was originally intended to be a proof of concept steganographic filesystem. 
As a proof of concept, with a very real deadline (it was for the Fall 2011 6.858 class 
at MIT), it suffered from a number of regrettable compromises. This project attempts to
make a more usable implementation, at a leisurely pace.

To build:
=========
$ ./autogen.sh
$ make

To cleanly rebuild:
===================
$ make clean
$ make
