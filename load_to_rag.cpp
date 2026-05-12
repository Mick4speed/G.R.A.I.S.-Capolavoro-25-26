#include <iostream>
#include <string>
#include <fstream>
#include <sqlite3.h>
#include "RAG_Memory.h"
#include "load_to_rag.h"
using namespace std;

static int callback_copy_to_string(void* vector_output, int count, char** data, char** columns) {
    if (count > 0) *((string*)vector_output)=(data[0]);
    return 0;
}

static int callback_copy_to_string_vector(void* vector_output, int count, char** data, char** columns) {
    if (count > 0) {
        string copy = data[0];
        if (copy.empty())((vector<string>*) vector_output)->push_back(" ");
        else ((vector<string>*)vector_output)->push_back(data[0]);
    }
    return 0;
}

void loadBibleFromTxt(string folder) {
    RAG_Memory rag = RAG_Memory();
    ifstream file;
    string capitolo, libro;
    for (int i = 1;i <= 66;i++) {
        file = ifstream(folder+"bible()" + to_string(i) + ").txt");
        string row;
        file >> libro >> capitolo;
        while (getline(file, row)) {
            if (isdigit(row[0])) {
                string num = row.substr(0, row.find_first_of(' '));
                string text = row.substr(row.find_first_of(' '), string::npos);
                rag.saveChunk(text, libro + " " + capitolo + ":" + num, 100);
            }
            else {
                capitolo = row.substr(row.find_first_of(' '), string::npos);
            }
        }
        file.close();
    }
    rag.saveMemory();
}

void loadFromPreChunkedDatabase(string databasePath) {
    int index = databasePath.find_last_of('.');
    string dataBaseIDFile = databasePath;
    dataBaseIDFile.replace(index, string::npos, "_lastParsedId.txt");
    sqlite3 *db;
    int rc = sqlite3_open(databasePath.c_str(), &db);
    if (rc != SQLITE_OK) {
        cerr << "SQLITE error: " << sqlite3_errmsg(db) << endl;
        return;
    }
    int lastID;
    ifstream(dataBaseIDFile) >> lastID;
    ofstream idFile = ofstream(dataBaseIDFile);
    RAG_Memory rag = RAG_Memory();
    string text = "";
    string sql;
    while(true) {
        sql = "SELECT text FROM corpus WHERE _id=" + to_string(lastID) + ";";
        int rc = sqlite3_exec(db, sql.c_str(), callback_copy_to_string, &text, nullptr);
        if (rc != SQLITE_OK) {
            cerr << "SQLITE error: " << sqlite3_errmsg(db) << endl;
            return;
        }
        if (text=="") break;
        if (text.empty()) continue;
        rag.saveChunk(text, "ms-marco", 70);
        rag.saveMemory();
        idFile.seekp(0);
        idFile << lastID;
        idFile.flush();
        cout << "ID: " << lastID << " parsed" << endl;
        lastID++;
    }
}