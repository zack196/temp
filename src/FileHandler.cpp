#include "../include/FileHandler.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>

/**
 * Default constructor
 */
FileHandler::FileHandler() : _filePath("") {
}

/**
 * Constructor with file path
 */
FileHandler::FileHandler(const std::string& filePath) : _filePath(filePath) {
}

/**
 * Copy constructor
 */
FileHandler::FileHandler(const FileHandler& other) : _filePath(other._filePath) {
}

/**
 * Assignment operator
 */
FileHandler& FileHandler::operator=(const FileHandler& other) {
	if (this != &other) {
		_filePath = other._filePath;
	}
	return *this;
}

/**
 * Destructor
 */
FileHandler::~FileHandler() {
}

/**
 * Get the file path
 */
std::string FileHandler::getPath() const {
	return _filePath;
}

/**
 * Set the file path
 */
void FileHandler::setPath(const std::string& filePath) {
	_filePath = filePath;
}

/**
 * Get the type of the path:
 * 0 - does not exist
 * 1 - regular file
 * 2 - directory
 * 3 - special file (device, pipe, socket, etc.)
 */
int FileHandler::getTypePath(const std::string& path) const {
	struct stat fileStat;

	// Check if the file exists
	if (stat(path.c_str(), &fileStat) != 0) {
		return 0; // Does not exist
	}

	// Check the file type
	if (S_ISREG(fileStat.st_mode)) {
		return 1; // Regular file
	} else if (S_ISDIR(fileStat.st_mode)) {
		return 2; // Directory
	} else {
		return 3; // Special file (device, pipe, socket, etc.)
	}
}

/**
 * Check if a file has specific permissions
 * @param path File path to check
 * @param mode 0 for readable, 1 for writable, 2 for executable
 * @return true if the file has the requested permission
 */
bool FileHandler::checkFile(const std::string& path, int mode) const {
	int accessMode;

	switch (mode) {
		case 0:
			accessMode = R_OK; // Check for read permission
			break;
		case 1:
			accessMode = W_OK; // Check for write permission
			break;
		case 2:
			accessMode = X_OK; // Check for execute permission
			break;
		default:
			accessMode = F_OK; // Check for existence only
			break;
	}

	return (access(path.c_str(), accessMode) == 0);
}

/**
 * Read entire file content into a string
 * @param path File path to read
 * @return String containing the file's content
 */
std::string FileHandler::readFile(const std::string& path) const 
{
	std::ifstream file(path.c_str());

	if (!file.is_open()) 
		throw std::runtime_error("Failed to open file: " + path);

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

/**
 * Check if a path exists
 * @param path Path to check
 * @return true if the path exists
 */
bool FileHandler::exists(const std::string& path) const {
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0);
}
