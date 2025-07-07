#include "../include/Utils.hpp"
#include <fstream>
#include <unistd.h>
#include <ctype.h>
#include <cstring>
#include <sys/time.h>
#include <iostream>

std::string Utils::getMimeType(const std::string &path)
{
	size_t dot_pos = path.find_last_of('.');
	if (dot_pos == std::string::npos)
		return "application/octet-stream";

	std::string extension = path.substr(dot_pos);
	for (size_t i = 0; i < extension.length(); ++i)
		extension[i] = std::tolower(extension[i]);

	if (extension == ".html" || extension == ".htm" || extension == ".shtml") 
		return "text/html";
	if (extension == ".css")
		return "text/css";
	if (extension == ".xml") 
		return "text/xml";
	if (extension == ".gif") 
		return "image/gif";
	if (extension == ".jpeg" || extension == ".jpg")
		return "image/jpeg";
	if (extension == ".js") 
		return "application/javascript";
	if (extension == ".atom")
		return "application/atom+xml";
	if (extension == ".rss") 
		return "application/rss+xml";
	if (extension == ".mml") 
		return "text/mathml";
	if (extension == ".txt")
		return "text/plain";
	if (extension == ".jad") 
		return "text/vnd.sun.j2me.app-descriptor";
	if (extension == ".wml") 
		return "text/vnd.wap.wml";
	if (extension == ".htc") 
		return "text/x-component";
	if (extension == ".avif") 
		return "image/avif";
	if (extension == ".png") 
		return "image/png";
	if (extension == ".svg" || extension == ".svgz") 
		return "image/svg+xml";
	if (extension == ".tif" || extension == ".tiff")
		return "image/tiff";
	if (extension == ".wbmp") 
		return "image/vnd.wap.wbmp";
	if (extension == ".webp") 
		return "image/webp";
	if (extension == ".ico") 
		return "image/x-icon";
	if (extension == ".jng") 
		return "image/x-jng";
	if (extension == ".bmp")
		return "image/x-ms-bmp";
	if (extension == ".woff")
		return "font/woff";
	if (extension == ".woff2") 
		return "font/woff2";
	if (extension == ".jar" || extension == ".war" || extension == ".ear") 
		return "application/java-archive";
	if (extension == ".json") 
		return "application/json";
	if (extension == ".hqx") 
		return "application/mac-binhex40";
	if (extension == ".doc")
		return "application/msword";
	if (extension == ".pdf") 
		return "application/pdf";
	if (extension == ".ps" || extension == ".eps" || extension == ".ai")
		return "application/postscript";
	if (extension == ".rtf") 
		return "application/rtf";
	if (extension == ".m3u8") 
		return "application/vnd.apple.mpegurl";
	if (extension == ".kml") 
		return "application/vnd.google-earth.kml+xml";
	if (extension == ".kmz") 
		return "application/vnd.google-earth.kmz";
	if (extension == ".xls")
		return "application/vnd.ms-excel";
	if (extension == ".eot") 
		return "application/vnd.ms-fontobject";
	if (extension == ".ppt") 
		return "application/vnd.ms-powerpoint";
	if (extension == ".odg") 
		return "application/vnd.oasis.opendocument.graphics";
	if (extension == ".odp") 
		return "application/vnd.oasis.opendocument.presentation";
	if (extension == ".ods") 
		return "application/vnd.oasis.opendocument.spreadsheet";
	if (extension == ".odt") 
		return "application/vnd.oasis.opendocument.text";
	if (extension == ".pptx")
		return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
	if (extension == ".xlsx") 
		return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
	if (extension == ".docx") 
		return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
	if (extension == ".wmlc") 
		return "application/vnd.wap.wmlc";
	if (extension == ".wasm") 
		return "application/wasm";
	if (extension == ".7z")
		return "application/x-7z-compressed";
	if (extension == ".cco") 
		return "application/x-cocoa";
	if (extension == ".jardiff") 
		return "application/x-java-archive-diff";
	if (extension == ".jnlp") 
		return "application/x-java-jnlp-file";
	if (extension == ".run") 
		return "application/x-makeself";
	if (extension == ".pl" || extension == ".pm")
		return "application/x-perl";
	if (extension == ".prc" || extension == ".pdb")
		return "application/x-pilot";
	if (extension == ".rar") 
		return "application/x-rar-compressed";
	if (extension == ".rpm") 
		return "application/x-redhat-package-manager";
	if (extension == ".sea")
		return "application/x-sea";
	if (extension == ".swf") 
		return "application/x-shockwave-flash";
	if (extension == ".sit") 
		return "application/x-stuffit";
	if (extension == ".tcl" || extension == ".tk")
		return "application/x-tcl";
	if (extension == ".der" || extension == ".pem" || extension == ".crt")
		return "application/x-x509-ca-cert";
	if (extension == ".xpi") 
		return "application/x-xpinstall";
	if (extension == ".xhtml") 
		return "application/xhtml+xml";
	if (extension == ".xspf")
		return "application/xspf+xml";
	if (extension == ".zip") 
		return "application/zip";
	if (extension == ".bin" || extension == ".exe" || extension == ".dll") 
		return "application/octet-stream";
	if (extension == ".deb") 
		return "application/octet-stream";
	if (extension == ".dmg") 
		return "application/octet-stream";
	if (extension == ".iso" || extension == ".img")
		return "application/octet-stream";
	if (extension == ".msi" || extension == ".msp" || extension == ".msm")
		return "application/octet-stream";
	if (extension == ".mid" || extension == ".midi" || extension == ".kar")
		return "audio/midi";
	if (extension == ".mp3") 
		return "audio/mpeg";
	if (extension == ".ogg") 
		return "audio/ogg";
	if (extension == ".m4a")
		return "audio/x-m4a";
	if (extension == ".ra") 
		return "audio/x-realaudio";
	if (extension == ".3gp" || extension == ".3gpp")
		return "video/3gpp";
	if (extension == ".ts") 
		return "video/mp2t";
	if (extension == ".mp4") 
		return "video/mp4";
	if (extension == ".mpeg" || extension == ".mpg") 
		return "video/mpeg";
	if (extension == ".mov") 
		return "video/quicktime";
	if (extension == ".webm") 
		return "video/webm";
	if (extension == ".flv") 
		return "video/x-flv";
	if (extension == ".m4v") 
		return "video/x-m4v";
	if (extension == ".mng") 
		return "video/x-mng";
	if (extension == ".asx" || extension == ".asf")
		return "video/x-ms-asf";
	if (extension == ".wmv")
		return "video/x-ms-wmv";
	if (extension == ".avi") 
		return "video/x-msvideo";
	return "application/octet-stream";
}


std::string Utils::getCurrentDate()
{
	char buffer[128];
	time_t now = time(NULL);
	struct tm* gmt = gmtime(&now);
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
	return std::string(buffer);
}

bool Utils::isPathWithinRoot(const std::string& root, const std::string& path) 
{
	return path.find(root) == 0;
}

std::string Utils::readFileContent(const std::string& filePath) 
{
	std::ifstream file(filePath.c_str(), std::ios::binary);
	if (!file.is_open()) 
		throw std::runtime_error("Could not open file: " + filePath);

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

bool Utils::isDirectory(const std::string& path)
{
	struct stat pathStat;
	if (stat(path.c_str(), &pathStat) != 0)
		return false;
	return S_ISDIR(pathStat.st_mode);
}

bool Utils::isFileExists(const std::string& path) 
{
	return (access(path.c_str(), F_OK) == 0);
}

bool Utils::isFileWritable(const std::string& path) 
{
	return (access(path.c_str(), W_OK) == 0);
}

bool Utils::isFileReadble(const std::string& path) 
{
	return (access(path.c_str(), R_OK) == 0);
}

std::string Utils::trimWhitespace(const std::string& str)
{
	size_t start = str.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) 
		return "";

	size_t end = str.find_last_not_of(" \t\r\n");
	return str.substr(start, end - start + 1);
}

std::string Utils::trim(const std::string& str) 
{
	size_t first = str.find_first_not_of(" \t");
	if (first == std::string::npos)
		return "";

	size_t last = str.find_last_not_of(" \t");
	return str.substr(first, last - first + 1);
}


void Utils::skipWhitespace(std::string& str) 
{
	size_t i = 0;
	while (i < str.size() && (str[i] == ' ' || str[i] == '\t')) 
		i++;
	str.erase(0, i);
}

int Utils::urlDecode(std::string& uri) 
{
	std::string result;
	result.reserve(uri.length());

	for (size_t i = 0; i < uri.length(); ++i)
	{
		if (uri[i] == '%')
		{
			if (i + 2 >= uri.length() || !isxdigit(uri[i + 1]) || !isxdigit(uri[i + 2]))
				return -1;

			int value;
			std::istringstream iss(uri.substr(i + 1, 2));
			if (!(iss >> std::hex >> value))
				return -1;

			result += value;
			i += 2;
		}
		else if (uri[i] == '+')
			result += ' ';
		else
			result += uri[i];
	}
	uri = result;
	return 0;
}


ssize_t Utils::parseHexChunk(const std::string& hexstr) 
{
	size_t result = 0;
	size_t i = 0;

	while (i < hexstr.size()) 
	{
		char c = hexstr[i];

		if (c >= '0' && c <= '9') 
			result = (result * 16) + (c - '0');
		else if (c >= 'a' && c <= 'f') 
			result = (result * 16) + (c - 'a' + 10);
		else if (c >= 'A' && c <= 'F') 
			result = (result * 16) + (c - 'A' + 10);
		else 
			break;
		++i;
	}

	if (i == 0 || i > 8)
		return -1;

	return result;
}

bool strToSizeT(const std::string& str, size_t& result, bool allowZero = false) 
{
	if (str.empty()) 
		return (false);

	for (size_t i = 0; i < str.size(); ++i) 
		if (!isdigit(str[i])) 
			return false;

	std::stringstream ss(str);
	ss >> result;

	if (ss.fail() || !ss.eof()) 
		return (false);

	if (!allowZero && result == 0) 
		return (false);

	return (true);
}

std::string Utils::getMessage(int code) 
{
	switch(code) 
	{
		// Success
		case 200: return "OK";
		case 201: return "Created";
		case 202: return "Accepted";
		case 203: return "Non-Authoritative Information";
		case 204: return "No Content";
		case 205: return "Reset Content";
		case 206: return "Partial Content";
		case 207: return "Multi-Status";
		case 208: return "Already Reported";
		case 226: return "IM Used";

		// Redirection
		case 300: return "Multiple Choices";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 303: return "See Other";
		case 304: return "Not Modified";
		case 305: return "Use Proxy";
		case 306: return "Switch Proxy";
		case 307: return "Temporary Redirect";
		case 308: return "Permanent Redirect";

		// Client Errors
		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 402: return "Payment Required";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 406: return "Not Acceptable";
		case 407: return "Proxy Authentication Required";
		case 408: return "Request Timeout";
		case 409: return "Conflict";
		case 410: return "Gone";
		case 411: return "Length Required";
		case 412: return "Precondition Failed";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		case 415: return "Unsupported Media Type";
		case 416: return "Range Not Satisfiable";
		case 417: return "Expectation Failed";
		case 418: return "I'm a teapot";
		case 421: return "Misdirected Request";
		case 422: return "Unprocessable Entity";
		case 423: return "Locked";
		case 424: return "Failed Dependency";
		case 425: return "Too Early";
		case 426: return "Upgrade Required";
		case 428: return "Precondition Required";
		case 429: return "Too Many Requests";
		case 431: return "Request Header Fields Too Large";
		case 451: return "Unavailable For Legal Reasons";

		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";
		case 504: return "Gateway Timeout";
		case 505: return "HTTP Version Not Supported";
		case 506: return "Variant Also Negotiates";
		case 507: return "Insufficient Storage";
		case 508: return "Loop Detected";
		case 510: return "Not Extended";
		case 511: return "Network Authentication Required";

		default: return "Unknown Status";
	}
}

std::vector<std::string> Utils::listDirectory(const std::string& path)
{
	std::vector<std::string> entries;
	DIR* dir = opendir(path.c_str());
	if (!dir)
		return entries;

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (std::string(entry->d_name) == ".")
			continue;
		entries.push_back(std::string(entry->d_name));
	}
	closedir(dir);
	return entries;
}

size_t Utils::stringToSizeT(const std::string& str) 
{
	std::istringstream iss(str);
	size_t result;
	iss >> result;

	if (iss.fail()) 
		throw std::invalid_argument("Invalid size_t conversion: " + str);

	char remaining;
	if (iss >> remaining) 
		throw std::invalid_argument("Extra characters after number: " + str);
	return (result);
}

bool Utils::isValidMethodToken(const std::string& method)
{
	if (method.empty())
		return false;

	const char* delimiters = "\"(),/:;<=>?@[\\]{} \t";

	for (size_t i = 0; i < method.length(); ++i) 
	{

		if (method[i] < 33 || method[i] > 126)
			return false;

		for (size_t j = 0; delimiters[j] != '\0'; ++j) 
			if (method[i] == delimiters[j])
				return false;

		if (method[i] >= 'a' && method[i] <= 'z')
			return false;
	}

	return true;
}


size_t Utils::skipLeadingWhitespace(const std::string& str)
{
	size_t pos = 0;
	while (pos < str.size() && (str[pos] == ' ' || str[pos] == '\t')) 
		++pos;
	return pos;
}

bool Utils::isValidUri(const std::string& uri)
{
	for (size_t i = 0; i < uri.size(); ++i) 
		if (!std::isprint(uri[i])) 
			return false;
	return true;
}

bool Utils::isValidHeaderKey(const std::string& key) 
{
	if (key.empty())
		return false;

	// RFC 7230: token = 1*tchar
	// tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "." /
	//         "^" / "_" / "`" / "|" / "~" / DIGIT / ALPHA
	// Separators (forbidden): "\"(),/:;<=>?@[\\]{} \t"

	for (size_t i = 0; i < key.length(); ++i) 
	{
		char ch = key[i];

		// Must be printable ASCII (33-126)
		if (ch < 33 || ch > 126)
			return false;

		// Check if it's a separator (forbidden)
		if (ch == '"' || ch == '(' || ch == ')' || ch == ',' || 
			ch == '/' || ch == ':' || ch == ';' || ch == '<' || 
			ch == '=' || ch == '>' || ch == '?' || ch == '@' || 
			ch == '[' || ch == '\\' || ch == ']' || ch == '{' || 
			ch == '}' || ch == ' ' || ch == '\t')
		{
			return false;
		}
	}
	return true;
}


// Validate HTTP header value per RFC 7230 (field-value)
bool Utils::isValidHeaderValue(const std::string& value) 
{
	// Empty values are allowed in some cases (e.g., Host: )
	if (value.empty()) 
		return false;

	// Check each character against field-vchar (VCHAR) and SP/HTAB
	for (size_t i = 0; i < value.size(); ++i) 
		if (value[i] != ' ' && value[i] != '\t' && (value[i] < 33 || value[i] > 126)) 
			return false;
	return true;
}

std::string Utils::extractAttribute(const std::string& headers, const std::string& key) {
	std::string search = key + "=\"";
	size_t start = headers.find(search);
	if (start == std::string::npos) return "";
	start += search.length();
	size_t end = headers.find("\"", start);
	if (end == std::string::npos) return "";
	return headers.substr(start, end - start);
}

std::string Utils::getFileExtension(const std::string& path)
{
	size_t dotPos = path.find_last_of('.');
	if (dotPos != std::string::npos) 
		return path.substr(dotPos);
	return "";
}


bool Utils::isExecutable(const std::string& path)
{
	return access(path.c_str(), X_OK) == 0;
}


char** Utils::mapToEnvp(const std::map<std::string, std::string>& env)
{
	std::vector<std::string> envStrings;

	for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it) 
		envStrings.push_back(it->first + "=" + it->second);

	char** result = new char*[envStrings.size() + 1];

	for (size_t i = 0; i < envStrings.size(); ++i) 
	{
		result[i] = new char[envStrings[i].size() + 1];
		strcpy(result[i], envStrings[i].c_str());
	}

	result[envStrings.size()] = NULL;
	return result;
}


std::string Utils::createUploadFile(const std::string& prefix, const std::string& dir)
{
	if (mkdir(dir.c_str(), 0755) == -1 && errno != EEXIST)
		throw std::runtime_error("Failed to create directory: " + dir);

	std::stringstream ss;
	ss << dir << "/" << prefix;
	return ss.str();
}

std::string Utils::createTempFile(const std::string& prefix, const std::string& dir)
{
	if (mkdir(dir.c_str(), 0755) == -1 && errno != EEXIST)
		throw std::runtime_error("Failed to create directory: " + dir);

	struct timeval tv;
	gettimeofday(&tv, NULL);

	std::stringstream ss;
	ss << dir << "/" << prefix << "_" << getpid() << "_" << tv.tv_sec << "_" << tv.tv_usec;
	return ss.str();
}


std::string Utils::getExtension(const std::string& path) 
{
	std::size_t slashPos = path.find_last_of("/\\");
	std::size_t dotPos = path.find_last_of('.');
	if (dotPos != std::string::npos && (slashPos == std::string::npos || dotPos > slashPos)) 
		return path.substr(dotPos);
	return "";
}
std::vector<std::string> Utils::split(const std::string& str, char delimiter) 
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream iss(str);

	while (std::getline(iss, token, delimiter)) 
	{
		token = Utils::trim(token);
		if (!token.empty()) 
			tokens.push_back(token);
	}

	return tokens;
}
