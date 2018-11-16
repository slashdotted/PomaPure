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

#include <boost/property_tree/json_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <cstdlib>
#include "DistributedConfigGenerator.h"
#include <iostream>

namespace poma {

    DistributedConfigGenerator::DistributedConfigGenerator(const std::string &p_source_mid,
                                                                 const std::map<std::string, Module> &p_modules,
                                                                 const std::vector<Link> &p_links,
                                                                 unsigned int base_port)
            : m_source_mid{p_source_mid}, m_modules{p_modules}, m_links{p_links}, m_base_port{base_port} {
    }

    void DistributedConfigGenerator::process()
    {
        for (auto &it : m_modules) {
            auto v{m_modules.find(it.first)};
            const Module &mod{v->second};
            m_host_modules_map.insert(std::make_pair(mod.mhost, mod));
            m_module_host_map.insert(std::make_pair(mod.mid, mod.mhost));
        }

        std::string source_host{m_module_host_map.find(m_source_mid)->second};
        Link lnkprocessor;
        lnkprocessor.fid = Module::source(source_host);
        lnkprocessor.tid = m_source_mid;
        m_host_link_map.insert(std::make_pair(source_host, lnkprocessor));
        for (auto &lnk : m_links) {
            if (lnk.fid == "" || m_module_host_map.count(lnk.fid) == 0) {
                die("invalid link definition: invalid or no source specified: " + lnk.fid + ", to " + lnk.tid +
                    ", channel " + lnk.channel);
            }
            if (lnk.tid == "" || m_module_host_map.count(lnk.tid) == 0) {
                die("invalid link definition: invalid or no destination specified: " + lnk.tid + ", from " + lnk.fid +
                    ", channel " + lnk.channel);
            }
            if (lnk.channel == "") {
                die("invalid link definition: channel cannot be empty");
            }
            std::string fhost{m_module_host_map[lnk.fid]};
            std::string thost{m_module_host_map[lnk.tid]};
            if (fhost == thost) {
                std::cout << fhost << "<->" << thost << std::endl;
                m_host_link_map.insert(std::make_pair(fhost, lnk));
            } else {
                // Insert ZeroMQ bridge
                Module sink;
                sink.mid = Module::unique("__net_sink_");
                sink.mtype = "ZeroMQSink";
                sink.mhost = fhost;
                sink.mparams["sinkaddress"] = Module::address(thost, m_base_port);
                sink.mparams["#bandwidth"] = lnk.bandwidth;
                Module source;
                source.mid = Module::unique("__net_source");
                source.mtype = "ZeroMQSource";
                source.mhost = thost;
                source.mparams["sourceaddress"] = Module::address("*", m_base_port);
                source.mparams["#bandwidth"] = lnk.bandwidth;
                ++m_base_port;
                m_host_modules_map.insert(std::make_pair(sink.mhost, sink));
                m_host_modules_map.insert(std::make_pair(source.mhost, source));
                m_module_host_map[sink.mid] = sink.mhost;
                m_module_host_map[source.mid] = source.mhost;
                Link lnksink;
                lnksink.fid = lnk.fid;
                lnksink.tid = sink.mid;
                lnksink.channel = lnk.channel;
                lnksink.debug = lnk.debug;
                lnksink.bandwidth = lnk.bandwidth;
                m_host_link_map.insert(std::make_pair(fhost, lnksink));
                Link lnksource;
                lnksource.fid = source.mid;
                lnksource.tid = lnk.tid;
                lnksource.channel = lnk.channel;
                lnksource.debug = lnk.debug;
                lnksource.bandwidth = lnk.bandwidth;
                m_host_link_map.insert(std::make_pair(thost, lnksource));
                Link lnkprocessor;
                lnkprocessor.fid = Module::source(thost);
                lnkprocessor.tid = source.mid;
                m_host_link_map.insert(std::make_pair(thost, lnkprocessor));
            }
        }

        // Each host must have a ParProcessor module as source
        for (auto const &h: hosts()) {
            Module source_pp;
            source_pp.mid = Module::source(h);
            source_pp.mhost = h;
            source_pp.mtype = "ParProcessor";
            m_host_modules_map.insert(std::make_pair(source_pp.mhost, source_pp));
            m_module_host_map.insert(std::make_pair(source_pp.mid, source_pp.mhost));
        }
    }


    void DistributedConfigGenerator::get_config(const std::string &host, std::string &p_source_mid,
                                                      std::map<std::string, Module> &p_modules,
                                                      std::vector<Link> &p_links) {
        p_source_mid = Module::source(host);

        auto iter = m_host_modules_map.equal_range(host);
        for (auto it = iter.first; it != iter.second; ++it) {
            const Module& module{it->second};
            p_modules[module.mid] = module;
        }
        auto iter2 = m_host_link_map.equal_range(host);
        for (auto it = iter2.first; it != iter2.second; ++it) {
            p_links.push_back(it->second);
        }
    }

    std::set<std::string> DistributedConfigGenerator::hosts() const {
        std::set<std::string> hosts;
        for(auto const& h: m_host_modules_map) {
            hosts.insert(h.first);
        }
        return hosts;
    }

    std::set<std::string> DistributedConfigGenerator::modules(const std::string &host) const {
        std::set<std::string> modules;
        auto iter = m_host_modules_map.equal_range(host);
        for (auto it = iter.first; it != iter.second; ++it) {
            const Module& module{it->second};
            modules.insert(module.mtype);
        }
        return modules;
    }

}