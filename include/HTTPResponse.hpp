/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zakaria <zakaria@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/10 16:10:25 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/27 17:11:00 by zakaria          ###   ########.fr       */
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
		enum response_state
		{
			INIT,
			PROCESS,
			CHUNK,
			FINISH
		};
		HTTPResponse(Client* client);
		HTTPResponse(const HTTPResponse& rhs);
		~HTTPResponse();

		HTTPResponse& operator=(const HTTPResponse& rhs);

		int buildResponse();
		void clear();

		void setStatusCode(int code);
		void setStatusMessage(const std::string& message);
		void setProtocol(const std::string& protocol);
		void setHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& body);
		void setResponse(/* const std::string& response */);
		void setState(response_state state);
		int getState() const { return _state; }

		std::string getResponse() const;


		void	print() ;

	private:
		void handleGet(LocationConfig location);
		void handlePost();
		void handleDelete();

		void	get_matched_location_for_request_uri(const std::string& requestUri);
		
		
		bool	is_req_well_formed();
		const std::string get_requested_resource(const LocationConfig& location);
		bool get_resource_type(std::string resource);
		bool is_uri_has_backslash_in_end(const std::string& resource);
		bool is_dir_has_index_files(const std::string& resource, const std::string& index);

		std::string readFileContent(const std::string& filePath) const;

		void buildSuccessResponse(const std::string& fullPath);
		void buildAutoIndexResponse(const std::vector<std::string>& list, const std::string& path);
		void buildErrorResponse(int statusCode, const std::string& message = "");
		void buildRediractionResponse(int code, const std::string& message, const std::string& newLocation);

		Client* _client;
		HTTPRequest* _request;
		int _statusCode;
		std::string	_protocol;
		std::string _statusMessage;
		std::map<std::string, std::string> _headers;
		std::string _body;
		std::string _response;
		response_state	_state;

		// new
		LocationConfig	_matched_location;
		bool			_hasMatchedLocation;
};

#endif // HTTPRESPONSE_HPP
