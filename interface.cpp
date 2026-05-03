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

using namespace std;

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
namespace ssl = net::ssl;			// from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

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

	string sanitizePage(string pageContent /*, vector<string> keywords*/) {
		pageContent = regex_replace(pageContent, regex(R"(<p[^>]*>([\s\S]*?)</p>)"), "$1\n");
		//TODO divide the page into chunk and search for the one which contains the keywords
		return pageContent;
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
				"Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
				"AppleWebKit/537.36 (KHTML, like Gecko) "
				"Chrome/124.0.0.0 Safari/537.36"); //Mask User-Agent Field

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

	vector<string> searchOnline(string query) {
		vector<string> url = getUrlFromQuery(query);
		string page = getWebPage(url);
		return extractUrlsFromWebPage(page);
	}

	string getActionSummary() {
		return
			" 0 - Response: Return a response to the user and end the task, input:response (string), return none (void)\n"
			" 1 - Search Online: Use DuckDuckGo to search on the web, input:query (string), return found links (vector<string>)\n"
			" 2 - Get Web Page: Return the content of a determined web page, input:URL (string), return web page as HTML (string)\n"
			" 3 - Get Sanitize Web Page: Maintain only usefull text from the HTML page, this text are divided into chunk and only the chunk containing keyword are then returned, meant to be passed to the AI, input:URL (string), keywords (vector<string>), return sanitized page as HTML (string)\n";
	}
}