#include "FrequencyCollector.h"
#include "Key.h"
#include "PlayfairGenetic.h"

#define ALPHABET "ABCDEFGHIKLMNOPQRSTUVWXYZ"

using std::vector;
using std::unordered_map;
using std::string;
using std::stringstream;


PlayfairGenetic::PlayfairGenetic() {}
PlayfairGenetic::~PlayfairGenetic() {}


int PlayfairGenetic::initializePopulationRandom(int popSize, vector<string> &population, std::mt19937 &rng) {
	population.clear();
	population.reserve(popSize);
	for(int i = 0; i < popSize; i++) {
		population.push_back(randomKey(rng));
	}
	return 0;
}

int PlayfairGenetic::initializePopulationSeed(int popSize, vector<string> &population, std::mt19937 &rng, string seed) {
	population.clear();
	population.reserve(popSize);
	for(int i = 0; i < popSize; i++) {
		population.push_back(seedKey(rng, seed));
	}
	return 0;
}




int PlayfairGenetic::printPopulation(vector<string> &population) {
	for(int i = 0; i < population.size(); i++) {
		std::cout << population.at(i) << '\n';
		vector<char> child(population.at(i).begin(), population.at(i).end());
		validKey(child);
	}
	return 0;
}


int PlayfairGenetic::nextGeneration(const NGrams &standardFreq, vector<string> &population, const vector<char> &cipherText, const GenerationParams &genParams, std::mt19937 rng) {
	std::pair<int, int> parents = selectParents(standardFreq, population, cipherText, genParams, rng);
	string p1 = population.at(parents.first);
	string p2 = population.at(parents.second);
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
		vector<string> newPop;
		initializePopulationRandom(genParams.newRandom, newPop, rng);
		population.reserve(population.size() + newPop.size());
		population.insert(population.end(), newPop.begin(), newPop.end());
	}
	try{
		mutation(population, genParams, rng);
	} catch(InvalidKeyException e) {
		std::cerr << "Mutation step produced an invalid key." << '\n';
		throw;
	}
	return 0;
}


string PlayfairGenetic::randomKey(std::mt19937 &rng) {
	string alphabet = ALPHABET;
	shuffle(alphabet.begin(), alphabet.end(), rng);
	return alphabet;
}

string PlayfairGenetic::seedKey(std::mt19937 &rng, string seed) {
	unordered_map<char, bool> letterUsed;
	string alphabet = ALPHABET;
	string key;
	for(int i = 0; i < alphabet.length(); i++) {
		letterUsed[toupper(alphabet.at(i))] = false;
	}
	for(int i = 0; i < seed.length(); i++) {
		char letter = toupper(seed.at(i));
		if(isalpha(letter)) {
			if(!letterUsed[letter])
				key.push_back(letter);
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
	shuffle(it, key.end(), rng);
	return key;
}

std::pair<int, int> PlayfairGenetic::selectParents(const NGrams &standardFreq, vector<string> &population, const vector<char> &cipherText, const GenerationParams &genParams, std::mt19937 rng) {

	vector<int> scores = fitnessPopulation(standardFreq, population, cipherText);
	//	Kill off the worst
	for(int index = 0; index < genParams.killWorst; index++) {
		int worst = std::distance(scores.begin(), std::min_element(scores.begin(), scores.end()));
		population.erase(population.begin()+worst);
		scores.erase(scores.begin()+worst);
	}
	
	int scoreSum = sumVectorInt(scores);

	//  Get a random integer in range [0-scoreSum]
	std::uniform_int_distribution<int> uid(0,scoreSum);
	int pSelect = uid(rng);
	//  Select first parent
	int selection = 0;
	while(pSelect < scoreSum) {
		pSelect += scores.at(selection++);
	}
	//  Select second parent
	scoreSum -= scores.at(selection);
	int selection2 = 0;
	std::uniform_int_distribution<int> uid2(0,scoreSum);
	pSelect = uid2(rng);
	while(pSelect < scoreSum) {
		//  Don't pick the first parent twice
		if(selection2 != selection)
			pSelect += scores.at(selection2++);
	}
	std::pair<int, int> p(selection, selection2);
	return p;
}
vector<int> PlayfairGenetic::fitnessPopulation(const NGrams &standardFreq, const vector<string> &population, const vector<char> &cipherText) {
	int n = standardFreq.n;
	vector<int> scores;
	FrequencyCollector fCollector;
	for(auto it = population.begin(); it != population.end(); ++it) {
		NGrams testFreq {n};
		int testCount;

		Key key(*it);
		vector<char> pText = key.decrypt(cipherText);        ;
		stringstream pTextStream(string(pText.begin(), pText.end()));

		fCollector.collectNGrams(pTextStream, testFreq);
		scores.push_back(fitness(standardFreq, testFreq));
	}
	return scores;
}
int PlayfairGenetic::fitness(const NGrams &standardFreq, const NGrams testFreq) {
	int n = standardFreq.n;
	double fitness = 0;
	if(standardFreq.freqs.empty() || testFreq.freqs.empty()) return -1;
	if((standardFreq.freqs.begin()->first.length() != n) || (testFreq.freqs.begin()->first.length() != n)) return -2;

	//  Iterate through all permutations of n letters
	//  perms[] is used like an odometer
	int *perms = new int[n]();
	int max = std::pow(26,n);
	for(int i = 0; i < max; i++) {
		string perm;
		for(int index = 0; index < n; index++) {
			perm.push_back(65 + perms[index]);
		}
		//  Increment perms
		perms[0] = (perms[0] + 1) % 26;
		//  Check if 'odometer' rolls over
		for(int index = 1; index < n; index++) {
			if(perms[index - 1] == 0) {
				perms[index] = (perms[index] + 1) % 26;
			} else {
				break;
			}
		}

		//  Collect the standard and test rate of permutation
		double standardF, testF;
		auto sF = standardFreq.freqs.find(perm);
		if(sF != standardFreq.freqs.end())
			standardF = double(sF->second) / standardFreq.count;
		else
			standardF = 0;
		auto tF = testFreq.freqs.find(perm);
		if(tF != testFreq.freqs.end())
			testF = double(tF->second) / testFreq.count;
		else
			testF = 0;

		//  Add to fitness score
		fitness += std::pow(std::abs(standardF - testF), 2);
	}
	//  Right now, lower fitness is better. Let's take the inverse. Now higher is better.
	return int(floor(1 / fitness));
}



int PlayfairGenetic::crossover(vector<string> &population, const GenerationParams &genParams, std::mt19937 rng) {
	string p1 = population.at(0);
	string p2 = population.at(1);
	std::uniform_int_distribution<int> uid(0, 1);
	//	Child becomes a copy of p1
	//	Go through every letter and 50% chance to keep it or not
	//	Letters that aren't being used are added in order from p2
	for(int i = 0; i < genParams.numChildren; i++) {
		// 	If a letter exists in this map, it is being used
		unordered_map<char, bool> letterUsed;
		vector<char> key(p1.begin(), p1.end());
		vector<bool> letterKeep(key.size(), false);
		for(int index = 0; index < key.size(); index++) {            
			if(uid(rng)) {
				letterKeep[index] = true;
				letterUsed.insert(std::make_pair(key.at(index), true));
			}
		}
		auto p2gene = p2.begin();
		for(int index = 0; index < key.size(); index++) {
			if(!letterKeep.at(index)) {
				while(letterUsed.find(*p2gene) != letterUsed.end()) {
					++p2gene;
				}
				key[index] = *p2gene++;
			}
		}
		if(!validKey(key)) {
			throw InvalidKeyException("InvalidKeyException in crossover()");
		}
		string keyString(key.begin(), key.end());
		population.push_back(keyString);
	}
	return 0;
}

//	Fix this return/th
int PlayfairGenetic::mutation(vector<string> &population, const GenerationParams &genParams, std::mt19937 rng) {
	switch(genParams.mutationType) {
		case 0: {
			for(int index = 0; index < population.size(); index++) {
				string key = population.at(index);
				swapMutation(key, genParams, rng);
				population.at(index) = key;
			}
			break;
		}
		case 1: {
			std::uniform_real_distribution<double> urd(0.0, 1.0);
			for(int index = 0; index < population.size(); index++) {
				double rand = urd(rng);
				if(rand < genParams.mutationRate) {
					string key = population.at(index);
					inversionMutation(key, genParams, rng);
					population.at(index) = key;
				}
			}
			break;
		}
		default: {
			std::cerr << "Invalid mutationType: " << genParams.mutationType << '\n';
			return -11;
		}
	}
	return 0;
}

int PlayfairGenetic::swapMutation(string &key, const GenerationParams &genParams, std::mt19937 rng) {
	vector<char> swapLetters;
	vector<char> swapIndicies;
	std::uniform_real_distribution<double> urd(0.0, 1.0);
	for(int index = 0; index < key.size(); index++) {
		double rand = urd(rng);
		if(rand < genParams.mutationRate) {
			swapLetters.push_back(key.at(index));
			swapIndicies.push_back(index);
		}
	}
	shuffle(swapLetters.begin(), swapLetters.end(), rng);
	for(int i = 0; i < swapIndicies.size(); i++) {
		key.at(swapIndicies.at(i)) = swapLetters.at(i);
	}
	if(!validKey(key)) {
		throw InvalidKeyException("InvalidKeyException in swapMutation()");
	}
	return 0;
}

int PlayfairGenetic::inversionMutation(string &key, const GenerationParams &genParams, std::mt19937 rng) {
	std::uniform_int_distribution<int> uid(0,key.size() - 1);
	int start = uid(rng);
	int end = uid(rng);
	if(start > end) {
		int temp = start;
		start = end;
		end = temp;
	}
	std::reverse(key.begin()+start, key.begin()+end);

	if(!validKey(key)) {
		throw InvalidKeyException("InvalidKeyException in inversionMutation())");
	}
	return 0;
}


template <typename T>
bool PlayfairGenetic::validKey(T key) {
	if(key.size() != 25) return false;

	unordered_map<char, int> letterUsed;
	for(int index = 0; index < key.size(); index++) {
		//	If the letter has alreay been put in
		if(letterUsed.find(key.at(index)) != letterUsed.end()) {
			return false;
		}
		letterUsed.insert(std::make_pair(key.at(index), 0));
	}
	return true;
}

int PlayfairGenetic::sumVectorInt(vector<int> vec) {
	int total = 0;
	for(auto it = vec.begin(); it != vec.end(); ++it)
		total += *it;
	return total;
}


std::ifstream::pos_type PlayfairGenetic::fileSize(const string fileName) {
	std::ifstream in(fileName, std::ifstream::ate | std::ifstream::binary);
	return in.tellg(); 
}

int PlayfairGenetic::readFile(const string fileName, vector<char> &text) {
	text.clear();
	text.reserve(fileSize(fileName));
	std::ifstream fileReader(fileName);
	if(!fileReader) {
		std::cerr << fileName << " can not be opened." << '\n';
		return -1;
	}
	char ch;
	while(fileReader.get(ch)) {
		text.push_back(ch);
	}
	return 0;
}