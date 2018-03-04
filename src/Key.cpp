#include "Key.hpp"
#include <vector>
#include <stdexcept>

Key::Key(char doubleFill, char extraFill, char omitLetter, char replaceLetter) : 
	Key("", doubleFill, extraFill, omitLetter, replaceLetter) { }
	
Key::Key(std::string keyWord, char doubleFill, char extraFill, char omitLetter, char replaceLetter) :
	keyword{keyWord}, key {}, letterPlace{25} {

	if(isalpha(doubleFill))
		bufferDouble = toupper(doubleFill);
	if(isalpha(extraFill))
		bufferExtra = toupper(extraFill);
	if(isalpha(omitLetter))
		letterOmit = toupper(omitLetter);

	if(isalpha(replaceLetter)) {
		if(toupper(replaceLetter) == letterOmit) {
			// 	LOMIT and LREPL are from header file
			
			//	Which is the same as (if both == LREPL)
			if(letterOmit == letterReplace)
				letterReplace = LOMIT;
		} else {
			letterReplace = toupper(replaceLetter);
		}
	}
    generate();
}

Key::~Key() { }

/**
 * @brief 		Returns keyword as given in constructor
 * 
 * @details 	The keyword given in constructor is returned, rather than the keyword
 * 					as it appears in the square. (e.g. applecake,  not APLECK)
 * 
 * @return keyword
 */
std::string Key::getKeyword() {
	return keyword;
}

/**
 * @brief 		Encrypts plainText with key
 * @details 	Call sanitizeText() on plainText before using this function to ensure no
 * 					errors occur.
 * 
 * @param plainText 	Must only contain the 25 valid uppercase characters. There will
 * 							be errors otherwise. (Likely out_of_range exception).
 * @return  Encrypted cipherText
 */
std::vector<char> Key::encrypt(const std::vector<char> &plainText) {
	std::vector<char> cipherText;
	cipherText.reserve(plainText.size());
	// Iterate plainText 2 char at a time. Decrypt digram and add to cipherText
	for(std::vector<char>::const_iterator it = plainText.begin(); it != plainText.end(); ) {
		char a(*it), b;
		bool doubleLetter = false;
		// Add buffer to the end of odd length message
		if(++it == plainText.end()) {			
			b = bufferExtra;
		}
		else {
			b = *it;
			// If there is a double letter, add buffer letter
			if(a == b) {
				b = bufferDouble;
				doubleLetter = true;
				// If the buffer letter is the same as the doubled letter!
				if(a == b) b++;
			}
		}

		char* newDigram = encryptDigram(a, b);
		cipherText.push_back(newDigram[0]);
		cipherText.push_back(newDigram[1]);
		delete[] newDigram;

		if(it == plainText.end()) {
			break;
		}
		// Don't move iterator if we added a buffer letter
		if(!doubleLetter) ++it;
	}
	return cipherText;
}

/**
 * @brief 		Decrypts cipherText with key
 * @details 	Call sanitizeText() on cipherText before using this function to ensure no
 * 					errors occur. Letters added during encryption, due to double letters
 * 					or odd length text, are still included in decrypted text.
 * 
 * @param cipherText 	Must only contain the 25 valid uppercase characters. There will
 * 							be errors otherwise. (Likely out_of_range exception).
 * @return Decrypted plainText
 */
std::vector<char> Key::decrypt(const std::vector<char> &cipherText) {
	std::vector<char> plainText;
	plainText.reserve(cipherText.size());
	// Iterate cipherText 2 char at a time. Decrypt digram and add to plainText
	for(std::vector<char>::const_iterator it = cipherText.begin(); it != cipherText.end(); ) {
		char a(*it), b;
		if(++it == cipherText.end()) {
			// Add buffer to the end of odd length message.
			// Though if this class' encrypt() was used, this will never be the case.
			b = bufferExtra;
		}
		else b = *it;

		char* newDigram = decryptDigram(a, b);
		plainText.push_back(newDigram[0]);
		plainText.push_back(newDigram[1]);
		delete[] newDigram;
		
		if(it == cipherText.end()) {
			break;
		}
		else ++it;
	}
	return plainText;
}

/**
 * @brief  	Prepare text for encrypt() and decrypt()
 * @details This function is used to sanitize:
 * @li 			plainText  for encrypt()
 * @li 			cipherText for decrypt()
 * @li 			keyword    for generate() *(called internally)
 *
 * Valid characters are standard Latin letters A to Z, minus letterOmit.
 * All non-valid characters are removed from text, while all lowercase equivalents
 * 		to valid characters are converted to uppercase.
 * 
 * @param text 	Reference to text to be sanitized
 * @return 	Reference to text
 */

std::vector<char>& Key::sanitizeText(std::vector<char> &text) {
	for (std::vector<char>::iterator it = text.begin() ; it != text.end();) {
		// Change uppercase letters to lowercase
		if(*it > 96 && *it < 123) {
			*it = *it - 32;
		}
		// Change all J to I (or alternative letters)
		if(*it == letterOmit) {
			*it = letterReplace;
		}
		// Remove all non-letters
		if(*it < 65 || *it > 90) {
			it = text.erase(it);
		}
		else ++it;
	}
	text.resize(text.size());
	return text;
}

/**
 * @brief 	Called during construction to set up private variables
 * 
 * @return 	Return code: 0
 */
int Key::generate() {
	std::vector<char> keywordV(keyword.begin(), keyword.end());
	sanitizeText(keywordV);
	
	// The letter's place on the square is held as a number 0-24
	int lettersUsed = 0;
	// If keyword was given, start writing letters to square from keyword
    for(int index = 0; index < int(keywordV.size()); index ++) {
    	char letter = keywordV.at(index);
    	// No duplicate letters in square!
    	if(letterPlace.find(letter) == letterPlace.end()) {
	    	letterPlace[letter] = lettersUsed;
	    	key[getRow(lettersUsed)][getColumn(lettersUsed)] = letter;
	    	lettersUsed ++;
	    }
    }
    // Add the remaining letters to square
    for(int i = 65; i < 91; i ++) {
    	// No letter J
    	if(i == letterOmit) continue;
    	char letter(i);
    	// No duplicate letters in square!
    	if(letterPlace.find(letter) == letterPlace.end()) {
    		letterPlace[letter] = lettersUsed;
    		key[getRow(lettersUsed)][getColumn(lettersUsed)] = letter;
    		lettersUsed ++;
    	}
    }
    return 0;
}

/**
 * @brief   Encrypts digram 
 * 
 * @return 	Encrypted digram as char[2]
 */
char* Key::encryptDigram(char a, char b) {
	char *digram = new char[2];
	int aPos = letterPlace.at(a);
	int bPos = letterPlace.at(b);
	int aRow = getRow(aPos);
	int aCol = getColumn(aPos);
	int bRow = getRow(bPos);
	int bCol = getColumn(bPos);

	if(aRow == bRow) {
		digram[0] = key[aRow][(aCol + 1) % 5];
		digram[1] = key[bRow][(bCol + 1) % 5];
	} else if(aCol == bCol) {
		digram[0] = key[(aRow + 1) % 5][aCol];
		digram[1] = key[(bRow + 1) % 5][bCol];
	} else {
		digram[0] = key[aRow][bCol];
		digram[1] = key[bRow][aCol];
	}
	return digram;
}

/**
 * @brief   Decrypts digram 
 * 
 * @return 	Decrypted digram as char[2]
 */
char *Key::decryptDigram(char a, char b){
	char *digram = new char[2];
	int aPos = letterPlace.at(a);
	int bPos = letterPlace.at(b);
	int aRow = getRow(aPos);
	int aCol = getColumn(aPos);
	int bRow = getRow(bPos);
	int bCol = getColumn(bPos);

	if(aRow == bRow) {
		digram[0] = key[aRow][(aCol + 4) % 5];
		digram[1] = key[bRow][(bCol + 4) % 5];
	} else if(aCol == bCol) {
		digram[0] = key[(aRow + 4) % 5][aCol];
		digram[1] = key[(bRow + 4) % 5][bCol];
	} else {
		digram[0] = key[aRow][bCol];
		digram[1] = key[bRow][aCol];
	}
	return digram;
}

int Key::getRow(int place) {
	return (place / 5);
}
int Key::getColumn(int place) {
	return (place % 5);
}

