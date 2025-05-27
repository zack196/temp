#ifndef FILE_HANDLER_HPP
#define FILE_HANDLER_HPP

#include <string>

/**
 * Class for handling file operations in C++98 compatible way
 * Provides utilities for checking file existence, type, and permissions
 */
class FileHandler 
{
private:
	std::string _filePath;

public:
	/* Constructors & Destructor */
	FileHandler();
	FileHandler(const std::string& filePath);
	FileHandler(const FileHandler& other);
	FileHandler& operator=(const FileHandler& other);
	~FileHandler();

	/* Getters & Setters */
	std::string getPath() const;
	void setPath(const std::string& filePath);

	/* File Operations */
	/**
     * Get the type of the path:
     * 0 - does not exist
     * 1 - regular file
     * 2 - directory
     * 3 - special file (device, pipe, socket, etc.)
     */
	int getTypePath(const std::string& path) const;

	/**
     * Check if a file has specific permissions
     * @param path File path to check
     * @param mode 0 for readable, 1 for writable, 2 for executable
     * @return true if the file has the requested permission
     */
	bool checkFile(const std::string& path, int mode) const;

	/**
     * Read entire file content into a string
     * @param path File path to read
     * @return String containing the file's content
     */
	std::string readFile(const std::string& path) const;

	/**
     * Check if a path exists
     * @param path Path to check
     * @return true if the path exists
     */
	bool exists(const std::string& path) const;
};

#endif // FILE_HANDLER_HPP
