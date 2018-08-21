/*
 * Copyright (C)2015,2016,2017 Amos Brocco (amos.brocco@supsi.ch)
 *                             Scuola Universitaria Professionale della
 *                             Svizzera Italiana (SUPSI)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Scuola Universitaria Professionale della Svizzera
 *       Italiana (SUPSI) nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without
 *       specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/    
    
#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <boost/algorithm/string.hpp>

namespace poma {
    
struct Module {
	static int counter;
	static std::string source(const std::string& host)
	{
		std::stringstream ss;
		ss << "__parallel_source_" << boost::replace_all_copy(host, ".", "_");
		return ss.str();
	}
	static std::string unique(const std::string& prefix)
	{
		std::stringstream ss;
		ss << prefix << ++counter;
		return ss.str();
	}
	static std::string address(const std::string& host, unsigned int port)
	{
		std::stringstream ss;
		ss << "tcp://" << host << ":" << port;
		return ss.str();
	}
	std::string mid;
	std::string mtype;
	std::string mhost{"localhost"};
	std::vector<std::string> cparams;
	std::unordered_map<std::string,std::string> mparams;
};

struct Link {
	std::string fid;
	std::string tid;
	std::string channel{"default"};
	bool debug{false};
};

}

#endif
