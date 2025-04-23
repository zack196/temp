#include "../include/HTTPResponse.hpp"
#include "../include/Utils.hpp"
#include "../include/webserver.hpp"

// Constructor/Destructor
HTTPResponse::HTTPResponse(Client* client) : _client(client)
	, _request(_client->getRequest()), _statusCode(200), _statusMessage("OK")
	, _body("") , _hasMatchedLocation(false)
{
}

HTTPResponse::HTTPResponse(const HTTPResponse& rhs) 
{
	*this = rhs;
}

HTTPResponse::~HTTPResponse() 
{
}

HTTPResponse& HTTPResponse::operator=(const HTTPResponse& rhs) 
{
	if (this != &rhs) 
	{
		_client = rhs._client;
		_request = rhs._request;
		_statusCode = rhs._statusCode;
		_statusMessage = rhs._statusMessage;
		_headers = rhs._headers;
		_body = rhs._body;
		_response = rhs._response;
	}
	return *this;
}

// Public methods
void HTTPResponse::clear() 
{
	_statusCode = 200;
	_statusMessage = "OK";
	_headers.clear();
	_body.clear();
	_response.clear();
}

void HTTPResponse::setStatusCode(int code) 
{
	_statusCode = code;
}

void HTTPResponse::setStatusMessage(const std::string& message) 
{
	_statusMessage = message;
}

void HTTPResponse::setProtocol(const std::string& protocol)
{
	_protocol = protocol;
}

void HTTPResponse::setHeader(const std::string& key, const std::string& value) 
{
	_headers[key] = value;
}

void HTTPResponse::setBody(const std::string& body) 
{
	_body = body;
	std::ostringstream oss;
	oss << _body.length();
	setHeader("Content-Length", oss.str());
}

void HTTPResponse::setResponse(/*const std::string& response*/) 
{
	std::ostringstream response;
	response << _protocol << " " << _statusCode << " " << _statusMessage << "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end();++it)	
		response << it->first << ": " << it->second << "\r\n";
	response << "\r\n";
	response << _body;
	_response = response.str();
}

std::string HTTPResponse::getResponse() const 
{
	return _response;
}

void HTTPResponse::get_matched_location_for_request_uri(const std::string& requestUri) 
{
	std::string best_match;
	std::map<std::string, LocationConfig> map_locations = _client->getServer()->getLocations();

	for (std::map<std::string, LocationConfig>::iterator it = map_locations.begin()
		; it != map_locations.end();it++)
	{
		if (it->first == requestUri)
		{
			_hasMatchedLocation = true;
			_matched_location = it->second ;
			return ;
		}
		if (requestUri.find(it->first) == 0)// /foo /foobar
		{
			if (requestUri.size() == it->first.size() || requestUri[it->first.size() - 1] == '/')
			{
				if (best_match.size() < it->first.size())
				{
					best_match = it->first;
					_hasMatchedLocation = true;
					_matched_location = it->second;
				}
			}
		}
	}
	// _matched_location.print();
}

const std::string HTTPResponse::get_requested_resource(LocationConfig location)
{
	std::string	root = location.getRoot() ;
	std::string	path = _request->getPath();
	
	size_t	a = 0;
	for (size_t i = 0; i < path.size(); i++)
	{
		if (path[i] == '/' && i + 1 < path.size())
			a = i;
	}
	path = path.substr(a, path.size());
	return root + path;
}

bool HTTPResponse::get_resource_type(std::string resource)
{
    struct stat path_stat;
	stat(resource.c_str(), &path_stat);
	if (S_ISDIR(path_stat.st_mode))
		return false; // is a diractory
	return true;
}

bool HTTPResponse::is_uri_has_backslash_in_end(const std::string& resource)
{
	if (resource.empty())
		return false;
	if (resource[resource.size() - 1] == '/')
		return true;
	return false;
}

bool HTTPResponse::is_dir_has_index_files(const std::string& resource, const std::string& index)
{
	//TODO this is not good
	std::string	fullPath = resource + index;
	if (access(fullPath.c_str(), F_OK) == 0)
		return true;
	return false;
}
void HTTPResponse::handleGet(LocationConfig location)
{
	std::string resource = get_requested_resource(location);
	if (access(resource.c_str(), F_OK) == 0)
	{
		if (get_resource_type(resource))// true if it is a regular file
		{
			if (location.if_location_has_cgi())
			{
				// code of cgi
			}
			else
				buildSuccessResponse(resource);
		}
		else// is a directory
		{
			if (is_uri_has_backslash_in_end(resource))
			{
				if (is_dir_has_index_files(resource, location.getIndex()))
				{
					if (location.if_location_has_cgi())
					{
						// run the cgi
					}
					else
						buildSuccessResponse(resource + location.getIndex());
				}
				else
				{
					if (location.getAutoindex())
					{
						buildAutoIndexResponse();
					}
					else
						buildErrorResponse(403 , "Forbidden");
				}
			}
			else
			{
				// make a 301 redirection to request
				// uri with “/” addeed at the end
			}
		}
	}
	else

		buildErrorResponse(404, "Not Found");
}

void HTTPResponse::handlePost() {}
void HTTPResponse::handleDelete() {}

int HTTPResponse::buildResponse()
{
	// don t forget to handel non-finish request if you have to.
	if (is_req_well_formed())
		return -1;
	get_matched_location_for_request_uri(_request->getPath());
	if (!_hasMatchedLocation)
	{
		buildErrorResponse(404 ,"Not Found");
		return -1;
	}
	if (_matched_location.is_location_have_redirection())
	{
		// handel redirection
		return 1;
	}
	if (!_matched_location.isMethodAllowed(_request->getMethod()))
	{
		buildErrorResponse(405, "Method Not Allowed");
		return -1;
	}
	if (_request->getMethod() == "GET")
		handleGet(_matched_location);
	else if (_request->getMethod() == "POST")
		handlePost();
	else if (_request->getMethod() == "DELETE")
		handleDelete();
	
	return 1;
}

bool HTTPResponse::is_req_well_formed()
{
	std::string encoding = _request->getHeaderValue("Transfer-Encoding");
	std::string content_lenght = _request->getHeaderValue("Content-Length");
	if (!encoding.empty() && encoding != "chunked")
	{
		buildErrorResponse(501 ,"Not Implemented");
		return true;
	}
	if (encoding.empty() && content_lenght.empty() && _request->getMethod() == "POST")
	{
		buildErrorResponse(400, "Bad Request");
		return true;
	}
	std::string uri = _request->getUri();
	size_t index = uri.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~:/?#[]@!$&'()*+,;=");
	if (index != std::string::npos)
	{
		buildErrorResponse(400, "Bad Request");
		return true;
	}
	if (uri.size() > 2048)
	{
		buildErrorResponse(414, "Request-URI Too Long");
		return true;
	}
	if (_request->getBody().size() > _client->getServer()->getClientMaxBodySize() )
	{
		buildErrorResponse(413, "Request Entity Too Large");
		return true;
	}
	return false;
}

// void HTTPResponse::buildErrorResponse(int statusCode, const std::string& message)
// {
// 	(void) statusCode;
// 	std::cout << message;
// }





// void HTTPResponse::handlePostRequest() 
// {
// 	buildErrorResponse(501, "Not Implemented");
// }

// void HTTPResponse::handleDeleteRequest() 
// {
// 	buildErrorResponse(501, "Not Implemented");
// }



// int HTTPResponse::buildResponse() 
// {
// 	const std::string& method = _request->getMethod();

// 	if (method == "GET") 
// 		handleGetRequest();
// 	else if (method == "POST") 
// 		handlePostRequest();
// 	else if (method == "DELETE") 
// 		handleDeleteRequest();
// 	else 
// 		buildErrorResponse(405, "Method Not Allowed");

// 	return 0;
// }

// LocationConfig HTTPResponse::findMatchingLocation(const std::string& requestUri) const 
// {
// 	ServerConfig* config = _client->getServer();
// 	const std::map<std::string, LocationConfig>& locations = config->getLocations();
// 	std::string bestMatch;

// 	for (std::map<std::string, LocationConfig>::const_iterator it = locations.begin(); it != locations.end(); ++it) 
// 	{
// 		if (it->first == requestUri) 
// 			return it->second;
// 		if (requestUri.find(it->first) == 0 && it->first.length() > bestMatch.length()) 
// 			bestMatch = it->first;
// 	}
/*
	/
	/image
	/image/cat


	/image/cat.png
	
*/

// 	if (!bestMatch.empty()) 
// 		return locations.find(bestMatch)->second;

// 	std::map<std::string, LocationConfig>::const_iterator defaultLoc = locations.find("/");
// 	if (defaultLoc != locations.end()) 
// 		return defaultLoc->second;

// 	throw std::runtime_error("No matching location found for URI: " + requestUri);
// }

// std::string HTTPResponse::buildFullPath(const LocationConfig& location, const std::string& requestUri) const 
// {
// 	std::string relativePath = requestUri.substr(location.getPath().length());
// 	std::string direPath = location.getRoot() + relativePath;

// 	if((!direPath.empty() && direPath[direPath.length() - 1] == '/') || Utils::isDirectory(direPath))
// 	{
// 		if (!direPath.empty() && direPath[direPath.length() - 1] != '/') 
// 			direPath += '/';

// 		std::string indexPath = direPath + location.getIndex();

// 		if (Utils::fileExists(indexPath)) 
// 			return (indexPath);
// 	}

// 	return (direPath);
// }

// void HTTPResponse::handleGetRequest() 
// {
// 	try {
// 		const std::string& requestUri = _request->getPath();
// 		LocationConfig location = findMatchingLocation(requestUri);

// 		if (!location.isMethodAllowed("GET")) 
// 		{
// 			buildErrorResponse(405);
// 			return;
// 		}
// 		std::string fullPath = buildFullPath(location, requestUri);
// 		if (Utils::isDirectory(fullPath)) 
// 		{
// 			if (location.isAutoIndexOn()) 
// 			{
// 				std::string autoIndexContent = Utils::listDirectory(fullPath, location.getRoot(), requestUri);
// 				buildAutoIndexResponse(autoIndexContent);
// 				return;
// 			}
// 			buildErrorResponse(403);
// 			return;
// 		}
// 		// Handle regular file requests
// 		if (!Utils::fileExists(fullPath)) 
// 		{
// 			buildErrorResponse(404);
// 			return;
// 		}

// 		buildSuccessResponse(fullPath);

// 	} 
// 	catch (const std::exception& e) 
// 	{
// 		buildErrorResponse(500, e.what());
// 	}
// }

std::string HTTPResponse::readFileContent(const std::string& filePath) const 
{
	std::ifstream file(filePath.c_str(), std::ios::binary);
	if (!file.is_open()) 
		throw std::runtime_error("Could not open file: " + filePath);

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

void  HTTPResponse::buildSuccessResponse(const std::string& fullPath)
{
	std::string fileContent = readFileContent(fullPath);
	std::ostringstream response;

	setStatusCode(_statusCode);
	setStatusMessage(_statusMessage);
	setProtocol("HTTP/1.1");
	setHeader("Content-Type", Utils::getMimeType(fullPath));
	setHeader("Connection", "close");
	setBody(fileContent);
	// response << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n"
	// 	<< "Content-Length: " << fileContent.length() << "\r\n"
	// 	<< "Content-Type: " << Utils::getMimeType(fullPath) << "\r\n"
	// 	<< "Connection: close\r\n";

	// response << "\r\n" << fileContent;
	this->setResponse();
}

void HTTPResponse::buildAutoIndexResponse(const std::string& dir, const std::string& root)
{
	std::ostringstream response;
	std::string autoIndexContent = Utils::listDirectory();
	setStatusCode(_statusCode);
	setStatusMessage(_statusMessage);
	setHeader("Content-Type", "text/html");
	setHeader("Connection", "close");
	setBody(autoIndexContent);
	// response << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n"
	// 	<< "Content-Length: " << autoIndexContent.length() << "\r\n"
	// 	<< "Content-Type: " << "text/html" << "\r\n"
	// 	<< "Connection: close\r\n";

	response << "\r\n" << autoIndexContent;
	setResponse(response.str());
}

void  HTTPResponse::buildErrorResponse(int statusCode, const std::string& message)
{

	ServerConfig* config = _client->getServer();
	const std::map<int, std::string>& error_pages = config->getErrorPages();

	std::map<int, std::string>::const_iterator it = error_pages.find(statusCode);
	std::string file_name = it->second;

	std::string fileContent = readFileContent(file_name);
	std::string statusMsg = message.empty() ? "Error" : message;
	
	setStatusCode(statusCode);
	setStatusMessage(message);
	setProtocol("HTTP/1.1");
	setHeader("Content-Type", Utils::getMimeType(file_name));
	setHeader("Connection", "close");
	setBody(fileContent);
	
	this->setResponse();
}
