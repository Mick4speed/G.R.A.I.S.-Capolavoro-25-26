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
#include <boost/asio/ssl/stream.hpp>

#include <lexbor/html/html.h>
#include <lexbor/dom/interfaces/element.h>
#include <lexbor/html/tag.h>    

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


namespace Interface {
	vector<string> extractUrlsFromWebPage(string pageContent) {
		vector<string> urls;
		regex pattern(DuckDuckGo_REGEX);

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
				url = url.substr(string_start_pos+5);	
				urls.push_back(url);
			}			
			it++;
		}
		return urls;
	}
	
	vector<string> getUrlFromString(string urlString) {
		vector<string> url;
		if(urlString.find("//")) urlString = urlString.substr(urlString.find("//") +2);
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

	vector<string> getUrlFromQuery(string query) {
		replace(query.begin(), query.end(), ' ', '+');
		vector<string> url;
		url.push_back("html.duckduckgo.com");
		url.push_back("/html/?q=" + query);
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
			tcp::resolver resolver(ioc); //Resolver of host name aka DNS

			ctx.set_default_verify_paths(); //Set the default paths for SSL certificate verification

			beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx); //TCP HTTPS Data-Stream

			auto const results = resolver.resolve(url[0], HTTPS_PORT);
			beast::get_lowest_layer(stream).connect(results);

			if (!SSL_set_tlsext_host_name(stream.native_handle(), url[0].c_str())) {
				throw beast::system_error(
					beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()),
					"Failed to set SNI Hostname");
			}

			stream.handshake(ssl::stream_base::client); //Perform SSL Handshake

			http::request<http::string_body> request{ http::verb::get, url[1], version };
			request.set(http::field::host, url[0]);
			request.set(http::field::user_agent,
				"Chrome/124.0.0.0"); //Mask User-Agent Field

			http::write(stream, request);

			beast::flat_buffer buffer; //Buffer for the response
			http::response_parser<http::dynamic_body> parser; //HTTP Response object
			parser.body_limit(20 * 1024 * 1024);
			http::read(stream, buffer, parser); //Read the response
			auto response = parser.release();
			pageContent = beast::buffers_to_string(response.body().data()); //Extract the body of the response as a string

			beast::error_code ec;
			beast::get_lowest_layer(stream).socket().close(ec); //Force closing
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

	vector<string> retrieveDataFromInternet(RAG_Memory* rag, string query) {
		vector<string> urls = searchOnline(query);
		urls.resize(SITE_TO_ANALIZE);
		for (string url : urls) {
			string data = sanitizePage(getWebPage(getUrlFromString(url)));
			vector<string> individualPhrases;
			while (!data.empty()) {
				string sub = data.substr(0, data.find_first_of('.'));
				individualPhrases.push_back(sub);
			}
			string chunk = individualPhrases.front();
			for (int i = 1; i < individualPhrases.size(); i++) {
				if (chunk.size() >= 350) {
					rag->saveChunk(chunk, url, 60);
					chunk = individualPhrases.at(i);
					continue;
				}
				if (RAG_Memory::areChunksCorrelated(rag, chunk, individualPhrases.at(i))) {
					chunk += individualPhrases.at(i);
				}
				else {
					rag->saveChunk(chunk, url, 60);
					chunk = individualPhrases.at(i+1);
					i++; //skip 2 element forward
				}
			}
		}
		return retrieveDataFromRAG(rag, query);
	}

	string getActionSummary() {
		return
			" 0 - Response: Return a response to the user and end the task, input:response (string), return none (void)\n"
			" 1 - Retrieve data from RAG memory, input: query(string), return list of result (vector<string>)\n";
	}
}