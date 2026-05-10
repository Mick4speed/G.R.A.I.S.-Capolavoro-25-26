#include <iostream>
#include <string>
#include <fstream>
#include "RAG_Memory.h"
#include "llama.h"
#include "Math.h"
#include <sqlite3.h>
#include <hnswlib/hnswlib.h>
using namespace std;

inline bool file_exists(string name) {
	ifstream f(name.c_str());
	return f.good();
}

//In case of multiple result the only latest will be be saved, see sqlite3_exec documentation for clarification
static int callback_copy_to_pair(void* pair_output, int count, char** data, char** columns) {
	if (count < 2) return 0;
	if(data[0]!=NULL) ((pair<string, string>*)pair_output)->first = data[0];
	if(data[1] != NULL) ((pair<string, string>*)pair_output)->second = data[1];
	return 0;
}

//copy to an int
static int callback_copy_to_int(void* int_output, int count, char** data, char** columns) {
	if (count == 0) return 0;
	if (data[0] != NULL) *((int*)int_output) = atoi(data[0]);
	return 0;
}

//write to a bool true 
static int callback_exists(void* bool_output, int count, char** data, char** columns) {
	if (count == 0) return 0;
	*((bool*)bool_output) = true;
	return 0;
}

//write first column into a std::vector<string>
static int callback_copy_to_string_vector(void* vector_output, int count, char** data, char** columns) {
	if (count > 0) ((vector<string>*)vector_output)->push_back(data[0]);
	return 0;
}

//write first column into a std::vector<string>
static int callback_copy_to_int_vector(void* vector_output, int count, char** data, char** columns) {
	if (count > 0) ((vector<int>*)vector_output)->push_back(atoi(data[0]));
	return 0;
}

bool RAG_Memory::existsIndex(std::vector<float>* query) {
	if (hnswIndex->getCurrentElementCount() == 0) return false;
	vector<pair<float, hnswlib::labeltype>> KNN = hnswIndex->searchKnnCloserFirst(query->data(), 1);
	if (KNN.size() == 0) return false;
	return KNN[0].first <= 0.1;
}

bool RAG_Memory::existsSQLite(int id) {
	bool result;
	string sql = "SELECT ID FROM memory WHERE ID=" + to_string(id);
	int rc = sqlite3_exec(db, sql.c_str(), callback_exists, &result, nullptr);
	if (rc != SQLITE_OK) {
		cerr << "SQLITE error: " << sqlite3_errmsg(db) << endl;
		return false;
	}
	return result;
}

bool RAG_Memory::loadSQLite() {
	int rc = sqlite3_open("memory.db", &db);
	if (rc != SQLITE_OK) {
		cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
		return false;
	}
	rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS memory (ID INTEGER PRIMARY KEY, source TEXT, chunk TEXT, importance INTEGER);", nullptr, nullptr, nullptr);
	if (rc != SQLITE_OK) {
		cerr << "Cannot create table: " << sqlite3_errmsg(db) << std::endl;
		return false;
	}
	return true;
}

bool RAG_Memory::loadIndex() {
	try{
		if (file_exists("index.bin")) hnswIndex = new hnswlib::HierarchicalNSW<float>(space, "index.bin");
		else hnswIndex = new hnswlib::HierarchicalNSW<float>(space, MAX_ELEMENTS, NODE_NEIGHBORS, EF_CONSTRUCTION, true);
		hnswIndex->setEf(200);
		return true;
	}
	catch (exception& e) {
		cerr << "Error loading HNSW index: " << e.what() << endl;
		return false;
	}
}

bool RAG_Memory::loadEmbedder() {
	try{
		llama_backend_init();
		llama_model_params parameters = llama_model_default_params();
		parameters.n_gpu_layers = GPU_LAYER;
		model = llama_load_model_from_file(modelPath.c_str(), parameters);
		llama_context_params ctx_param = llama_context_default_params();
		ctx_param.n_ctx = 8192;
		ctx_param.n_batch = 8192;
		ctx_param.embeddings = true;
		ctx_param.pooling_type = LLAMA_POOLING_TYPE_CLS;
		//ctx_param.n_threads = 8;
		ctx = llama_new_context_with_model(model, ctx_param);
		return true;
	}
	catch (const std::exception& e) {
		cerr << "Error while loading embedder: " << e.what() << endl;
		return false;
	}
}

bool RAG_Memory::closeSQLite() {
	if (sqlite3_close(db) != SQLITE_OK) {
		cerr << "Cannot close database: " << sqlite3_errmsg(db) << std::endl;
		return false;
	}
	return true;
}

bool RAG_Memory::saveIndex() {
	try{
		hnswIndex->saveIndex("index.bin");
		return true;
	}
	catch (exception& e) {
		cerr << "Error saving HNSW index: " << e.what() << endl;
		return false;
	}
}

bool RAG_Memory::insertToSQLite(int id, string chunk, string source, int importance) {
	string sql = "INSERT INTO memory (ID, chunk, source, importance) VALUES (" + to_string(id) + ", '" + chunk + "', '" + source + "', + "+to_string(importance) + ");";
	int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
	if (rc != SQLITE_OK) {
		cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
		return false;
	}
	return true;
}

bool RAG_Memory::insertToIndex(int id, vector<float>* embedding) {
	try {
		hnswIndex->addPoint(embedding->data(), id);
		return true;
	}
	catch (exception e) {
		cerr << "Index Error: " << e.what() << endl;
		return false;
	}
}

bool RAG_Memory::removeFromSQLite(int id) {
	string sql = "DELETE FROM memory WHERE ID=" + to_string(id)+";";
	int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
	if (rc != SQLITE_OK) {
		cerr << "SQLITE error: " << sqlite3_errmsg(db) << endl;
		return false;
	}
	return true;
}

bool RAG_Memory::removeFromIndex(int id) {
	try {
		if (hnswIndex) {
			hnswIndex->markDelete(id);
			return true;
		}
		return false;
	}
	catch (const std::exception& e) {
		cerr << "Index error: " << e.what() << endl;
		return false;
	}
}

vector<int> RAG_Memory::searchIndex(vector<float>* query) {
	if (hnswIndex->getCurrentElementCount() == 0)
		return {};
	vector<pair<float, hnswlib::labeltype>> KNN = hnswIndex->searchKnnCloserFirst(query->data(), MAX_ELEMENT_RETURN);
	vector<int> result;
	for (auto i : KNN) {
		if (i.first < 0.9) {
			result.push_back(i.second);
		}
	}
	return result;
}

pair<string, string> RAG_Memory::getChunkByID(int id) {
	string sql = "SELECT source, chunk FROM memory WHERE ID="+to_string(id)+";";
	pair<string, string> result;
	int rc = sqlite3_exec(db, sql.c_str(), callback_copy_to_pair, &result, nullptr);
	if (rc != SQLITE_OK) {
		cerr << "SQLITE error = " << sqlite3_errmsg(db) << endl;
		return pair<string, string>();
	}
	return result;
}

int RAG_Memory::getNextID() {
	int max_id=0;
	int rc = sqlite3_exec(db, "SELECT COALESCE(MAX(ID), 0) FROM memory;", callback_copy_to_int, &max_id, nullptr);
	if (rc != SQLITE_OK) {
		cerr << "SQLITE error: " << sqlite3_errmsg(db) << endl;
		return -1;
	}
	return max_id + 1;
}

vector<float> RAG_Memory::embedString(string chunk) {
	if (!ctx) return {};
	const llama_vocab* vocab = llama_model_get_vocab(model);
	// 1. Tokenizzazione
	int n_tokens = abs(llama_tokenize(vocab, chunk.c_str(), (int)chunk.size(), nullptr, 0, true, false));
	vector<llama_token> tokens(n_tokens);
	llama_tokenize(vocab, chunk.c_str(), (int)chunk.size(), tokens.data(), n_tokens, true, false);

	// 2. Batch (Ricorda: n_tokens deve essere impostato!)
	llama_batch batch = llama_batch_init(n_tokens, 0, 1);

	for (int i = 0; i < n_tokens; i++) {
		batch.token[i] = tokens[i];
		batch.pos[i] = i;
		batch.n_seq_id[i] = 1;
		batch.seq_id[i][0] = 0;
		batch.logits[i] = true; // Attiviamo per sicurezza su tutti, il pooling CLS farà il resto
	}
	batch.n_tokens = n_tokens;

	if (llama_encode(ctx, batch) != 0) return {};

	// 4. Recupero Embedding
	// Prova prima _ith(ctx, 0) perché è un modello BERT (token CLS)
	float* embd = llama_get_embeddings_seq(ctx, 0);

	if (embd == nullptr) {
		embd = llama_get_embeddings_ith(ctx, n_tokens-1); // Fallback al pooling globale
	}

	if (embd == nullptr) {
		cerr << "Embedding NULL dopo encode!" << endl;
		llama_batch_free(batch);
		return {};
	}

	int n_embd = llama_n_embd(model);
	vector<float> res(embd, embd + n_embd);
	// 5. Cleanup e Ritorno
	llama_batch_free(batch);
	Math::normalize_vector(&res);

	return res;
}

bool RAG_Memory::saveChunk(std::string chunk, std::string source, int importance) {
	vector<float> embed = embedString(chunk);
	if (existsIndex(&embed)) {
		cout << "ERROR EMBED ALREADY EXISTS" << endl;
		return true;
	}
	cout << "EMBED DOESN'T EXIST" << endl;
	int id = getNextID();
	if (!(insertToIndex(id, &embed) && insertToSQLite(id, chunk, source, importance))) {
		cerr << "Error while inserting data chunk: "<<chunk<<endl;
		return false;
	}
	return true;
}

vector<string> RAG_Memory::search(string query) {
	vector<float> embeddedQuery = embedString(query);
	vector<int> ids = searchIndex(&embeddedQuery);
	vector<string> result;
	for (int id : ids) {
		pair<string, string> retrieved = getChunkByID(id);
		result.push_back("Source: " + retrieved.first + ", " + retrieved.second);
	}
	return result;
}

bool RAG_Memory::saveMemory() {
	return saveIndex();
}

int RAG_Memory::getCount() {
	int count;
	int rc = sqlite3_exec(db, "SELECT COUNT(*) FROM memory;", callback_copy_to_int, &count, nullptr);
	if (rc != SQLITE_OK) {
		cerr << "SQLITE error: " << sqlite3_errmsg(db) << endl;
		return -1;
	}
	return count;
}

int RAG_Memory::freeMemory() {
	int idCount= getCount() - (MAX_ELEMENTS * 0.8);
	if (idCount< 0) return 0;
	string sql = "SELECT ID FROM memory ORDER BY importance ASC LIMIT" + to_string(idCount) + ";";
	vector<int> idsToDelete;
	int rc = sqlite3_exec(db, sql.c_str(), callback_copy_to_int_vector, &idsToDelete, nullptr);
	if (rc != SQLITE_OK) {
		cerr << "SQLITE error: " << sqlite3_errmsg(db) << endl;
		return -1;
	}
	for (int i : idsToDelete) {
		removeFromSQLite(i);
		removeFromIndex(i);
	}
	return idCount;
}

RAG_Memory::RAG_Memory() {
	space = new hnswlib::InnerProductSpace((size_t)VECTOR_DIMENSION);
	if (!loadSQLite() || !loadIndex() || !loadEmbedder()) {
		cerr << "Error while trying to read memory" << endl;
	}
	cout << "Memory loaded correctly!" << endl;
}

RAG_Memory::~RAG_Memory() {
	saveMemory();
	closeSQLite();
	if(space) delete space;
	if(hnswIndex) delete hnswIndex;
	if (model) llama_free_model(model);
	if(ctx)llama_free(ctx);
}