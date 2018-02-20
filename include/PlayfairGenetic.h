#ifndef PLAYFAIRGENETIC_H
#define PLAYFAIRGENETIC_H

#include "FrequencyCollector.h"
#include "Key.h"
#include <string>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <random>
#include <chrono>
#include <algorithm>
#include <iterator>

using std::vector;
using std::unordered_map;
using std::string;
using std::stringstream;

class InvalidKeyException {
public:
   InvalidKeyException(const char* msg = "InvalidKeyException") : e_msg(msg) {}
  ~InvalidKeyException( ) {}

   const char* getMessage( ) const {return(e_msg);}
private:
   const char* e_msg;
};

class InvalidParameters {
public:
	InvalidParameters(const char* msg = "Invalid Parameters") : e_msg{msg} {}
	~InvalidParameters( ) {}

	const char* getMessage( ) const {return(e_msg);}
private:
	const char* e_msg;
};
/*
	--- PARAMETERS ---
	These parameters are applied each generation	

	numChildren
		How many children will be produced

	newRandom
		How many new randomized keys will be added to population
 	
 	mutationRate
 		Mutation is applied to each individual. MUST BE BETWEEN 0.0 - 1.0
 		The effect of this rate depends on mutationType
 	
 	mutationType	MutationType
 		
 	killWorst
 		How many of the worst population are killed off each generation.
 		In effect, the members who don't get any proportional chance.

 */
enum MutationType { SWAP, INVERSION };
/*
	SWAP  		mutationRate affects the chance that a letter will be swapped.
				Each letter in key is added to a list that then gets shuffled.

	INVERSION 	A member of the population has a mutationRate chance of having
				a random sized substring of letters reversed.
*/
struct GenerationParams {
	int numChildren;
	int newRandom;
	double mutationRate;
	int mutationType;
	int killWorst;
};

class PlayfairGenetic {
public:
	PlayfairGenetic();
	~PlayfairGenetic();

	//	This initializes a pseudo-random population. 'population' will be cleared 
	//  	and resized appropriately.
	int initializePopulationRandom(int popSize, vector<string> &population,
			std::mt19937 &rng);

	//	This initializes a population with pseudo-random variation around a given
	//		seed, or keyword. If seed is 'Apple', each member will start with
	//		'APLE', with the remaining characters randomized. 'population' will be
	//		cleared and resized appropriately. 
	int initializePopulationSeed(int popSize, vector<string> &population,
			std::mt19937 &rng, string seed);


	int nextGeneration(const NGrams &standardFreq, vector<string> &population,
			const vector<char> &cipherText,	const GenerationParams &genParams,
			std::mt19937 rng);

	//	Prints given population
	int printPopulation(vector<string> &population);

	std::pair<string, double> bestMember(const NGrams &standardFreq, const vector<string> &population, const vector<char> &cipherText);

private:
	//	Produces a pseudo-random key	
	string randomKey(std::mt19937 &rng);
	// 	Produces a key from seed
	string seedKey(std::mt19937 &rng, string seed);

	//	Returns the indices of the two parents
	std::pair<int, int> selectParents(const NGrams &standardFreq,
			vector<string> &population,	const vector<char> &cipherText,
			const GenerationParams &genParams, std::mt19937 rng);

	//	Applies the fitness function on the entire population
	vector<int> fitnessPopulation(const NGrams &standardFreq,
			const vector<string> &population, const vector<char> &cipherText);

	int fitness(const NGrams &standardFreq, const NGrams testFreq);

	int crossover(vector<string> &population, const GenerationParams &genParams,
			std::mt19937 rng);

	int mutation(vector<string> &population, const GenerationParams &genParams,
			std::mt19937 rng);

	//	Each letter 
	int swapMutation(string &key, const GenerationParams &genParams,
			std::mt19937 rng);

	int inversionMutation(string &key, const GenerationParams &genParams,
			std::mt19937 rng);

	//	Iterates using .at()
	//	Used on string, vector<char>, etc.
	template <typename T>
	bool validKey(T key);
	int sumVectorInt(vector<int> vec);
};


#endif // PLAYFAIRGENETIC_H