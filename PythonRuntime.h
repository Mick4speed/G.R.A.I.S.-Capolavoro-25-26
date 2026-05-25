#pragma once
#include <string>
#include <vector>
#include <Python.h>
#include "json.hpp"
using json = nlohmann::json;

struct PyModule {
	std::string name;
	std::string description;
	std::vector<std::string> argsList;

};

class PythonRuntime {
private:
	//Collection of found tools
	std::vector<PyModule> toolset;
	//Method to load a single tool by its name, returns a colllection of PyModule struct, if error occur return a PyModule with field name=="Error"
	PyModule loadTool(std::string fileName);
	//Method to load the entire toolset
	void loadToolSet();
public:
	//Constructor
	PythonRuntime();
	//Execute a python script, the code is passed as a string
	std::string executeString(std::string script);
	//execute a tool loaded from the tools folder
	std::string executeTool(int index, json* JSON);
	//Get tools count
	int getToolSetSize();
	//Get the description list of the toolset
	std::vector<std::string> getDescriptionList();
	//Destructor
	~PythonRuntime();
};