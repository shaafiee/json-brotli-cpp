
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>

char VMAJOR = 0;
char VMINOR = 1;
char VPATCH = 2;

//#include "common/constants.h"
//#include "common/version.h"
#include <brotli/decode.h>
#include <brotli/encode.h>

//#include "enc/encode.c"

struct DataPart {
	std::string name;
	std::string file;
	
	std::string source;
	unsigned short fileId;

	bool sourceLoaded = false;
};

struct PartLength {
	int manifest;
	int lottie;
	int media;
	int script;
	int manifestHead;
	int lottieHead;
	int mediaHead;
	int scriptHead;
} partLength;

struct File {
	unsigned short ident;


} *currentFile;

struct Params {
	unsigned short ident;
	unsigned char compress;
	bool manifestSet;

	std::string manifestFile;
	std::string manifest;

	std::string dotLottieFile;

	std::vector<DataPart> lottie;
	std::vector<DataPart> media;
	std::vector<DataPart> script;
} currentParams;

std::string manifestFile;
unsigned short lastFileId = 0;

enum Reading {
	COMPRESS = 0,
	LOTTIE = 1,
	MEDIA = 2,
	MANIFEST = 3,
	SCRIPT = 4 
};

unsigned short ident;
unsigned int lengthSoFar;

int readString(std::ifstream* FH, int length, std::string* newString) {
	int currentPosition = 0;

	if (length > 0) {
		while (FH->good() && (currentPosition < length)) {
			newString->push_back(FH->get());
			currentPosition++;
		}
	} else {
		while (FH->good() && ! FH->eof()) {
			newString->push_back(FH->get());
			currentPosition++;
		}
	}

	return 1;
}

char* readChar(std::ifstream* FH, int length) {
	char buffer[length + 1];

	for (int i = 0; i < length; i++) {
		if (FH->good()) {
			buffer[i] = FH->get();
		} else {
			break;
		}
	}

	buffer[length] = '\0';

	return strdup(buffer);
}

int readOldFile(std::string oldFile) {
	if (! std::filesystem::exists(oldFile)) {
		return 0;
	}
	char *readBuffer;

	std::ifstream fileHandle(oldFile);

	if (fileHandle.good()) {
	} else {
		return 0;
	}

	readBuffer = readChar(&fileHandle, 2);
	//int fileVersion = 

	return 1;
}

int splitNameFile(struct DataPart* passedPart, char* argument) {
	std::string tempString(argument);
	std::size_t found = tempString.find("=");
	if (found == std::string::npos) {
		return 0;
	}

	passedPart->name = tempString.substr(0, found);
	passedPart->file = tempString.substr((found + 1), (tempString.length() - found));

	return 1;
}

int readEntireFile(std::string path, std::string* passedString) {
	if (! std::filesystem::exists(path)) {
		return 0;
	}

	std::ifstream fileHandle(path);
	readString(&fileHandle, 0, passedString);
	fileHandle.close();

	return 1;
}

int prepareDataParts() {
	for (int i = 0; i < currentParams.lottie.size(); i++) {
		if (! currentParams.lottie[i].sourceLoaded) {
			readEntireFile(currentParams.lottie[i].file, &(currentParams.lottie[i].source));
			currentParams.lottie[i].sourceLoaded = true;
			lastFileId++;
			currentParams.lottie[i].fileId = lastFileId;
		}
		partLength.lottie += currentParams.lottie[i].source.length();
		partLength.lottieHead += 11;
	}
	for (int i = 0; i < currentParams.media.size(); i++) {
		if (! currentParams.media[i].sourceLoaded) {
			readEntireFile(currentParams.media[i].file, &(currentParams.media[i].source));
			currentParams.media[i].sourceLoaded = true;
			lastFileId++;
			currentParams.media[i].fileId = lastFileId;
		}
		partLength.media += currentParams.media[i].source.length();
		partLength.mediaHead += 11;
	}
	for (int i = 0; i < currentParams.script.size(); i++) {
		if (! currentParams.script[i].sourceLoaded) {
			readEntireFile(currentParams.script[i].file, &(currentParams.script[i].source));
			currentParams.script[i].sourceLoaded = true;
			lastFileId++;
			currentParams.script[i].fileId = lastFileId;
		}
		partLength.script += currentParams.script[i].source.length();
		partLength.scriptHead += 11;
	}

	return 1;
}

int writeToFile() {
	//std::cout << "done 0.0.1 \n";
	std::ofstream fileHandle(currentParams.dotLottieFile, std::ofstream::binary);
	//std::cout << "done 0.0.2 \n";

	char partType;
	unsigned int offset;
	unsigned int offsetBeforePartDesc;
	unsigned int length;
	unsigned int partDescLength;

	if (! fileHandle) {
		perror("Could not create target dotLottie file");
	}

	lengthSoFar += sizeof(currentParams.ident);
	//fileHandle.write((char *)(currentParams.ident), sizeof(currentParams.ident));
	fileHandle << currentParams.ident;

	lengthSoFar += 4;
	fileHandle.put(currentParams.compress);
	fileHandle.put(VMAJOR);
	fileHandle.put(VMINOR);
	fileHandle.put(VPATCH);
	//std::cout << "done 0.1 \n";

	lengthSoFar += 16;
	offset = lengthSoFar;
	//fileHandle.write((char *)offset, sizeof(offset));
	fileHandle << offset;
	//fileHandle.write((char *)partLength.manifest, sizeof(partLength.manifest));
	fileHandle << partLength.manifest;

	lengthSoFar += partLength.manifest;
	offset = lengthSoFar;
	//fileHandle.write((char *)offset, sizeof(offset));
	fileHandle << offset;
	partDescLength = (currentParams.lottie.size() * 9) + (currentParams.media.size() * 9) + (currentParams.script.size() * 9);
	//fileHandle.write((char *)length, sizeof(length));
	fileHandle << length;

	fileHandle.write(currentParams.manifest.c_str(), currentParams.manifest.size());

	//std::cout << "done 0.1 \n";
	lengthSoFar += partDescLength;
	offset = lengthSoFar;
	offsetBeforePartDesc = offset;
	partType = 1;
	fileHandle.put(partType);
	//fileHandle.write((char *)offset, sizeof(offset));
	fileHandle << offset;
	length = (partLength.lottie + partLength.lottieHead);
	//fileHandle.write((char *)length, sizeof(length));
	fileHandle << length;
	if (currentParams.media.size() > 0) {
		offset += (partLength.lottie + partLength.lottieHead);
		length = (partLength.media + partLength.mediaHead);
		//fileHandle.write((char *)offset, sizeof(offset));
		fileHandle << offset;
		//fileHandle.write((char *)length, sizeof(length));
		fileHandle << length;
	}
	if (currentParams.script.size() > 0) {
		offset += (partLength.media + partLength.mediaHead);
		length = (partLength.media + partLength.mediaHead);
		//fileHandle.write((char *)offset, sizeof(offset));
		fileHandle << offset;
		//fileHandle.write((char *)length, sizeof(length));
		fileHandle << length;
	}

	//std::cout << "done 1.0 \n";
	std::string payloadData;
	offset = offsetBeforePartDesc;
	payloadData += currentParams.dotLottieFile;
	payloadData += ".temp";
	std::ofstream PH(payloadData, std::ofstream::binary);
	//std::cout << "done 1.1 \n";
	for (int i = 0; i < currentParams.lottie.size(); i++) {
		offset += 11;
		//PH.write((char *)offset, sizeof(offset));
		PH << offset;
		length = currentParams.lottie[i].source.length();
		//PH.write((char *)length, sizeof(length));
		PH << length;
		//PH.write((char *)currentParams.lottie[i].fileId, sizeof(currentParams.lottie[i].fileId));
		PH << currentParams.lottie[i].fileId;
		PH.write(currentParams.lottie[i].source.c_str(), currentParams.lottie[i].source.length());
		offset += currentParams.lottie[i].source.length();
	}
	if (currentParams.media.size() > 0) {
		for (int i = 0; i < currentParams.media.size(); i++) {
			offset += 11;
			//PH.write((char *)offset, sizeof(offset));
			PH << offset;
			length = currentParams.media[i].source.length();
			//PH.write((char *)length, sizeof(length));
			PH << length;
			//PH.write((char *)currentParams.media[i].fileId, sizeof(currentParams.media[i].fileId));
			PH << currentParams.media[i].fileId;
			PH.write(currentParams.media[i].source.c_str(), currentParams.media[i].source.length());
			offset += currentParams.media[i].source.length();
		}
	}
	if (currentParams.script.size() > 0) {
		for (int i = 0; i < currentParams.script.size(); i++) {
			offset += 11;
			//PH.write((char *)offset, sizeof(offset));
			PH << offset;
			length = currentParams.script[i].source.length();
			//PH.write((char *)length, sizeof(length));
			PH << length;
			//PH.write((char *)currentParams.script[i].fileId, sizeof(currentParams.script[i].fileId));
			PH << currentParams.script[i].fileId;
			PH.write(currentParams.script[i].source.c_str(), currentParams.script[i].source.length());
			offset += currentParams.script[i].source.length();
		}
	}
	PH.close();
	//std::cout << "done 1.2 \n";

	std::string payloadUncompressed;
	readEntireFile(payloadData, &payloadUncompressed);

	if (currentParams.compress == 0) {
		std::remove(payloadData.c_str());
		fileHandle.write(payloadUncompressed.c_str(), payloadUncompressed.length());
		fileHandle.close();
		std::cout << "Done.\n";
		return 1;
	}

	unsigned char payloadCompressed[20000000];
	size_t compressedSize = 20000000;
	
	size_t uncompressedSize = payloadUncompressed.length();
	unsigned char payloadUncompressedChar[uncompressedSize + 1];
	payloadUncompressedChar[uncompressedSize] = '\0';
	for (int i = (uncompressedSize - 1); i >= 0; i--) {
		payloadUncompressedChar[i] = payloadUncompressed.at(i);
		//std::cout << payloadUncompressed[i];
		payloadUncompressed.pop_back();
	}

	//std::cout << "\ndone 1.3 \n";
	//std::cout << uncompressedSize << "\n";

	std::remove(payloadData.c_str());
	//std::cout << "done 1.4 \n";
	bool brotliStatus = BrotliEncoderCompress(11, BROTLI_MAX_WINDOW_BITS, BROTLI_MODE_GENERIC, uncompressedSize, payloadUncompressedChar, &compressedSize, payloadCompressed);
	//std::cout << brotliStatus << " - SIZE " << compressedSize << " done 1.5 \n";

/*
		BROTLI_ENC_API BROTLI_BOOL BrotliEncoderCompress(
    int quality, int lgwin, BrotliEncoderMode mode, size_t input_size,
    const uint8_t input_buffer[BROTLI_ARRAY_PARAM(input_size)],
    size_t* encoded_size,
    uint8_t encoded_buffer[BROTLI_ARRAY_PARAM(*encoded_size)]);
*/
	//fileHandle.write(payloadCompressed, compressedSize);
	//std::cout << compressedSize << "\n";
	for (int i = 0; i < compressedSize; i++) {
		fileHandle.put(payloadCompressed[i]);
		//std::cout << payloadCompressed[i];
	}
	//std::cout << "\n";

	fileHandle.close();	
	std::cout << "Done.\n";
	return 1;	
}

int prepareManifest() {
	std::size_t position = currentParams.manifest.rfind("}");

	std::string fileList;

	fileList = ",\n\"lottie\":[";
	for (int i = 0; i < currentParams.lottie.size(); i++) {
		fileList += currentParams.lottie[i].fileId;
		fileList += ",\"";
		fileList += currentParams.lottie[i].name;
		fileList += "\"";
		if (i < (currentParams.lottie.size() - 1)) {
			fileList += ",";
		}
	}
	fileList += "]";
	if (currentParams.media.size() > 0) {
		fileList += ",\n";
		fileList += "\"media\":[";
		for (int i = 0; i < currentParams.media.size(); i++) {
			fileList += currentParams.media[i].fileId;
			fileList += ",\"";
			fileList += currentParams.media[i].name;
			fileList += "\"";
			if (i < (currentParams.media.size() - 1)) {
				fileList += ",";
			}
		}
		fileList += "]";
	}
	if (currentParams.script.size() > 0) {
		fileList += ",\n";
		fileList += "\"script\":[";
		for (int i = 0; i < currentParams.script.size(); i++) {
			fileList += currentParams.script[i].fileId;
			fileList += ",\"";
			fileList += currentParams.script[i].name;
			fileList += "\"";
			if (i < (currentParams.script.size() - 1)) {
				fileList += ",";
			}
		}
		fileList += "]";
	}

	currentParams.manifest.insert(position, fileList);
	partLength.manifest = currentParams.manifest.length();

	return 1;
}

void printHelp() {
	std::cout << "Usage:\n";
	std::cout << "\tdotlottie2 COMMAND SAVEFILE --compress COMPRESS [--manifest MANIFEST] --lottie name=file [name2=file2 name3=file3 ...] [--media name4=file4 [name5=file5 name6=file6 ...]] [--script name7=file7 [name8=file8 name9=file9 ...]]\n\n";
	std::cout << "\tCOMMAND : either 'add' or 'remove'\n";
	std::cout << "\tSAVEFILE : new dotLottie file to be created\n";
	std::cout << "\tCOMPRESS : '0' for uncompressed and '1' for Brotli\n\n";
	std::cout << "Important: the filenames (name=) should always be unique\n";
}

int main(int argc, char* argv[]) {

	currentParams.ident = 7311;
	currentParams.ident = currentParams.ident << 3;

	if (argc < 4) {
		perror("Not enough arguments\n");
		printHelp();
		return -1;
	}

	// current file crud

	struct DataPart dataPart;
	enum Reading currentState;
	currentParams.dotLottieFile += argv[2];

	for (int i = 3; i < argc; i++) {
		if (strcmp(argv[i], "--compress") == 0) {
			currentState = COMPRESS;
		} else if (strcmp(argv[i], "--manifest") == 0) {
			currentState = MANIFEST;
		} else if (strcmp(argv[i], "--lottie") == 0) {
			currentState = LOTTIE;
		} else if (strcmp(argv[i], "--media") == 0) {
			currentState = MEDIA;
		} else if (strcmp(argv[i], "--script") == 0) {
			currentState = SCRIPT;
		} else {
			switch (currentState) {
				case COMPRESS:
					currentParams.compress = atoi(argv[i]);
					break;
				case LOTTIE:
					if (splitNameFile(&dataPart, argv[i]) == 1) {
						currentParams.lottie.push_back(dataPart);
					}
					break;
				case MEDIA:
					if (splitNameFile(&dataPart, argv[i]) == 1) {
						currentParams.media.push_back(dataPart);
					}
					break;
				case SCRIPT:
					if (splitNameFile(&dataPart, argv[i]) == 1) {
						currentParams.script.push_back(dataPart);
					}
					break;
				case MANIFEST:
					currentParams.manifestFile = argv[i];
					readEntireFile(argv[i], &(currentParams.manifest));
					partLength.manifest = currentParams.manifest.length();
					partLength.manifestHead = 8;
					break;
			}
		}
	}

	if (currentParams.lottie.size() < 1) {
		perror("No lotties supplied");
		printHelp();
		return -1;
	}

	//std::cout << "toast 1.0\n";
	prepareDataParts();
	//std::cout << "toast 1.1\n";
	if (currentParams.manifest.length() < 3) {
		currentParams.manifest += "{\n\t\"generator\":\"Lottiefiles 2.0\",\n\t\"version\":2.0\n}";
		partLength.manifest = currentParams.manifest.length();
		partLength.manifestHead = 8;
	}
	prepareManifest();
	//std::cout << "toast 1.2\n";

	if (strcmp(argv[1], "add")) {
		writeToFile();
	} else if (strcmp(argv[1], "remove")) {
	} else {
		perror("Unsupported command\n");
		return 0;
	}

	//std::cout << "toast 1.3\n";

	return 1;
}

