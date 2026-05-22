#pragma once
#include <string>

void loadBibleFromTxt(std::string folder);
void loadFromPreChunkedDatabase(std::string databasePath, std::string dataRow, std::string table,std::string source, int importance);
void addFromExternalMemoryDB(std::string databasePath);