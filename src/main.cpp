// Copyright (C) 2011 by RODO - INTELLIGENT COMPUTING (www.rodo.nl)
//
// 2017-2018 - fixes by Viachaslau Tratsiak (aka restorer)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>

#include <cstdlib>
#include <cassert>

enum return_values {
	kReturnBadArguments = 1,
	kReturnCanNotOpen,
	kReturnPrematureEndSQL,
	kReturnCanNotOpenForWrite,
	kReturnCanNotWrite,
	kReturnMaxByteSizeExceeded,
	kReturnUnknownReason,
};

bool isNewLine(char c) {
	return (c == '\n' || c == '\r');
}

void outputStatistics(
	size_t const& currentBytes,
	size_t const& maxBytes,
	std::string const& prefix,
	std::string const& suffix = "",
	size_t const barWidth = 37
) {
	std::cout << '\n';
	std::cout << std::setw(4) << std::right << prefix << '[';

	float const ratio = currentBytes/(float)maxBytes;
	size_t const maxBar = ratio * barWidth;

	for (size_t currentBar = 0; currentBar < maxBar; ++currentBar) {
		std::cout << '=';
	}

	for (size_t currentEmpty = maxBar; currentEmpty < barWidth; ++currentEmpty) {
		std::cout << ' ';
	}

	std::cout << ']' << suffix;
	std::cout.flush();
}

int main(int argc, char * const argv[]) {
	size_t maxByteSize;
	std::string inputFilePath;
	size_t outputBarWidth = 60;
	std::ifstream sqlFile;

	if (argc < 3) {
		std::cout << "Oops! Usage: " << argv[0] << " <input file> <maximum output file size in bytes> [output bar width]\n";
		return kReturnBadArguments;
	} else {
		inputFilePath = argv[1];
		maxByteSize = atol(argv[2]);

		if (argc >= 4) {
			outputBarWidth = atoi(argv[3]);
		}
	}

	sqlFile.open(inputFilePath.c_str(), std::ifstream::binary);

	if (!sqlFile) {
		std::cerr << "Can't open file (" << inputFilePath << ") for reading\n";
		return kReturnCanNotOpen;
	}

	std::string inputFileName(inputFilePath, inputFilePath.find_last_of('/') + 1);
	std::cout << "Good to go! Will split (" << inputFilePath << " to separate files with a maximum size of " << maxByteSize << " bytes" << std::endl;

	for (;;) {
		static size_t partCount = 0;
		std::stringstream partStream;
		partStream << std::setw(5) << std::setfill('0') << partCount;
		std::string outputFilename(partStream.str() + "-" + inputFileName);

		std::ofstream currentOutputFile(outputFilename.c_str(), std::ofstream::binary);

		if (!currentOutputFile) {
			std::cerr << "Fatal: Failed to open (" << outputFilename << ") for writing" << std::endl;
			return kReturnCanNotOpenForWrite;
		} else {
			std::cout << "Will write part [" << partCount << "] to " << outputFilename << std::endl;
		}

		static std::string lastReadStatement;
		std::string blockOfStatements(lastReadStatement);

		while(sqlFile) {
			// Fetch new statement, the first unescaped ';', that's not between '\''
			std::string singleStatement;

			{
				char current = 0;
				bool endFound = false;
				bool insideQuotes = false;
				bool isEscaped = false;

				do {
					sqlFile.get(current);

					if (!sqlFile) {
						break;
					}

					if (isEscaped) {
						isEscaped = false;
					} else if (current == '\\') {
						isEscaped = true;
					} else if (current == '\'') {
						insideQuotes = !insideQuotes;
					} else if (current == ';' && !insideQuotes) {
						endFound = true;
					}

					singleStatement += current;

					if (singleStatement.length() > 1L * 1024L * 1024L * 1024L) {
						currentOutputFile.write(blockOfStatements.c_str(), blockOfStatements.length());

						std::string debugFilename("debug-" + inputFileName);
						std::ofstream statementDebugFile(debugFilename.c_str(), std::ofstream::binary);
						statementDebugFile.write(singleStatement.c_str(), singleStatement.length());

						std::cerr << std::endl << "Too long statement - probably an internal bug" << std::endl;
						return kReturnMaxByteSizeExceeded;
					}
				} while (!endFound);

				std::string linePreview = singleStatement.substr(0, 40);
				std::string::iterator newEnd = std::remove_if(linePreview.begin(), linePreview.end(), isNewLine);
				linePreview.resize(std::distance(linePreview.begin(), newEnd));

				outputStatistics(blockOfStatements.length(), maxByteSize, partStream.str() + " ", " " + linePreview + "...", outputBarWidth);
			}

			if (singleStatement.length() > maxByteSize) {
				currentOutputFile.write(blockOfStatements.c_str(), blockOfStatements.length());

				std::string debugFilename("debug-" + inputFileName);
				std::ofstream statementDebugFile(debugFilename.c_str(), std::ofstream::binary);
				statementDebugFile.write(singleStatement.c_str(), singleStatement.length());

				std::cerr << std::endl << "Fatal: Smallest statement is bigger (" << singleStatement.length() << ") than given bytesize (" << maxByteSize << ")" << std::endl;
				return kReturnMaxByteSizeExceeded;
			}

			if ((singleStatement.length() + blockOfStatements.length()) > maxByteSize) {
				lastReadStatement = singleStatement;
				break;
			} else {
				blockOfStatements += singleStatement;
			}
		}

		outputStatistics(blockOfStatements.length(), maxByteSize, partStream.str(), " Writing to file\n", outputBarWidth);
		currentOutputFile.write(blockOfStatements.c_str(), blockOfStatements.length());

		if (!currentOutputFile) {
			std::cerr << "Fatal: Failed to write to output file, aborting\n";
			return kReturnCanNotWrite;
		}

		if (!sqlFile) {
			// We're done!
			break;
		}

		++partCount;
	}

	return 0;
}
