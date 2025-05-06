/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zakaria <zakaria@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/10 16:09:58 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/28 17:53:10 by zakaria          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <cstddef>
#include <string>
#include <map>
#include "../include/LocationConfig.hpp"

class Client;

class HTTPRequest 
{
	public:
		// Orthodox Canonical Form
		HTTPRequest();
		HTTPRequest(Client* client);
		HTTPRequest(const HTTPRequest& other);
		HTTPRequest& operator=(const HTTPRequest& other);
		~HTTPRequest();

		// Public interface
		const std::string& getMethod() const;
		const std::string& getPath() const;
		const std::string& getHeaderValue(const std::string& key) const;
		int getState() const;
		int getStatusCode() const;
		const std::string&	getUri() const;
		void parse(const std::string& rawdata);
		const std::string&	getBody() const;
		bool shouldKeepAlive() const;
		void	parseHeadersKey(void);
		void	parseHeadersValue(void);
		void    checkHeadersEnd();
		int	checkCgi(void);

		// int	checkMethod(void);
		int	checkTransferEncoding(void);
		LocationConfig findMatchingLocation(const std::string& requestUri) const;
		void	defineBodyDestination(void);
		int	checkClientMaxBodySize(void);
		void findLocation(void);
		const LocationConfig* findLocationByPath(const std::string& path) const;
		const LocationConfig* getMatchedLocation() const;
		const LocationConfig* getFinalLocation() const;

		enum ParseState {
			INIT, LINE_METHOD, LINE_URI, LINE_VERSION, LINE_END,
			HEADERS_INIT, HEADER_KEY, HEADER_VALUE, HEADERS_END, BODY_INIT, BODY_END, FINISH
		};

	private:
		// Types and constants

		static const size_t MAX_URI_LENGTH = 8000;
		static const size_t MAX_QUERY_LENGTH = 8000;
		static const size_t MAX_BODY_SIZE = 1000000; // 1MB

		// Member variables
		Client*             _client;
		int                 _statusCode;
		ParseState          _state;
		size_t              _parsePosition;
		size_t              _contentLength;
		std::string         _method;
		std::string         _uri;
		std::string         _path;
		std::string         _query;
		std::string         _protocol;
		std::string         _body;
		std::string         _tmpHeaderKey;
		std::string         _tmpHeaderValue;
		std::string         _request;
		std::map<std::string, std::string> _headers;
		std::string         _contentType;
		std::string         _boundary;
		bool                _isChunked;
		std::string         _host;
		bool                _keepAlive;
		int		    _chunkSize;
		const LocationConfig* _matchedLocation;
		const LocationConfig* _finalLocation; 

		// Helper functions
		void setStatusCode(int code);
		void setState(ParseState state);
		bool isUriCharacter(char c);

		// Parsing functions
		void parseRequestLine();
		void parseMethod();
		void parseUri();
		void parseVersion();
		void checkLineEnd();

		void parseChunkBody();

		void parseRequestHeader();
		void parseHeaderLine();
		std::string parseHeaderValue();
		bool isVersionChar(char c) const;
		bool isValidHeaderKeyChar(char c) const;



		void parseRequestBody();
		// Debug functions
		void debugPrintRequest() const;
};

#endif // HTTPREQUEST_HPP
