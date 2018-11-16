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

#ifndef DISTRIBUTEDCONFIGGENERATOR_H
#define DISTRIBUTEDCONFIGGENERATOR_H

#include "Common.h"
#include <string>
#include <istream>
#include <sstream>
#include <set>
#include <map>

namespace poma {

    class DistributedConfigGenerator {
    public:
        DistributedConfigGenerator(const std::string& p_source_mid, const std::map<std::string,Module>& p_modules, const std::vector<Link> &p_links, unsigned int base_port = 6000);
        void process();
        void get_config(const std::string& host,
                        std::string& p_source_mid, std::map<std::string,Module>& p_modules, std::vector<Link>& p_links);

        std::set<std::string> hosts() const;
        std::set<std::string> modules(const std::string& host) const;

    private:
        std::unordered_multimap<std::string,Module> m_host_modules_map;
        std::unordered_multimap<std::string,Link> m_host_link_map;
        std::unordered_map<std::string,std::string> m_module_host_map;

        const std::string& m_source_mid;
        const std::map<std::string,Module>& m_modules;
        const std::vector<Link>& m_links;
        unsigned int m_base_port;
    };

}

#endif
