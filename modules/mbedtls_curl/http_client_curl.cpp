/*************************************************************************/
/*  http_client_curl.cpp                                                 */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "http_client_curl.h"

#include "core/string/print_string.h"
#include "core/templates/ring_buffer.h"

char const *HTTPClientCurl::methods[10] = {
	"GET",
	"HEAD",
	"POST",
	"PUT",
	"DELETE",
	"OPTIONS",
	"TRACE",
	"CONNECT",
	"PATCH",
	"MAX",
};

void HTTPClientCurl::make_default() {
	print_verbose("Libcurl HTTP Client enabled.");
	_create = _create_func;
}

HTTPClient *HTTPClientCurl::_create_func() {
	return memnew(HTTPClientCurl);
}

size_t HTTPClientCurl::_header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
	HTTPClientCurl *client = (HTTPClientCurl *)userdata;
	String s((const char *)buffer);
	Vector<String> parts = s.split(":");
	if (parts.size() != 2) {
		return size * nitems;
	}

	client->response_headers.push_back(s);

	String header = parts[0].to_lower();
	String val = parts[1];
	// Trim any whitespace prefix
	while (val.begins_with(" ")) {
		val = val.trim_prefix(" ");
	}

	// Use the content length to determine body size.
	if (header == "content-length") {
		client->body_size = val.to_int();
	}

	// If the Connection header is set to "close" then
	// keep-alive isn't enabled.
	if (header == "connection" && val == "close") {
		client->keep_alive = false;
	}

	if (header == "transfer-encoding" && val == "chunked") {
		client->body_size = -1;
		client->chunked = true;
	}

	client->response_available = true;

	return size * nitems;
}

size_t HTTPClientCurl::_read_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
	RingBuffer<uint8_t> *b = (RingBuffer<uint8_t> *)userdata;
	int n = b->copy((uint8_t *)buffer, 0, size * nitems);
	return n;
}

size_t HTTPClientCurl::_write_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
	HTTPClientCurl *client = (HTTPClientCurl *)userdata;
	PackedByteArray chunk;
	chunk.resize(size * nitems);
	memcpy(chunk.ptrw(), buffer, size * nitems);
	client->response_chunks.append(chunk);
	client->status = STATUS_BODY;

	return size * nitems;
}

curl_slist *HTTPClientCurl::_ip_addr_to_slist(const IPAddress &p_addr) {
	String addr = String(p_addr);
	if (addr.find(":") != -1) {
		addr = "[" + addr + "]";
	}
	String h = host + ":" + String::num_int64(port) + ":" + addr;
	return curl_slist_append(nullptr, h.ascii().get_data());
}

String HTTPClientCurl::_hostname_from_url(const String &p_url) {
	String hostname = p_url.trim_prefix("http://");
	hostname = hostname.trim_prefix("https://");
	return hostname.split("/")[0];
}

Error HTTPClientCurl::_resolve_dns() {
	IP::ResolverStatus rstatus = IP::get_singleton()->get_resolve_item_status(resolver_id);
	switch (rstatus) {
		case IP::RESOLVER_STATUS_WAITING:
			return OK;
		case IP::RESOLVER_STATUS_DONE: {
			addr = IP::get_singleton()->get_resolve_item_address(resolver_id);

			Error err = _request(true);

			IP::get_singleton()->erase_resolve_item(resolver_id);
			resolver_id = IP::RESOLVER_INVALID_ID;

			if (err != OK) {
				status = STATUS_CANT_CONNECT;
				return err;
			}
			return OK;
		} break;
		case IP::RESOLVER_STATUS_NONE:
		case IP::RESOLVER_STATUS_ERROR: {
			IP::get_singleton()->erase_resolve_item(resolver_id);
			resolver_id = IP::RESOLVER_INVALID_ID;
			close();
			status = STATUS_CANT_RESOLVE;
			return ERR_CANT_RESOLVE;
		} break;
	}

	return OK;
}

Error HTTPClientCurl::_poll_curl() {
	CURLMcode rc = curl_multi_perform(curl, &still_running);
	if (still_running) {
		rc = curl_multi_wait(curl, nullptr, 0, 1000, nullptr);
	}

	if (rc != CURLM_OK) {
		ERR_PRINT_ONCE("Curl multi error while performing. RC: " + String::num_int64(rc));
		return FAILED;
	}

	if (still_running == 0) {
		int n = 0;
		CURLMsg *msg = curl_multi_info_read(curl, &n);
		if (msg && msg->msg == CURLMSG_DONE) {
			if (msg->data.result != CURLE_OK) {
				ERR_PRINT_ONCE("Curl result failed. RC: " + String::num_int64(msg->data.result));
				status = STATUS_DISCONNECTED;
				return FAILED;
			}
			CURLcode return_code = curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &response_code);
			if (return_code != CURLE_OK) {
				ERR_PRINT_ONCE("Couldnt get curl status code. RC:" + String::num_int64(return_code));
				return FAILED;
			}

			curl_multi_remove_handle(curl, msg->easy_handle);
			curl_easy_cleanup(msg->easy_handle);
			in_flight = false;
		}
	}
	return OK;
}

void HTTPClientCurl::_init_upload(CURL *p_chandle, Method p_method, uint8_t *p_body, int p_body_size) {
	RingBuffer<uint8_t> b;
	b.resize(p_body_size);
	b.write(p_body, p_body_size);

	// Special cases for POST and PUT to configure uploads.
	switch (p_method) {
		case METHOD_GET:
			break;
		case METHOD_HEAD:
			break;
		case METHOD_POST:
			curl_easy_setopt(p_chandle, CURLOPT_POST, 1L);
			curl_easy_setopt(p_chandle, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)p_body_size);
			break;
		case METHOD_PUT:
			curl_easy_setopt(p_chandle, CURLOPT_UPLOAD, 1L);
			curl_easy_setopt(p_chandle, CURLOPT_INFILESIZE_LARGE, (curl_off_t)p_body_size);
			break;
		case METHOD_DELETE:
			break;
		case METHOD_OPTIONS:
			break;
		case METHOD_TRACE:
			break;
		case METHOD_CONNECT:
			break;
		case METHOD_PATCH:
			break;
		case METHOD_MAX:
			break;
	}

	// Somewhat counter intuitively, the read function is actually used by libcurl to send data,
	// while the write function (see below) is used by libcurl to write response data to storage
	// (or in our case, memory).
	curl_easy_setopt(p_chandle, CURLOPT_READFUNCTION, _read_callback);
	curl_easy_setopt(p_chandle, CURLOPT_READDATA, &b);
	read_buffer = b;
}


Error HTTPClientCurl::_init_dns(CURL *p_chandle, IPAddress p_addr) {
	// TODO: Support resolving multiple addresses.
	curl_slist *h = _ip_addr_to_slist(p_addr);
	CURLcode return_code = curl_easy_setopt(p_chandle, CURLOPT_RESOLVE, h);
	if (return_code != CURLE_OK) {
		ERR_PRINT("failed to initialize dns resolver: " + String::num_int64(return_code));
		return FAILED;
	}
	return OK;
}

Error HTTPClientCurl::_init_request_headers(CURL *p_chandler, Vector<String> p_headers) {
	curl_slist* h = nullptr;
	for (int i = 0; i < p_headers.size(); i++) {
		h = curl_slist_append(h, p_headers[i].ascii().get_data());
	}
	if (h) {
		CURLcode return_code = curl_easy_setopt(p_chandler, CURLOPT_HTTPHEADER, h);
		if (return_code != CURLE_OK) {
			ERR_PRINT("failed to set request headers: " + String::num_uint64(return_code));
			return FAILED;
		}
	}
	return OK;
}

Error HTTPClientCurl::_request(bool p_init_dns) {
	String a = addr;
	if (a.find(":") != -1) {
		a = "[" + a + "]";
	}
	CURL *eh = curl_easy_init();
	curl_easy_setopt(eh, CURLOPT_URL, (scheme + host + ":" + String::num_int64(port) + url).ascii().get_data());
	curl_easy_setopt(eh, CURLOPT_CUSTOMREQUEST, methods[(int)method]);
	curl_easy_setopt(eh, CURLOPT_BUFFERSIZE, read_chunk_size);
	Error err;
	if (p_init_dns) {
		err = _init_dns(eh, addr);
		if (err != OK) {
			return err;
		}
	}

	if (request_body_size > 0) {
		_init_upload(eh, method, (uint8_t *)request_body, request_body_size);
	}

	if (ssl) {
		curl_easy_setopt(eh, CURLOPT_USE_SSL, CURLUSESSL_ALL);
	}

	if (verify_host) {
		// When CURLOPT_SSL_VERIFYHOST is 2, that certificate must indicate
		// that the server is the server to which you meant to connect, or
		// the connection fails. Simply put, it means it has to have the same
		// name in the certificate as is in the URL you operate against.
		// @see https://curl.se/libcurl/c/CURLOPT_SSL_VERIFYHOST.html
		curl_easy_setopt(eh, CURLOPT_SSL_VERIFYHOST, 2L);
	}

	// Initialize callbacks.
	curl_easy_setopt(eh, CURLOPT_HEADERFUNCTION, _header_callback);
	curl_easy_setopt(eh, CURLOPT_HEADERDATA, this);
	curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, _write_callback);
	curl_easy_setopt(eh, CURLOPT_WRITEDATA, this);

	err = _init_request_headers(eh, request_headers);
	if (err != OK) {
		return err;
	}

	// Set the request context. CURLOPT_PRIVATE is just arbitrary data
	// that can be associated with request handlers. It's used here to
	// keep track of certain data that needs to be manipulated throughout
	// the pipeline.
	// @see https://curl.se/libcurl/c/CURLOPT_PRIVATE.html
	curl_easy_setopt(eh, CURLOPT_PRIVATE, this);

	curl_multi_add_handle(curl, eh);

	in_flight = true;
	status = STATUS_REQUESTING;
	still_running = 0;
	return OK;
}

Error HTTPClientCurl::get_response_headers(List<String> *r_response) {
	*r_response = response_headers;
	return OK;
}

Error HTTPClientCurl::connect_to_host(const String &p_host, int p_port, bool p_ssl, bool p_verify_host) {
	if (curl == nullptr) {
		curl = curl_multi_init();
	}

	response_code = 0;
	body_size = -1;
	response_headers.clear();
	response_available = false;
	response_code = 0;
	response_chunks.clear();
	keep_alive = false;
	status = STATUS_CONNECTED;
	scheme = p_host.begins_with("https://") ? "https://" : "http://";
	host = p_host.trim_prefix("http://").trim_prefix("https://");
	port = p_port;
	ssl = p_ssl;
	verify_host = p_verify_host;

	return OK;
}

HTTPClientCurl::~HTTPClientCurl() {
	close();
}

void HTTPClientCurl::close() {
	if (curl) {
		curl_multi_cleanup(curl);
		curl = nullptr;
	}
}

Error HTTPClientCurl::request(Method p_method, const String &p_url, const Vector<String> &p_headers, const uint8_t *p_body, int p_body_size) {
	// Only one request can be in flight at a time.
	if (in_flight) {
		return ERR_ALREADY_IN_USE;
	}

	method = p_method;
	url = p_url;
	request_headers = p_headers;
	request_body = p_body;
	request_body_size = p_body_size;

	if (host.is_valid_ip_address()) {
		_request(false);
	} else {
		resolver_id = IP::get_singleton()->resolve_hostname_queue_item(host, IP::Type::TYPE_ANY);
		status = STATUS_RESOLVING;
	}

	return OK;
}

Error HTTPClientCurl::poll() {
	if (status == STATUS_RESOLVING) {
		ERR_FAIL_COND_V(resolver_id == IP::RESOLVER_INVALID_ID, ERR_BUG);
		return _resolve_dns();
	}

	// Important! Since polling libcurl will greedily read response data from the
	// network we don't want to poll when we are in STATUS_BODY state. The reason
	// for this is that the HTTPClient API is expected to only read from the network
	// when read_response_body_chunk is called. This means that here, in poll, we only
	// poll libcurl when we are not in the STATUS_BODY state and we poll libcurl in
	// read_response_body_chunk instead, when we are in STATUS_BODY state.
	if (status != STATUS_BODY) {
		return _poll_curl();
	}
	return OK;
}

PackedByteArray HTTPClientCurl::read_response_body_chunk() {
	if (status == STATUS_BODY) {
		Error err = _poll_curl();
		if (err != OK) {
			ERR_PRINT_ONCE("Failed when polling curl in STATUS_BODY. RC: " + String::num_int64(err));
			return PackedByteArray();
		}
	}
	if (response_chunks.is_empty()) {
		status = keep_alive ? STATUS_CONNECTED : STATUS_DISCONNECTED;
		return PackedByteArray();
	}
	PackedByteArray chunk = response_chunks[0];
	response_chunks.remove_at(0);

	return chunk;
}
