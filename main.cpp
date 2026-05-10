#include <iostream>
#include <cstdlib>
#include <hnswlib/hnswlib.h>
#include <sqlite3.h>
#include "Brain.h"
#include "RAG_Memory.h"
#include "interface.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <fstream>
#include "llama.h"
#include <iomanip>
using namespace std;

int main() {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif
    /*
    RAG_Memory rag=RAG_Memory();
    ifstream file;
    string capitolo, libro;
    for(int i = 1;i<=66;i++) {
        
        file = ifstream(R"(C:\Users\itali\Downloads\bible\bible ()" + to_string(i) +").txt");
        cout << R"(C:\Users\itali\Downloads\bible\bible ()" + to_string(i) + ").txt" << endl;
        string row;
        file >> libro >> capitolo;
        while (getline(file, row)){
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
    */
    try {
        cout << "Caricamento R2-D2 in corso..." << endl;
        Brain r2d2;

        system("cls");
        cout << "--- R2-D2 ONLINE ---" << endl;
        while (true) {
            cout << "You>";
            string user_input;
            getline(cin, user_input);
            if (user_input == "exit") {
                cout << "R2-D2: Arrivederci!" << endl;
                break;
            }
            string response = r2d2.execPrompt(user_input);
            cout << "R2-D2> " << response << endl;
		}
	}
	catch (exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
    //exit
    return 0;
}