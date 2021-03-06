#include "GenericPak.h"

bool alphaNumericSort(const std::string& a, const std::string& b) {
	//Find the position of the number
	int aPos = -1;
	int aPosEnd = a.size() - 4; //Exclude ".bin"

	for (unsigned int p = aPosEnd - 1; p > 0; p--) {
		if (!isdigit(a[p])) {
			aPos = p + 1;
			break;
		}
	}
	int aNum = std::stoi(a.substr(aPos, aPosEnd - aPos));
	int bPos = -1;
	int bPosEnd = b.size() - 4; //Exclude ".bin"

	for (unsigned int p = bPosEnd - 1; p > 0; p--) {
		if (!isdigit(b[p])) {
			bPos = p + 1;
			break;
		}
	}
	int bNum = std::stoi(b.substr(bPos, bPosEnd - bPos));
	if (aNum < bNum) {
		return true;
	}
	else {
		return false;
	}
}

void GenericPak::populate(std::ifstream& inputFILE) {
	files.resize(header.numOfFiles); //We know a priori the number of files in this PKG.

									 //Populate Pointer List
	for (auto& file : files) {
		inputFILE.read(reinterpret_cast<char*>(&file.fileOffset), sizeof(file.fileOffset));
		inputFILE.read(reinterpret_cast<char*>(&file.unCompressedSize), sizeof(file.unCompressedSize));
		inputFILE.read(reinterpret_cast<char*>(&file.size), sizeof(file.size));
		inputFILE.read(reinterpret_cast<char*>(&file.bCompressed), sizeof(file.bCompressed));
	}

	for (auto& file : files) {

		inputFILE.seekg(file.fileOffset);
		file.data = std::make_unique<char[]>(file.size);
		inputFILE.read(file.data.get(), file.size);
	}
}
void GenericPak::exportFile(int f) {

}

void GenericPak::exportAll(std::string& filename) {
	std::ofstream outputNCLRFILE;
	fs::path thisPath(filename);
	fs::path stemName = thisPath.stem();
	fs::create_directory(stemName);
	for (unsigned int i = 0; i < files.size(); i++) {
		outputNCLRFILE.open(stemName.string() + "/" + stemName.string() + std::to_string(i) + "." + extension, std::ios::binary);
		if (files[i].bCompressed == 0x00000000) {
			std::unique_ptr<char[]> uncompressedBuffer = decompressPrototype(files[i].data.get(), files[i].unCompressedSize);
			outputNCLRFILE.write(uncompressedBuffer.get(), files[i].unCompressedSize);
		}
		else {
			outputNCLRFILE.write(files[i].data.get(), files[i].unCompressedSize);
		}

		outputNCLRFILE.close();
	}
}

void GenericPak::import(std::string& dir, std::string& outFilename) {
	std::string newFilename = dir + ".PAK";
	
	fs::path tarPath(dir);
	const fs::directory_iterator end{};

	std::ifstream fileFILE;
	
	std::vector<std::string> paths;
	for (fs::directory_iterator dirIter(tarPath); dirIter != end; dirIter++) {
		paths.emplace_back(dirIter->path().string());
	}
	//File order by alpha + numeric
	std::sort(paths.begin(), paths.end(), alphaNumericSort);
	//Pull in the data 

	for(const auto& dirFile : paths){
		NDSFile file;
		fileFILE.open(dirFile, std::ifstream::binary);
		file.unCompressedSize = static_cast<uint32_t>(fs::file_size(dirFile));
		file.size = file.unCompressedSize;
		file.bCompressed = 0x80000000;

		file.data = std::make_unique<char[]>(file.size);
		fileFILE.read(file.data.get(), file.size);
		fileFILE.close();
		files.emplace_back(std::move(file));
	}

	//Create Pak Header
	header.numOfFiles = files.size();
	header.version = 0x31302e32;
	header.junk = 0;
	header.junk2 = 0;
	std::ofstream outputPAKFILE;
	outputPAKFILE.open(newFilename, std::ios::binary);
	outputPAKFILE.write(reinterpret_cast<char*>(&header.numOfFiles), sizeof(std::uint32_t));
	outputPAKFILE.write(reinterpret_cast<char*>(&header.version), sizeof(std::uint32_t));
	outputPAKFILE.write(reinterpret_cast<char*>(&header.junk), sizeof(std::uint32_t));
	outputPAKFILE.write(reinterpret_cast<char*>(&header.junk2), sizeof(std::uint32_t));

	//Create Pointers
	std::uint32_t offset = header.numOfFiles * 16 + 16;

	for (auto& file : files) {
		file.fileOffset = offset;
		outputPAKFILE.write(reinterpret_cast<char*>(&file.fileOffset), sizeof(std::uint32_t));
		outputPAKFILE.write(reinterpret_cast<char*>(&file.size), sizeof(std::uint32_t));
		outputPAKFILE.write(reinterpret_cast<char*>(&file.unCompressedSize), sizeof(std::uint32_t));
		outputPAKFILE.write(reinterpret_cast<char*>(&file.bCompressed), sizeof(std::uint32_t));
		offset += file.unCompressedSize;
	}
	//Dump data
	for (const auto& file : files) {
		outputPAKFILE.write(file.data.get(), file.unCompressedSize);
	}

	outputPAKFILE.close();
}
