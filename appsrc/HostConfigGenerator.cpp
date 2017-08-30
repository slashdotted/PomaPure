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
#include "HostConfigGenerator.h"
#include <iostream>

namespace poma {

int HostConfigGenerator::Module::counter{0};

HostConfigGenerator::HostConfigGenerator(std::istream& pipeline, unsigned int base_port)
{
    boost::property_tree::ptree pt;
    boost::property_tree::json_parser::read_json(pipeline, pt);
    boost::property_tree::ptree jmodules{pt.get_child("modules.")};
    std::string source_mid{pt.get("source", "")};

    for(auto &m : jmodules) {
        Module mod;
        mod.mid = m.first.data();
        if (mod.mid.find("#") == 0) continue; // disabled modules
        for (auto &md : m.second) {
            if (md.first == "type") {
                mod.mtype = md.second.data();
            } else if (md.first == "cparams") {
                for (auto &cp : md.second) {
                    mod.cparams.push_back(cp.second.data());
                }
            } else if (md.first == "#host") {
                mod.mhost = md.second.data();
            } else if (md.first.find("#") == 0) {
                continue; // comments
            } else {
                mod.mparams[md.first] = md.second.data();
            }
        }
        if (mod.mtype == "") {
            die("invalid module type definition: " + mod.mtype);
        }
        m_host_modules_map.insert(std::make_pair(mod.mhost,mod));
        m_module_host_map.insert(std::make_pair(mod.mid,mod.mhost));
    }

    std::string source_host{m_module_host_map.find(source_mid)->second};
    Link lnkprocessor;
    lnkprocessor.fid = Module::source(source_host);
    lnkprocessor.tid = source_mid;
    m_host_link_map.insert(std::make_pair(source_host,lnkprocessor));

    boost::property_tree::ptree jlinks{pt.get_child("links.")};
    for(auto &l : jlinks) {
        Link lnk;
        for (auto &ld : l.second) {
            if (ld.first == "from") {
                lnk.fid = ld.second.data();
            } else if (ld.first == "to") {
                lnk.tid = ld.second.data();
            } else if (ld.first == "channel") {
                lnk.channel = ld.second.data();
            } else if (ld.first == "debug") {
                if (ld.second.data() == "true") {
                    lnk.debug = true;
                }
            }
        }

        if (lnk.fid.find("#") == 0 || lnk.tid.find("#") == 0) continue; // commented modules

        if (lnk.fid == "" || m_module_host_map.count(lnk.fid) == 0) {
            die("invalid link definition: invalid or no source specified: " + lnk.fid);
        }
        if (lnk.tid == "" || m_module_host_map.count(lnk.tid) == 0) {
            die("invalid link definition: invalid or no destination specified: " + lnk.tid);
        }
        if (lnk.channel == "") {
            die("invalid link definition: channel cannot be empty");
        }
        std::string fhost{m_module_host_map[lnk.fid]};
        std::string thost{m_module_host_map[lnk.tid]};
        if (fhost == thost) {
            m_host_link_map.insert(std::make_pair(fhost,lnk));
        } else {
            // Insert ZeroMQ bridge
            Module sink;
            sink.mid = Module::unique("__net_sink_");
            sink.mtype = "ZeroMQSink";
            sink.mhost = fhost;
            sink.mparams["sinkaddress"] = Module::address(thost, base_port);
            Module source;
            source.mid = Module::unique("__net_source");
            source.mtype = "ZeroMQSource";
            source.mhost = thost;
            source.mparams["sourceaddress"] = Module::address("*", base_port);
            ++base_port;
            m_host_modules_map.insert(std::make_pair(sink.mhost,sink));
            m_host_modules_map.insert(std::make_pair(source.mhost,source));
            m_module_host_map[sink.mid] = sink.mhost;
            m_module_host_map[source.mid] = source.mhost;
            Link lnksink;
            lnksink.fid = lnk.fid;
            lnksink.tid = sink.mid;
            lnksink.channel = lnk.channel;
            lnksink.debug = lnk.debug;
            m_host_link_map.insert(std::make_pair(fhost,lnksink));
            Link lnksource;
            lnksource.fid = source.mid;
            lnksource.tid = lnk.tid;
            lnksource.channel = lnk.channel;
            lnksource.debug = lnk.debug;
            m_host_link_map.insert(std::make_pair(thost,lnksource));
            Link lnkprocessor;
            lnkprocessor.fid = Module::source(thost);
            lnkprocessor.tid = source.mid;
            m_host_link_map.insert(std::make_pair(thost,lnkprocessor));
        }
    }

    // Each host must have a ParProcessor module as source
    for(auto const& h: hosts()) {
        Module source_pp;
        source_pp.mid =  Module::source(h);
        source_pp.mhost = h;
        source_pp.mtype = "ParProcessor";
        m_host_modules_map.insert(std::make_pair(source_pp.mhost,source_pp));
        m_module_host_map.insert(std::make_pair(source_pp.mid,source_pp.mhost));
    }
}

std::set<std::string> HostConfigGenerator::hosts() const
{
    std::set<std::string> hosts;
    for(auto const& h: m_host_modules_map) {
        hosts.insert(h.first);
    }
    return hosts;
}

std::set<std::string> HostConfigGenerator::modules(const std::string& host) const
{
    std::set<std::string> modules;
    for (auto it = m_host_modules_map.find(host); it != m_host_modules_map.end(); it++) {
        const Module& module{it->second};
        modules.insert(module.mtype);
    }
    return modules;
}

void HostConfigGenerator::die(const std::string& msg)
{
    std::cerr << msg << std::endl;
    exit(-1);
}


std::string HostConfigGenerator::operator[](const std::string& host) const
{
    boost::property_tree::ptree pt;
    boost::property_tree::ptree modules;
    boost::property_tree::ptree links;
    pt.put("source", Module::source(host));

    for (auto it = m_host_modules_map.find(host); it != m_host_modules_map.end(); it++) {
        const Module& module{it->second};
        boost::property_tree::ptree data;
        data.put("type", module.mtype);
        for(auto& pit : module.mparams) {
            data.put(pit.first, pit.second);
        }
        if (module.cparams.size() > 0) {
            boost::property_tree::ptree cpar;
            for (const auto& v : module.cparams) {
                boost::property_tree::ptree value;
                value.put("", v);
                cpar.push_back(std::make_pair("", value));
            }
            data.add_child("cparams", cpar);
        }
        modules.add_child(module.mid, data);
    }

    for (auto it = m_host_link_map.find(host); it != m_host_link_map.end(); it++) {
        const Link& link{it->second};
        boost::property_tree::ptree data;
        data.put("from", link.fid);
        data.put("to", link.tid);
        data.put("channel", link.channel);
        data.put("debug", link.debug);
        links.push_back(std::make_pair("", data));
    }

    pt.add_child("modules", modules);
    pt.add_child("links", links);

    std::ostringstream out;
    boost::property_tree::write_json (out, pt, true);
    return out.str();
}

}
