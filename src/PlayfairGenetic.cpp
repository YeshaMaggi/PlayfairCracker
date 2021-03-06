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
#include "Key.hpp"
#include "PlayfairGenetic.hpp"
#include "PfHelpers.hpp"
#include <algorithm>
#include <sstream>

#define ALPHABET "ABCDEFGHIKLMNOPQRSTUVWXYZ"

using std::vector;
using std::unordered_map;
using std::string;
using std::stringstream;
using std::list;

namespace {
	string randomKey(rng_t &rng) {
		string alphabet = ALPHABET;
		std::shuffle(alphabet.begin(), alphabet.end(), rng);
		return alphabet;
	}

	string seedKey(rng_t &rng, string seed) {
		unordered_map<char, bool> letterUsed;
		string alphabet = ALPHABET;
		string key;
		for(unsigned i = 0; i < alphabet.length(); i++) {
			letterUsed[toupper(alphabet[i])] = false;
		}
		for(unsigned i = 0; i < seed.length(); i++) {
			char letter = toupper(seed[i]);
			if(isalpha(letter)) {
				if(!letterUsed[letter]) {
					letterUsed[letter] = true;
					key.push_back(letter);
				}
			}
		}
		//  Might be different than seed.length() because doubles and non-letters are removed!
		int seedLength = key.length();
		for(auto it = letterUsed.begin(); it != letterUsed.end(); ++it) {
			if(!it->second)
				key.push_back(it->first);
		}

		string::iterator it = key.begin();
		std::advance(it, seedLength);   
		std::shuffle(it, key.end(), rng);
		return key;
	}

	list<string> keepBest(const pop_t &population, const vector<score_t> scores,
			const GenParams genParams) {
		if(population.size() != scores.size()) 
			throw InvalidParameters("Vector sizes do not match: population & scores");

		list<string> bestPop;
		list<std::pair<string, score_t>> pop;
		for(unsigned index = 0; index < scores.size(); index++) {
			pop.push_back(std::pair<string, score_t> (population.at(index), scores.at(index)));
		}


		for(unsigned index = 0; index < genParams.keepBest; index++) {
			auto max = std::max_element(pop.begin(), pop.end(),
	            [](const std::pair<string, score_t>& left, const std::pair<string, score_t>& right){
		            return left.second <  right.second;
		        });
			bestPop.push_back(max->first);
			pop.erase(max);
		}	

		return bestPop;
	}

	vector<score_t> fitnessPopulation(const EnglishFitness &englishFit, const pop_t &population,
			const vector<char> &cipherText) {
		unsigned n = englishFit.getN();
		vector<score_t> scores;
		FrequencyCollector fCollector(n);
		for(auto it = population.begin(); it != population.end(); ++it) {
			fCollector.clear();

			Key key(*it);
			vector<char> pText = key.decrypt(cipherText);
			stringstream pTextStream(string(pText.begin(), pText.end()));

			fCollector.collectNGrams(pTextStream);
			try {
				scores.push_back(englishFit.fitness(fCollector));
			} catch(Exception e) {
				std::cerr << e.what() << '\n';
				throw;
			}
		}
		return scores;
	}

	std::pair<int, int> selectParents(const vector<score_t> &scores, rng_t &rng) {
		// 	Update to parent selection. Subtracting lowestFitnessValue from all fitness scores,
		//		then we use their proportions
		vector<score_t> cpyScores = scores;
		score_t worst = cpyScores.at(std::distance(cpyScores.begin(), std::min_element(cpyScores.begin(), cpyScores.end())));
		for(unsigned index = 0; index < cpyScores.size(); index++) {
			cpyScores[index] = cpyScores[index] - worst;
		}

		score_t scoreSum = PfHelpers::sumVector(cpyScores);

		//  Get a random double in range [0-scoreSum]
		std::uniform_real_distribution<score_t> uid(0,scoreSum);
		score_t pSelect = uid(rng);
		//  Select first parent
		int selection = -1;
		while(pSelect < scoreSum) {
			pSelect += cpyScores.at(++selection);
		}

		//  Select second parent
		scoreSum -= cpyScores.at(selection);
		int selection2 = 0;
		std::uniform_real_distribution<score_t> uid2(0,scoreSum);
		pSelect = uid2(rng);
		while(pSelect < scoreSum) {
			//  Don't pick the first parent twice
			if(selection2 != selection)
				pSelect += cpyScores.at(selection2);
			++selection2;
		}
		--selection2;

		std::pair<int, int> p(selection, selection2);
		return p;
	}

	pop_t& crossover(pop_t &population, const GenParams &genParams, rng_t &rng) {
		string p1 = population.at(0);
		string p2 = population.at(1);
		std::uniform_int_distribution<int> uid(0, 1);
		//	Child becomes a copy of p1
		//	Go through every letter and 50% chance to keep it or not
		//	Letters that aren't being used are added in order from p2
		for(unsigned i = 0; i < genParams.numChildren; i++) {
			// 	If a letter exists in this map, it is being used
			unordered_map<char, bool> letterUsed;
			string key(p1.begin(), p1.end());
			vector<bool> letterKeep(key.size(), false);
			for(unsigned index = 0; index < key.size(); index++) {            
				if(uid(rng)) {
					letterKeep[index] = true;
					letterUsed.insert(std::make_pair(key[index], true));
				}
			}
			auto p2gene = p2.begin();
			for(unsigned index = 0; index < key.size(); index++) {
				if(!letterKeep[index]) {
					while(letterUsed.find(*p2gene) != letterUsed.end()) {
						++p2gene;
					}
					key[index] = *p2gene++;
				}
			}
			if(!PfHelpers::validKey(key)) {
				throw InvalidKeyException("InvalidKeyException in crossover()");
			}
			string keyString(key.begin(), key.end());
			population.push_back(keyString);
		}
		return population;
	}

	string& swapMutation(string &key, const GenParams &genParams, rng_t &rng) {
		vector<char> swapLetters;
		vector<char> swapIndicies;
		std::uniform_int_distribution<int> uid(0, 24);
		int first = uid(rng);
		int second = uid(rng);
		while(first == second) {
			second = uid(rng);
		}
		char temp = key[first];
		key[first] = key[second];
		key[second] = temp;
		if(!PfHelpers::validKey(key)) {
			throw InvalidKeyException("InvalidKeyException in swapMutation()");
		}
		return key;
	}

	string& inversionMutation(string &key, const GenParams &genParams, rng_t &rng) {
		std::uniform_int_distribution<int> uid(0,key.size() - 1);
		int start = uid(rng);
		int end = uid(rng);
		while(start == end) {
			end = uid(rng);
		}
		if(start > end) {
			int temp = start;
			start = end;
			end = temp;
		}
		std::reverse(key.begin()+start, key.begin()+end);

		if(!PfHelpers::validKey(key)) {
			throw InvalidKeyException("InvalidKeyException in inversionMutation())");
		}
		return key;
	}

	pop_t mutation(pop_t &population, const GenParams &genParams, rng_t &rng) {
		unsigned index = 0;
		while(index < population.size()) {
			string key = population[index];

			switch(genParams.mutationType) {
				case SWAP: {
					swapMutation(key, genParams, rng);
					break;
				}
				case INVERSION: {
					inversionMutation(key, genParams, rng);					
					break;
				}
				default: {
					std::cerr << "Invalid mutationType: " << genParams.mutationType << '\n';
					throw InvalidParameters("Invalid Parameters: mutationType");
				}
			}
			population[index] = key;
			++index;
		}
		return population;
	}

}


pop_t& PlayfairGenetic::initializePopulationRandom(unsigned popSize, pop_t &population, rng_t &rng) {
	population.clear();
	population.reserve(popSize);
	for(unsigned i = 0; i < popSize; i++) {
		population.push_back(randomKey(rng));
	}
	return population;
}

pop_t& PlayfairGenetic::initializePopulationSeed(unsigned popSize, pop_t &population, rng_t &rng, string seed) {
	population.clear();
	population.reserve(popSize);
	for(unsigned i = 0; i < popSize; i++) {
		population.push_back(seedKey(rng, seed));
	}
	return population;
}

pop_t& PlayfairGenetic::nextGeneration(const EnglishFitness &englishFit, const vector<char> &cipherText,
	const GenParams &genParams, pop_t &population, rng_t &rng) {
	//	get fitness scores for the population
	vector<score_t> scores = fitnessPopulation(englishFit, population, cipherText);
	//	Kill off the worst
	for(unsigned index = 0; index < genParams.killWorst; index++) {
		int worst = std::distance(scores.begin(), std::min_element(scores.begin(), scores.end()));
		population.erase(population.begin()+worst);
		scores.erase(scores.begin()+worst);
	}
	std::pair<int, int> parents = selectParents(scores, rng);
	
	string p1 = population.at(parents.first);
	string p2 = population.at(parents.second);

	list<string> bestPop = keepBest(population, scores, genParams);
	population.clear();
	population.push_back(p1);
	population.push_back(p2);

	try{
		crossover(population, genParams, rng);
	} catch(InvalidKeyException e) {
		std::cerr << "Crossover step produced an invalid key." << '\n';
		throw;
	}
	if(genParams.newRandom > 0) {
		pop_t newPop;
		initializePopulationRandom(genParams.newRandom, newPop, rng);
		population.reserve(population.size() + newPop.size());
		population.insert(population.end(), newPop.begin(), newPop.end());
	}
	try{
		mutation(population, genParams, rng);
	} catch(InvalidKeyException e) {
		std::cerr << "Mutation step produced an invalid key." << '\n';
		throw;
	} catch(InvalidParameters e) {
		std::cerr << e.what() << '\n';
	}

	//	Add the best elements that we kept earlier
	auto bestMember = bestPop.begin();
	for(unsigned index = 0; index < bestPop.size(); index++) {
		population.push_back(*bestMember++);
	}
	return population;
}

vector<score_t> PlayfairGenetic::fitScores(const EnglishFitness &englishFit, const pop_t &population,
		const vector<char> &cipherText) {
	return fitnessPopulation(englishFit, population, cipherText);
}

std::pair<string, score_t> PlayfairGenetic::bestMember(const pop_t &population, const vector<score_t> &scores) {
	int best = std::distance(scores.begin(), std::max_element(scores.begin(), scores.end()));

	return std::pair<string, score_t> (population.at(best), scores.at(best));
}

