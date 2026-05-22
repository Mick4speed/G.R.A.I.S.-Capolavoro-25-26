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
        cout << "Loading G.R.A.I.S." << endl;
        Brain grais;

        system("cls");
        cout << "--- G.R.A.I.S. ONLINE ---" << endl;
        string user_input="";
        do{
            string response;
            if (!user_input.empty()) {
                response = grais.execPrompt(user_input);
                cout << "G.R.A.I.S.> " << response << endl;
            }
            cout << "You>";
            getline(cin, user_input);
        } while (user_input != "exit");
        cout << "G.R.A.I.S.> Bye!" << endl;
    }
	catch (exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
    return 0;
}