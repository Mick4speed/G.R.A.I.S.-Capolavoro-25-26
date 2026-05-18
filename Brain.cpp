#include "llama.h"
#include <iostream> 
#include <fstream>
#include <sstream>
#include <string>
#include <locale>
#include <vector>
#include <boost/algorithm/string/join.hpp>
#include "Brain.h"
#include "interface.h"
#include "json.hpp"
using namespace std;
using json = nlohmann::json;

string loadFile(string path) {
	ifstream file(path);
	if (!file.is_open()) {
		cerr << "Error: Grammar file wasn't found" << endl;
		return "";
	}
	stringstream ss;
	ss << file.rdbuf();
	file.close();
	return ss.str();
}

Brain::Brain()
	:rag()
{
	//TODO remove this when we find a better way to make the user interact with the AI
	auto silent_log = [](ggml_log_level level, const char* text, void* user_data) {
		(void)level; (void)user_data;
		// Non facciamo nulla, quindi il testo non viene stampato
		};

	// 2. Diciamo a llama.cpp di usare questa funzione
	llama_log_set(silent_log, nullptr);

	llama_backend_init();
	llama_model_params model_params = llama_model_default_params();
	model_params.n_gpu_layers = gpu_layer;
	model_params.use_mmap = true;
	model = llama_model_load_from_file(modelPath.c_str(), model_params);
	if (model==NULL) {
		throw runtime_error("Failed to load the model from file: "+modelPath);
	}
}

int Brain::countTokens(llama_context* context) {
	llama_memory_t memory = llama_get_memory(context);
	return llama_memory_seq_pos_max(memory, 0) + 1;
}

void Brain::shiftContext(llama_context* context, int size_prompt) {
	llama_memory_t memory = llama_get_memory(context);
	llama_memory_seq_rm(memory, 0, size_prompt + 1, size_prompt + delete_token);
	llama_memory_seq_add(memory, 0, size_prompt, -1, -delete_token);
}

llama_sampler* Brain::getSampler() {
	llama_sampler* sampler = llama_sampler_chain_init(llama_sampler_chain_default_params()); // Initialize the sampler with default parameters
	/*string grammar = loadFile("grammar.gbnf"); //GRAMMAR DO NOT USE NOT STABLE
	if(!grammar.empty()) llama_sampler_chain_add(sampler, llama_sampler_init_grammar(llama_model_get_vocab(model), grammar.c_str(), "root"));*/
	llama_sampler_chain_add(sampler, llama_sampler_init_penalties(64, 1.1f, 0.0f, 0.0f));	
	llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.7f)); 
	llama_sampler_chain_add(sampler, llama_sampler_init_dist(time(NULL)));
	return sampler;
}

string Brain::cleanResponse(string response) {
    size_t l = response.find_first_of("{");
    size_t r = response.find_last_of("}");
    if (l == string::npos || r == string::npos || r < l) {
		return "";
    }
	response = response.substr(l, (r - l) + 1);
	string result = "";
	bool inString = false;
	bool isSpecial = false;
	for (int i = 0; i < response.size(); i++) {
		char current = response.at(i);
		if (current == '\\' && inString && (i + 1 < response.length())) {
			result += current;        
			result += response[i + 1];
			++i;
			continue;
		}
		if (current == '"') {
			inString = !inString;
			result += i;
		}else if (current == '\n'&&inString) {
			result += "\\n";
		}
		else {
			result += i;
		}

	}
	return result;
}

int Brain::getMaxContextSize(int size_prompt) {
	return 2*(size_prompt + output_token);
}

llama_context* Brain::createContext() {
	llama_context_params context_params = llama_context_default_params();;
	context_params.n_ctx = 4096; // Set the context size to accommodate the prompt and the output tokens
	context_params.n_batch = 4096; // n_batch is the maximum number of tokens that can be processed in a single call to llama_decode
	llama_context* context = llama_init_from_model(model, context_params); // Create a new context with the model and parameters
	if (context == NULL) {
		throw runtime_error("Failed to initialize the context from the model.");
	}
	return context;
}

string Brain::generateResponse(vector<llama_token> prompt_tokens, llama_context* context, llama_sampler* sampler, const llama_vocab* vocabulary) {
	string response = "";
	llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
	char buf[256];
	cout << "Thinking..." << endl;
	for (int i = 0; i < output_token; i++) {
		if (llama_decode(context, batch) != 0) {
			cerr << "Error while decoding token" << endl;
			break;
		}
		//Get next token
		llama_token token = llama_sampler_sample(sampler, context, -1);
		if (llama_token_is_eog(vocabulary, token)) break;

		int len = llama_token_to_piece(vocabulary, token, buf, sizeof(buf), 0, true);
		if (len > 0) response.append(buf, len);
		batch = llama_batch_get_one(&token, 1);
	}
	return response;
}

string Brain::execAction(int action, string JSON) {
	json jsonResponse = json::parse(JSON);
	switch (action) {
		case 0: {	//Return Text
			return jsonResponse.value("response", "");
		} case 1: { //Return data from RAG
			cout << "[Action] Retrieving Data from the RAG" << endl;
			vector<string> results = Interface::retrieveDataFromRAG(&rag, jsonResponse.value("query", ""));
			cout.flush();
			return boost::algorithm::join(results, ",");
		} case 2: {
			cout << "[Action] Retrieving Data from the internet" << endl;
			vector<string> results = Interface::retrieveDataFromInternet(&rag, jsonResponse.value("url", ""), jsonResponse.value("query", ""));
			cout.flush();
			return boost::algorithm::join(results, ",");
		}default:
				return "Unknown Action";
	}
}

vector<llama_token> Brain::tokenizeString(std::string STRING, bool is_first_turn) {
	const int size_prompt = -llama_tokenize(llama_model_get_vocab(model), STRING.c_str(), STRING.size(), NULL, 0, is_first_turn, true); //Calculate the number of tokens in the prompt

	vector<llama_token> prompt_tokens(size_prompt); // Create a vector to hold the tokenized prompt
	if (llama_tokenize(llama_model_get_vocab(model), STRING.c_str(), STRING.size(), prompt_tokens.data(), prompt_tokens.size(), is_first_turn, true) < 0) {
		throw runtime_error("Failed to tokenize the prompt: " + STRING);
	}
	return prompt_tokens;
}

string Brain::execPrompt(string prompt) {
	//Add prompt instruction and format

	const llama_vocab* vocabulary = llama_model_get_vocab(model); // Get the model's vocabulary
	llama_sampler* sampler = getSampler(); // Initialize the sampler with default parameters

	string system_prompt = "<|im_start|>system\n" + SYSTEM_PROMPT + "\n<|im_end|>";
	vector<llama_token> system_token = tokenizeString(system_prompt, true);
	int current_action = -1;
	llama_context* context = createContext();
	string response = "";
	generateResponse(system_token, context, sampler, vocabulary);
	string actual_prompt = "<|im_start|>user\n"+prompt+"<|im_end|>\n<|im_start|>assistant\n";
	do {
		vector<llama_token> prompt_tokens = tokenizeString(actual_prompt, false);
		do{
			response = generateResponse(prompt_tokens, context, sampler, vocabulary);
		} while (cleanResponse(response) == "");
		json jsonResponse = json::parse(response);
		current_action = jsonResponse["action"];
		string result = execAction(current_action, response);
		if (current_action == 0) response = result; //If the action is 0, we want to return the response to the user
		actual_prompt = "<|im_end|>\n<|im_start|>tool\n" + result + "\n<|im_end|>\n<|im_start|>assistant\n";
	} while (current_action > 0);

	
	llama_free(context);
	llama_sampler_free(sampler);
	return response;
}

Brain::~Brain() {
	if (model != nullptr) {
		llama_model_free(model);
	}
	rag.saveMemory();
	llama_backend_free();
}