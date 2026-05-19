#include <iostream>
#include <string>
#include <cstdlib>
#include <hnswlib/hnswlib.h>
#include <sqlite3.h>
#include "Brain.h"
#include "RAG_Memory.h"
#include "interface.h"
#include "load_to_rag.h"
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

int main() {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif
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
                cout << "R2-D2> Bye!" << endl;
                break;
            }
            string response = r2d2.execPrompt(user_input);
            cout << "R2-D2> " << response << endl;
		}
	}
	catch (exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
    return 0;
}