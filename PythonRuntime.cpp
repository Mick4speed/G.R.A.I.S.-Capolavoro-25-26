#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include "PythonRuntime.h"
#include "json.hpp"
using json = nlohmann::json;
using namespace std;

#pragma region OutputRedirect
string PYTHON_OUTPUT = "";
PyObject* pythonPrintToVariable(PyObject* self, PyObject* args) {
    const char* text;
    
    if (!PyArg_ParseTuple(args, "s", &text)) {
        return NULL;
    }
    string strText(text);
    int i = strText.size() - 1;

    while (i >= 0 && strText[i] == '\n') {
        i--;
    }
    strText.resize(i + 1);

    if (!strText.empty()) {
        PYTHON_OUTPUT += strText+"\n";
        cout << "[Python Runtime] " << strText << endl;
    }
    Py_RETURN_NONE;
}

static PyMethodDef catcherMethods[] = {
    {"printToCPP", pythonPrintToVariable, METH_VARARGS, "Save STDOUT to C++ String"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef catcherModuleDef = {
    PyModuleDef_HEAD_INIT,
    "embed_catcher",
    NULL,
    -1,
    catcherMethods
};

PyMODINIT_FUNC init_embed_catcher(void) {
    return PyModule_Create(&catcherModuleDef);
}
#pragma endregion

#pragma region private
PyModule PythonRuntime::loadTool(string fileName) {
    PyModule err;
    err.name = "Error";
    try{
        ifstream file("./tools/" + fileName);
        string description;
        string argsList;
        getline(file, description);
        getline(file, argsList);
        if (description[0] != '#' || argsList[0] != '#') {
            err.description = "File malformed";
            return err;
        }
        description.erase(0, 1);
        argsList.erase(0, 1);
        PyModule tool;
        tool.description = description;
        tool.name = fileName.substr(0, fileName.find('.'));
        int startIndex = 0;
        int endIndex;
        do {
            endIndex = argsList.find(' ', startIndex);
            if (endIndex == string::npos) {
                tool.argsList.push_back(argsList.substr(startIndex));
                break;
            }
            else if (endIndex == startIndex) {
                startIndex++;
                continue;
            }
            tool.argsList.push_back(argsList.substr(startIndex, endIndex-startIndex));
            startIndex = endIndex + 1;
        } while (startIndex < argsList.size());
        return tool;
    }
    catch (exception e) {
        err.description = e.what();
        return err;
    }
}

void PythonRuntime::loadToolSet() {
    if (!filesystem::exists("./tools") || !filesystem::is_directory("./tools")) {
        this->executeString("print('''Error 'tools' folder unvailable''')");
    }
    for (const auto& entry : filesystem::directory_iterator("./tools")) {
        if (entry.is_directory()) continue;
        string filename = entry.path().filename().string();
        auto tool = this->loadTool(filename);
        if (tool.name == "Error") {
            this->executeString("print('''Error loading tool "+filename+": " + tool.description + "''')");
            continue;
        }
        this->toolset.push_back(tool);
    }
}
#pragma endregion

#pragma region public
PythonRuntime::PythonRuntime() {
    PyImport_AppendInittab("embed_catcher", init_embed_catcher);
    Py_Initialize();
    this->executeString("import sys; sys.path.append('./tools')");
    this->executeString(R"(
import sys
import embed_catcher
    
class OutputCatcher :
    def write(self, text):
        embed_catcher.printToCPP(text)
        
    def flush(self):
        pass
    
sys.stdout=OutputCatcher();
sys.stderr=OutputCatcher();
)");
    loadToolSet();
    this->executeString("print('Python Runtime Online!')");
}

string PythonRuntime::executeString(string script) {
    PYTHON_OUTPUT = "";
    PyRun_SimpleString(script.c_str());
    return PYTHON_OUTPUT;
}

string PythonRuntime::executeTool(int index, json* JSON) {
    PYTHON_OUTPUT = "";
    if (index < 0 || index >= this->toolset.size()) return this->executeString("print('Error: Tool index out of bound')");
    PyModule toolInfo = this->toolset[index];
    cout << "[Action] Executing tool: " << toolInfo.name << endl;
    cout.flush();
    PyObject* moduleName = PyUnicode_FromString(toolInfo.name.c_str());
    PyObject* toolModule = PyImport_Import(moduleName);
    Py_DECREF(moduleName);
    if (toolModule != NULL) {
        PyObject* function = PyObject_GetAttrString(toolModule, "execute");
        if (function && PyCallable_Check(function)) {
            //Load input argument
            PyObject* input = PyTuple_New(toolInfo.argsList.size());
            for (int i = 0; i < toolInfo.argsList.size(); i++) {
                string s = toolInfo.argsList[i];
                if (JSON->contains(s)) {
                    PyObject* arg = PyUnicode_FromString(JSON->value(s, "").c_str());
                    PyTuple_SetItem(input, i, arg);
                }
                else {
                    return "Python Output:" + PYTHON_OUTPUT + "\nError while retrieving input arguments from JSON";
                }
            }
            PyObject* output = PyObject_CallObject(function, input);
            Py_XDECREF(input);
            Py_XDECREF(function);
            Py_DECREF(toolModule);
            if (output != NULL) {
                if (PyUnicode_Check(output)) {
                    string result;
                    PyObject* ouputString = PyUnicode_AsUTF8String(output);

                    if (ouputString != NULL) {
                        const char* cString = PyBytes_AsString(ouputString);
                        result = string(cString);
                        Py_DECREF(ouputString);
                    }
                    return "Python Output:" + PYTHON_OUTPUT + "\nTool: " + result;
                }else if (output == Py_None) {
                    return "Python Output:" + PYTHON_OUTPUT;
                }else {
                    PyErr_Print();
                    return "Python Output:" + PYTHON_OUTPUT + "\nError while converting from python string to C++ string";
                }
            }
        }
        else {
            PyErr_Print();
            return "Python Output:" + PYTHON_OUTPUT + "\nError function 'execute' doesn't exists";
        }
    }
    else {
        PyErr_Print();
        return "Python Output:"+ PYTHON_OUTPUT+"\nError while retrieving module " + toolInfo.name;
    }
}

int PythonRuntime::getToolSetSize() {
    return this->toolset.size();
}

vector<string> PythonRuntime::getDescriptionList() {
    vector<string> result;
    for (auto i : this->toolset) {
        result.push_back(i.description);
    }
    return result;
}

PythonRuntime::~PythonRuntime() {
    Py_Finalize();
}
#pragma endregion