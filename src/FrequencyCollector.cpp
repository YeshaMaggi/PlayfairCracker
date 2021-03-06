/* PlayfairCracker - Crack Playfair Encryptions
 * Copyright (C) 2018 Yesha Maggi
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "FrequencyCollector.hpp"
#include "PfHelpers.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <string.h>
#include <regex>

FrequencyCollector::FrequencyCollector(unsigned N) :
	n{N}, totalCount{0}, freqs{} {
		if(N < 1) {
			throw InvalidParameters("N must be greater than 0");
		}
		if(N > 13) {
			throw InvalidParameters("N cannot be greater than 13");
		}
		if(N > 5) {
			std::cout << "Collecting n-grams with an N larger than 5 might produce " <<
				"errors or inaccurate results. It will be quite slow as well." << '\n';
		}
	}

FrequencyCollector::~FrequencyCollector() {}

bool FrequencyCollector::validNgramFile(const char* fileName) {
	std::ifstream fileReader(fileName);
	if(fileReader.fail()) {
		std::cerr << "Failed to open: " << fileName << '\n';
		return false;
	}

	std::unordered_map<ngram_t, count_t> NGrams;
	long lineNum = 1;
	while(!fileReader.eof()) {
		std::string line, ngramString, countString;
		std::getline(fileReader, line);
		if(line.empty() || line.at(0) == '/') continue;

		std::regex format("^[[:alpha:]]{" + std::to_string(n) + "} [[:digit:]]+$");
		if(!std::regex_match(line, format)) {
			std::cerr << "Line " << lineNum << " of " << fileName << "is of wrong format\n";
			return false;
		}

		ngram_t ngram = line.substr(0,n);
		if(NGrams.find(ngram) != NGrams.end()) {
			std::cerr << "Line " << lineNum << " of " <<fileName << "has duplicate: " <<
				ngram << '\n';
			return false;
		}
		NGrams.insert(std::make_pair(ngram, 0));

		++ lineNum;
	}
	return true;
}

int FrequencyCollector::readNgramCount(const char* fileName) {
	unsigned long numLines;
	try {
		numLines = numFileLines(fileName);
	} catch (const std::ifstream::failure e) {
		std::cerr << e.what() << '\n';
		throw;
	}
	
	freqs.reserve(numLines);

	std::ifstream fileReader(fileName);
	if(fileReader.fail()) {
		string e = "Failed to open: ";
		e += fileName;
		throw std::ios_base::failure(e.c_str());
	}
	
	while(!fileReader.eof()) {
		//	Read each line, split line into - n-gram and count
		std::string line, ngramString, countString;
		std::getline(fileReader, line);
		if(line.empty() || line.at(0) == '/') continue;
		// 	Split the line
		std::stringstream ss(line);
		ss >> ngramString;
		ss >> countString;
		count_t count = std::stoi(countString);

		totalCount += count;

		// 	If the n-gram read was of the wrong length.
		if(ngramString.size() != n) {
			std::cerr << "Ngram length contradiction!" << '\n';
			std::cerr << "  Ngram read from " << fileName << " was of length: " << ngramString.size() << '\n';
			std::cerr << "  Ngram length expected: " << n << '\n';
			throw Exception("Error: Ngram length contradiction");
		}
		
		if(freqs.find(ngramString) == freqs.end()) {
			freqs.insert(std::make_pair(ngramString, 0));
		}
		freqs.at(ngramString) += count;
	}
	return 0;
}

int FrequencyCollector::writeNGramCount(const char* fileName) {
	std::ofstream fileWriter(fileName);
	if(fileWriter.fail()) {
		string e = "Failed to open: ";
		e += fileName;
		throw std::ios_base::failure(e.c_str());
	}

	printNGrams(fileWriter);
	return 0;
}

int FrequencyCollector::setNGramCount(const char* fileRead, const char* fileWrite) {
	this->clear();
	collectNGramsFile(fileRead);
	writeNGramCount(fileWrite);
	return 0;
}

int FrequencyCollector::collectNGramsFile(const char fileName[]) {
	std::ifstream fileReader(fileName);
	if(fileReader.fail()) {
		string e = "Failed to open: ";
		e += fileName;
		throw std::ios_base::failure(e.c_str());
	}

	std::stringstream buffer;
	buffer << fileReader.rdbuf();

	collectNGrams(buffer);
	return 0;
}

int FrequencyCollector::collectNGrams(std::stringstream &buffer) {
	// 	Read stream 1 letter at a time and add them to this queue
	// 	Queue will "rotate" as letters get added
	char *queue = new char[n] ();
	//	Keep track of the current start of the queue
	unsigned curPos = 0;
	// 	This first while-loop is just to fill up the queue
	//	Collect the first N letters before adding the ngram to map
	char ch;
	while(buffer.get(ch)) {
		if(!isalpha(ch)) continue;
		queue[curPos++] = toupper(ch);
		// 	Fill up the queue before adding ngrams to map
		if(curPos >= n-1) break;
	}
	// 	Read buffer one character at a time
	while(buffer.get(ch)) {
		//  Only select the letters
		if(!isalpha(ch)) continue;
		++totalCount;
		// 	Make sure it is uppercase
		queue[curPos++] = toupper(ch);
		curPos = curPos % n;
		// 	Reorder the chars starting with curPos and wrapping
		ngram_t ngram;
		for(unsigned i = 0; i < n; i++) {
			ngram.push_back(queue[(curPos + i) % n]);
		}
		// 	Count how many times that n-gram occurs
		freqs[ngram] ++;
	}
	delete[] queue;
	return 0;
}

int FrequencyCollector::printNGrams(std::ostream &buffer) {
    for(auto it = freqs.begin(); it != freqs.end(); ++it) {
        buffer << it->first << " " << it->second << '\n';
    }
    return 0;
}

unsigned FrequencyCollector::getN() const {
	return n;
}

count_t FrequencyCollector::getCount() const {
	return totalCount;
}

double FrequencyCollector::frequency(ngram_t ngram) const {
	auto tF = freqs.find(ngram);
		if(tF != freqs.end())
			return double(tF->second) / totalCount;
		else
			return 0;
}

bool FrequencyCollector::isEmpty() const {
	if(freqs.empty()) return true;
	return false;
}

int FrequencyCollector::clear() {
	totalCount = 0;
	freqs.clear();
	return 0;
}


unsigned long FrequencyCollector::numFileLines(const char* fileName) {
	unsigned long numLines = 0;
	std::ifstream fileReader(fileName);
	if(fileReader.fail()) {
		string e = "Failed to open: ";
		e += fileName;
		throw std::ios_base::failure(e.c_str());
	}
	
	std::string buffer;
	while(!fileReader.eof()) {
		std::getline(fileReader, buffer);
		++ numLines;
	}
	return numLines;
}