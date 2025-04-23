/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/10 16:10:25 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/20 01:29:35 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "Client.hpp"
#include "LocationConfig.hpp"
#include "HTTPRequest.hpp"
#include <string>
#include <map>

class HTTPResponse 
{
	public:
		HTTPResponse(Client* client);
		HTTPResponse(const HTTPResponse& rhs);
		~HTTPResponse();

		HTTPResponse& operator=(const HTTPResponse& rhs);

		int	buildResponse();
		void	get_matched_location_for_request_uri(const std::string& requestUri) ;
		bool	is_req_well_formed();
		const std::string get_requested_resource(LocationConfig location);
		void handleGet(LocationConfig location);
		void handlePost();
		void handleDelete();

		void clear();

		void setStatusCode(int code);
		void setStatusMessage(const std::string& message);
		void setProtocol(const std::string& protocol);
		void setHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& body);
		void setResponse();

		std::string getResponse() const;
		
		// add just added
		bool get_resource_type(std::string resource);
		bool is_uri_has_backslash_in_end(const std::string& resource);
		bool is_dir_has_index_files(const std::string& resource, const std::string& index);

		//

	private:
		// void handleGetRequest();
		// void handlePostRequest();
		// void handleDeleteRequest();

		// LocationConfig findMatchingLocation(const std::string& requestUri) const;

		// std::string buildFullPath(const LocationConfig& location, const std::string& requestUri) const;
		std::string readFileContent(const std::string& filePath) const;

		void buildSuccessResponse(const std::string& fullPath);
		void buildAutoIndexResponse(/*const std::string& autoIndexContent*/);
		void buildErrorResponse(int statusCode, const std::string& message = "");

		Client* _client;
		HTTPRequest* _request;
		
		int _statusCode;
		std::string	_protocol;
		std::string _statusMessage;
		std::map<std::string, std::string> _headers;
		std::string _body;
		std::string _response;

		LocationConfig	_matched_location;
		bool			_hasMatchedLocation;
		
};

#endif // HTTPRESPONSE_HPP
