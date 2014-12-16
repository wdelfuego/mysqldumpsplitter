MySQLDump Splitter
===================

Splits mysqldump output to files of any maximum size. Keeps syntax correct, even when using extended inserts.

##What is it?

A short piece of C++ that will split an SQL file created by mysqldump into several smaller files of configurable maximum size, while keeping the SQL syntax correct. This supports multiple insert syntax, so it can split a single insert with lots of rows into two separate insert queries each with their own set of rows.

May work on other dialects than MySQL, but that's untested. Please let us know if you find that it works (or doesn't)!

##Why we built it

To split really large SQL files into smaller ones that could be handled more easily.

We are never storing so many BLOBs in a single MySQL database again :).

##Compiling
Pre-compiled binaries can be found in /bin, and are currently supplied for the following platforms:

- OS X

If a pre-compiled binary for your platform is not available, you must compile it yourself. The required files are in /src: 
* main.cpp is the source code
* Makefile is supplied
* SQLSplitter.xcodeproj is an XCode project for those who like that, you won't need it for compiling.

##Usage
Make sure the compiled file (in our case, 'sqlsplitter') is executable ('chmod +x sqlsplitter'), then run the following single command:

    ./sqlsplitter input_filename max_output_filesize_in_bytes

For example, to split `dump.sql` into files of 1MB each:

    ./sqlsplitter dump.sql 1048576

Which will result in a list of files named `[00001]dump.sql`, `[00002]dump.sql`, et cetera.

##Can I help?
Yes! We'd be glad to receive pre-compiled versions that are tested on other platforms.
Put them in a clearly named folder in /bin and send us a pull request.

Also, any improvements to the code are welcome, just send a pull request.

The code works but can be extended in quite some ways, here are some features that would be nice to have:

* Configurable output file name format
* A dry-run mode that doesn't output files, to see if the requested file size can be met.
* Tested support for more SQL dialects (currently the only known supported format is MySQL).
* Support for human-readable file size input (`1M` versus `1048576`)

##License

This code is licensed under the MIT License.

That boils down to:

- You are free to do with this code what you want, personally or commercially.
- You are not required to release modifications, extensions or updates to the code (but it'd be great if you would).
- You must keep the original copyright notice intact in all instances of the code.
- The software is provided "as is"; we are never liable for any damage it causes.