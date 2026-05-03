#include <iostream>
#include <cstdlib>
#include "Brain.h"
#include "interface.h"
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
            cout << ">";
            string user_input;
            getline(cin, user_input);
            if (user_input == "exit") {
                cout << "R2-D2: Arrivederci!" << endl;
                break;
            }
            string response = r2d2.execPrompt(user_input);
            cout << "R2-D2: " << response << endl;
		}
	}
	catch (exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
    //exit
    return 0;
}