/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HTTPRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/10 16:09:58 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/19 20:49:09 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>

class Client;

class HTTPRequest 
{
	public:
		enum ParseState 
		{
			INIT,
			LINE_METHOD,
			LINE_URI,
			LINE_VERSION,
			LINE_END,
			HEADERS_INIT,
			HEADERS_KEY,
			HEADERS_VALUE,
			HEADERS_END,
			BODY_INIT,
			BODY_PROCESS,
			BODY_END,
			FINISH
		};

		HTTPRequest(Client* client);
		~HTTPRequest();

		// Getters
		const std::string& getMethod() const;
		const std::string& getPath() const;
		const std::string& getHeaderValue(const std::string& key) const;
		int getState() const;
		int getStatusCode() const;
		const std::string&	getUri() const;
		const std::string&	getBody() const;

		// Parsing control
		void parse(const std::string& rawdata);
		void setState(ParseState state);
		void setStatusCode(int code);

		// Parsing methods
		void parseRequestLine();
		void parseRequestHeader();
		void parseRequestBody();
		void parseMethod();
		void parseUri();
		void parseVersion();
		void checkLineEnd();
		void parseHeaderKey();
		void parseHeaderValue();
		void parseHeaderEnd();
		void checkHeaderEnd();

		void print();
		// const std::string&	getHeader(const std::string &key) const;
		// bool	is_well_formed();

	private:
		std::string _request;
		std::map<std::string, std::string> _headers;
		std::string _method;
		std::string _uri;
		std::string _path;
		std::string _query;
		std::string _protocol;
		std::string _body;
		std::string _tmpHeaderKey;
		std::string _tmpHeaderValue;
		Client* _client;
		int _statusCode;
		ParseState _state;
		size_t _parsePosition;
		size_t _contentLength;
};

#endif // HTTPREQUEST_HPP
