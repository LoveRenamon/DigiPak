#pragma once

#include "Pak.h"

class GenericPak : public Pak {
private:

	virtual void populate(std::ifstream& inputFILE) override;
public:
	GenericPak() {};
	void exportFile(int f);
	void exportAll(std::string& filename);
	virtual void import(std::string& dir, std::string& pakFilename) override;

};