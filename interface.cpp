#include <iostream>
#include <string>
#include <regex>
#include <algorithm>
#include <vector>
#include "interface.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include "json.hpp"
#include "PythonRuntime.h"

#include <lexbor/html/html.h>
#include <lexbor/dom/interfaces/element.h>
#include <lexbor/html/tag.h>    

using json = nlohmann::json;
using namespace std;

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
namespace ssl = net::ssl;			// from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

string removeSpaces(string s) {
	auto new_end = remove(s.begin(), s.end(), ' ');
	s.erase(new_end, s.end());
	return s;
}

vector<string> extractHTML(string pageContent, string parseRegex) {
	vector<string> urls;
	regex pattern(parseRegex);

	auto it = std::sregex_iterator(pageContent.begin(), pageContent.end(), pattern);
	auto end = std::sregex_iterator();

	while (it != end) {
		std::smatch match = *it;
		std::string url = match[1];
		std::string titolo = match[2];
		url = regex_replace(url, regex("&amp;"), "&");
		url = regex_replace(url, regex("%2F"), "/");
		url = regex_replace(url, regex("%3A"), ":");

		if (url.find("/l/")) {
			int string_start_pos = url.find("uddg=");
			url = url.substr(string_start_pos + 5);
			urls.push_back(url);
		}
		it++;
	}
	return urls;
}

void insertChunks(RAG_Memory *rag, vector<string> individualPhrases, string link, string query) {
	if (individualPhrases.empty()) return;
	string chunk = individualPhrases.front();
	for (int i = 1; i < individualPhrases.size(); i++) {
		if (!RAG_Memory::areChunksCorrelated(rag, individualPhrases.at(i), query)) continue;
		if (chunk.size() >= 200) {
			rag->saveChunk(chunk, link, 60);
			chunk = individualPhrases.at(i);
			continue;
		}
		else if (RAG_Memory::areChunksCorrelated(rag, chunk, individualPhrases.at(i))) {
			chunk += individualPhrases.at(i);
		}
		else {
			rag->saveChunk(chunk, link, 60);
			chunk = individualPhrases.at(i);
		}
	}
	rag->saveChunk(chunk, link, 60);
	rag->saveMemory();
}

namespace Interface {
	vector<string> extractUrlsFromWebPage(string pageContent, int count=0) {
		if (!json::accept(pageContent)) return extractHTML(pageContent, DuckDuckGo_REGEX);
		json result = json::parse(pageContent);
		vector<string> links;
		if (result.contains("AbstractURL")) {
			links.push_back(result["AbstractURL"]);
			cout << result["AbstractURL"] << endl;
		}
		if (result.contains("RelatedTopics")) {
			for (const auto& item : result["RelatedTopics"]) {
				string url = item["FirstURL"];
				vector<string> externalLinks;
				if (!url.empty()) externalLinks=extractUrlsFromWebPage(getWebPage(getUrlFromString(url)), count+links.size());
				for (string i : externalLinks) {
					links.push_back(i);
					if (count + links.size() >= 5) return links;
				}
			}
		}
		return links;
	}
	
	vector<string> getUrlFromString(string urlString) {
		vector<string> url;
		if(urlString.find("//")!=string::npos) urlString = urlString.substr(urlString.find("//") +2);
		size_t slashPos = urlString.find("/");

		if (slashPos != string::npos) {
			url.push_back(urlString.substr(0, slashPos));
			url.push_back(urlString.substr(slashPos));
		}
		else {
			url.push_back(urlString);
			url.push_back("/"); 
		}
		return url;
	}

	vector<string> getUrlFromString(string urlString, vector<string> curUrl) {
		vector<string> url;
		if (urlString.find("//")!=string::npos) urlString = urlString.substr(urlString.find("//") + 2);
		size_t slashPos = urlString.find("/");
		if (slashPos != string::npos) {
			if (slashPos == 0) {
				url.insert(url.begin(), curUrl[0]);
			}
			else {
				url.insert(url.begin(), urlString.substr(0, slashPos));
			}
			url.insert(url.begin()+1, urlString.substr(slashPos));
		}
		else {
			url.push_back(urlString);
			url.push_back("/");
		}
		return url;
	}

	vector<string> getUrlFromQuery(string query) {
		replace(query.begin(), query.end(), ' ', '+');
		vector<string> url;
		url.push_back("api.duckduckgo.com");
		url.push_back("/?q=" + query + "&format=json&no_redirect=1");
		return url;
	}

	string analyze_HTML_Lexbor(lxb_dom_node_t* node){
		if (node == nullptr) return "";
		if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
			lxb_dom_element_t* element = lxb_dom_interface_element(node);
			lxb_tag_id_t tag_id = lxb_dom_element_tag_id(element);
			if (tag_id == LXB_TAG_SCRIPT || tag_id == LXB_TAG_STYLE) {
				return "";
			}
		}
		string result = "";
		if (node->type == LXB_DOM_NODE_TYPE_TEXT) {
			size_t len;
			const lxb_char_t* text = lxb_dom_node_text_content(node, &len);
			if (text) result.append((const char*)text, len);
		}
		
		lxb_dom_node* child = lxb_dom_node_first_child(node);
		while (child) {
			result.append(analyze_HTML_Lexbor(child));
			child = lxb_dom_node_next(child);
		}

		return result;
	}

	string sanitizePage(string pageContent) {
		lxb_html_parser_t* parser = lxb_html_parser_create();
		lxb_status_t status = lxb_html_parser_init(parser);

		lxb_html_document_t* document = lxb_html_parse(parser, (const lxb_char_t*)pageContent.c_str(), strlen(pageContent.c_str()));
		
		stringstream textStream(analyze_HTML_Lexbor(lxb_dom_interface_node(document->body)));
		string line;
		string text;
		while (getline(textStream, line)) {
			string lineNoSpace = removeSpaces(line);
			if (lineNoSpace.length() >= 20) text.append('.' + line);
		}
		return text;
	}

	string getWebPage(vector<string> url) {
		string pageContent = "";
		try {
			net::io_context ioc; //needed for the resolver and the stream
			ssl::context ctx(ssl::context::tlsv12_client); //SSL Context for HTTPS
			ctx.set_default_verify_paths();
			vector<string> currentURL = url;
			for (int i = 0; i < 5;i++) {
				tcp::resolver resolver(ioc); //DNS resolver object 
				beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx); //Stream

				//DNS
				auto const results = resolver.resolve(currentURL[0], "443");
				beast::get_lowest_layer(stream).connect(results);
				
				//SNI 
				if (!SSL_set_tlsext_host_name(stream.native_handle(), currentURL[0].c_str())) {
					throw beast::system_error(
						beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()),
						"Failed to set SNI Hostname");
				}

				//Handshake
				stream.handshake(ssl::stream_base::client);

				//creating the http request
				http::request<http::string_body> request{ http::verb::get, currentURL[1], 11}; // forzato HTTP/1.1 (11)
				request.set(http::field::host, currentURL[0]);
				request.set(http::field::user_agent, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36");
				request.set(http::field::accept_language, "it-IT,it;q=0.9,en-US;q=0.8,en;q=0.7");
				request.set(http::field::connection, "keep-alive");

				//Sending data
				http::write(stream, request);

				//Recieving Response
				beast::flat_buffer buffer;
				http::response_parser<http::dynamic_body> parser;
				parser.body_limit(20 * 1024 * 1024);
				http::read(stream, buffer, parser);

				auto response = parser.release();

				unsigned int status_code = response.result_int();
				if (status_code >= 300 && status_code < 400) { //Redirect
					std::string location = response[http::field::location];
					if (!location.empty()) {
						beast::error_code ec;
						stream.shutdown(ec);
						beast::get_lowest_layer(stream).socket().close(ec);
						currentURL = getUrlFromString(location, currentURL);
						cout << "[Redirect] : " << location << endl;
						continue;
					}
				}
				pageContent = beast::buffers_to_string(response.body().data());

				beast::error_code ec;
				stream.shutdown(ec);
				beast::get_lowest_layer(stream).socket().close(ec);

				return pageContent;
			}
			cerr << "Too much redirecting!!" << endl;
			return "";
		}
		catch (exception& e) {
			cerr << "Error fetching the web page: " << e.what() << endl;
		}
		return pageContent;
	}

	vector <string> retrieveDataFromRAG(RAG_Memory* rag, string query) {
		return rag->search("Query: "+query);
	}

	vector<string> searchOnline(string query) {
		vector<string> url = getUrlFromQuery(query);
		string page = getWebPage(url);
		return extractUrlsFromWebPage(page);
	}

	vector<string> retrieveDataFromInternet(RAG_Memory* rag, string link, string query) {
		string site = getWebPage(getUrlFromString(link));
		string data = sanitizePage(site);
		vector<string> individualPhrases;
		if (data.empty()) return vector<string>();
		int batchProcessed = 0;
		while (!data.empty()&&batchProcessed<=5) {
			string sub = data.substr(0, data.find_first_of('.'));
			data = data.substr(data.find_first_of('.') + 1);
			individualPhrases.push_back(sub);
			if (individualPhrases.size() > 500) {
				insertChunks(rag, individualPhrases, link, query);
				individualPhrases.clear();
				batchProcessed++;
			}
		}
		return retrieveDataFromRAG(rag, query);
	}

	string getActionSummary(PythonRuntime* python) {
		string staticAction = 
			" 0 - Response: Return a response to the user and end the task, input:response (string), return none (void)\n"
			" 1 - Retrieve data from RAG memory, input: query(string), return list of result (vector<string>)\n"
			" 2 - Retrieve data from the internet page specified with the given query, input: url(string), query(string), return list of result(vector<string>)\n"
			" 3 - Execute Python Code, input: code(string), return stdout of the python code (string)";
		vector<string> pythonDescriptions = python->getDescriptionList();
		for (int i = 0; i < python->getToolSetSize(); i++) {
			staticAction += "\n " + to_string(4 + i) + " - " + pythonDescriptions[i];
		}
		return staticAction;
	}
}