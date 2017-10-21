//    Copyright (C) 2011 by RODO - INTELLIGENT COMPUTING (www.rodo.nl)
//
//    Permission is hereby granted, free of charge, to any person obtaining a copy
//    of this software and associated documentation files (the "Software"), to deal
//    in the Software without restriction, including without limitation the rights
//    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//    copies of the Software, and to permit persons to whom the Software is
//    furnished to do so, subject to the following conditions:
//
//    The above copyright notice and this permission notice shall be included in
//    all copies or substantial portions of the Software.
//
//    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//    THE SOFTWARE.

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

bool isNewLine(char c){
	return (c == '\n' || c == '\r');
}

void outputStatistics(size_t const& currentBytes, size_t const& maxBytes, std::string const& prefix, std::string const& suffix = "", size_t const barWidth = 37){
	std::cout << '\n';
	std::cout << std::setw(4) << std::right << prefix << '[';
	float const ratio = currentBytes/(float)maxBytes;
	
	size_t const maxBar = ratio * barWidth;
	for(size_t currentBar=0; currentBar < maxBar; ++currentBar){
		std::cout << '=';
	}
	for(size_t currentEmpty=maxBar; currentEmpty < barWidth; ++currentEmpty){
		std::cout << ' ';
	}
	
	std::cout << ']' << suffix;
	std::cout.flush();
}

int main (int argc, char * const argv[]) {
	size_t maxByteSize;
	std::string inputFilePath;
	size_t outputBarWidth = 60;
	std::ifstream sqlFile;
	
	if(argc == 2){
		std::string potentialCommand(argv[1]);
		if(potentialCommand == "!!test!!"){
			std::cout << "WARNING: Testing mode enabled!";
			inputFilePath = "../../test.sql";
			maxByteSize = 1024*1024*10;
		}
	} else if(argc < 3){
		//Inform herp-derp user of his sins committed.
		std::cout << "Herp derp! Usage: " << argv[0] << " <sqldump to parse> <maximum packet size>\n";
		return kReturnBadArguments;
	} else {
		//Parse arguments
		inputFilePath = argv[1];
		maxByteSize = atol(argv[2]);
		if(argc >= 4){
			outputBarWidth = atoi(argv[3]);
		}
	}
	
	sqlFile.open(inputFilePath.c_str(), std::ifstream::binary);
	if(!sqlFile){
		std::cerr << "Can't open file(" << inputFilePath << ") for reading\n";
		return kReturnCanNotOpen;
	} else {
		//std::cout << "File has been opened for reading\n";
	}
	
	std::string inputFileName(inputFilePath, inputFilePath.find_last_of('/')+1);
	
	std::cout << 
	"Good to go! Will split (" << inputFilePath << " to a maximum of " << maxByteSize << " bytes" <<
	std::endl;
	
#pragma mark Split it!
	while(true){
		//Create filename base + partCount
		static size_t partCount = 0;
		std::stringstream partStream;
		partStream << std::setw(5) << std::setfill('0') << partCount;
		std::string outputFilename("[" + partStream.str() + "]" + inputFileName);
		
		//Open output file
		std::ofstream currentOutputFile(outputFilename.c_str(), std::ofstream::binary);
		if(!currentOutputFile){
			std::cerr << "Fatal: Failed to open(" << outputFilename << ") for writing" << std::endl;
			return kReturnCanNotOpenForWrite;
		} else {
			std::cout << "Will write part[" << partCount << "] to " << outputFilename << std::endl;
		}
		
		//Fetch the new statement, prepare block of statements to be written
		static std::string lastReadStatement;
		std::string blockOfStatements(lastReadStatement);
		while(sqlFile){
			//Fetch new statement, the first unescaped ';', that's not between '\''
			std::string singleStatement;
			{
				char current, previous = 0;
				bool endFound = false;
				unsigned int quoteCount = 0;
				
				do {
					sqlFile.get(current);
					if(!sqlFile) {
						//std::cout << "End of file reached, writing last parts (if any) and quitting";
						break;
					}
					//std::cout << current;
					if(current == '\'' && previous != '\\'){
						++quoteCount;
					} else if (current == ';' && previous != '\\' && (quoteCount % 2) == 0){
						endFound = true;
					}
					
					singleStatement += current;
					previous = current;
				} while(!endFound);
				
				std::string linePreview = singleStatement.substr(0, 40);
				std::string::iterator newEnd = std::remove_if(linePreview.begin(), linePreview.end(), isNewLine);
				linePreview.resize(std::distance(linePreview.begin(), newEnd));
				
				outputStatistics(blockOfStatements.length(), maxByteSize, partStream.str(), " " + linePreview + "...", outputBarWidth);
			}
			
			if(singleStatement.length() > maxByteSize){
				std::cerr << 
				"Fatal: Smallest statement is bigger(" << singleStatement.length() << ") than given bytesize(" << maxByteSize << ")";
				std::cout << singleStatement;
				return kReturnMaxByteSizeExceeded;
			}
			
			if((singleStatement.length() + blockOfStatements.length()) > maxByteSize){
				//std::cout << "Appending statement would make block bigger than allowed bytesize\n";
				lastReadStatement = singleStatement;
				break;
			} else {
				blockOfStatements += singleStatement;
			}
		}
		
		outputStatistics(blockOfStatements.length(), maxByteSize, partStream.str(), " Writing to file\n", outputBarWidth);
		currentOutputFile.write(blockOfStatements.c_str(), blockOfStatements.length());
		if(!currentOutputFile) {
			std::cerr << "Fatal: Failed to write to output file, aborting\n";
			return kReturnCanNotWrite;
		}
		
		
		if(!sqlFile) break; //We're done!
		
		++partCount;
	}
	
	return 0;
}
