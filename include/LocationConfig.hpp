/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationConfig.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zakaria <zakaria@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/04 16:54:43 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/28 18:34:02 by zakaria          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>
// is_location_have_redirection
class LocationConfig 
{
	public:
		LocationConfig();
		~LocationConfig();
		LocationConfig(const LocationConfig& other);
		LocationConfig& operator=(const LocationConfig& other); 

		void setRoot(const std::string& root);
		void setIndex(const std::string& index);
		void setAutoindex(const std::string& autoindex);
		void setAllowedMethods(const std::string& methods);
		void setCgiExtension(const std::string& extension);
		void setCgiPath(const std::string& path);
		bool isMethodAllowed(const std::string& method) const;
		bool isAutoIndexOn() const;

		const std::string& getRoot() const;
		const std::string& getIndex() const;
		bool getAutoindex() const;
		const std::vector<std::string>& getAllowedMethods() const;
		const std::string& getCgiExtension() const;
		const std::string& getCgiPath() const;
		std::string resolvePath(const std::string& requestPath) const;
		const std::pair<int, std::string>& getRedirect() const;

		void setPath(const std::string& path);
		void setRedirect(const std::string& redirectValue);
		const std::string& getPath() const;

		bool is_location_have_redirection() const;

		const std::string& getRedirectPath() const; 

		int getRedirectCode() const; 


		void print() const;
		std::vector<std::string> split(const std::string& str, char delimiter);

		bool if_location_has_cgi() const {
			//TODO verify also if the file extention have a _cgiExtension
			return !_cgiExtension.empty() && !_cgiPath.empty();
		}
		
	private:
		std::string _root;
		std::string _path;
		std::string _index;
		bool _autoindex;
		std::vector<std::string> _allowedMethods;
		std::string _cgiExtension;
		std::string _cgiPath;
		std::pair<int, std::string> _redirect;
};

#endif
