/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigFile.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/28 00:23:50 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/09 21:50:07 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_FILE_HPP
#define CONFIG_FILE_HPP

#include <string>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <unistd.h>

class ConfigFile
{
	private:
		std::string _path;
		size_t      _size;

	public:
		ConfigFile();
		ConfigFile(const std::string& path);
		ConfigFile(const ConfigFile& other);
		ConfigFile& operator=(const ConfigFile& other);
		~ConfigFile();

		static int  getTypePath(const std::string& path);
		static int  checkFile(const std::string& path, int mode);
		static bool readFile(const std::string& path);
		static bool readFile(const std::string& path, std::string& output);
		static bool isFileExistAndReadable(const std::string& path, const std::string& index = "");

		bool readFile(std::string& output) const;
		const std::string& getPath() const;
		size_t getSize() const;
};

#endif

