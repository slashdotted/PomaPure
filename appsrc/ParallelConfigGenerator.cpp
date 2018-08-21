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

#include "ParallelConfigGenerator.h"
#include <boost/property_tree/json_parser.hpp>
#include <cstdlib>

namespace poma {
	
ParallelConfigGenerator::ParallelConfigGenerator(std::istream& pipeline)
{
	boost::property_tree::ptree pt;
    boost::property_tree::json_parser::read_json(pipeline, pt);
    boost::property_tree::ptree jmodules{pt.get_child("modules.")};
    m_source_mid = pt.get("source", "");
	// Process modules
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
            } else {
                mod.mparams[md.first] = md.second.data();
            }
        }
        if (mod.mtype == "") {
            die("invalid module type definition: " + mod.mtype);
        }
        m_modules.push_back(mod);
    }
	// Process links
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

        if (lnk.channel == "") {
            die("invalid link definition: channel cannot be empty");
        }
        m_links.push_back(lnk);
    }
}

void ParallelConfigGenerator::die(const std::string& msg)
{
    std::cerr << msg << std::endl;
    std::exit(-1);
}

void ParallelConfigGenerator::generate(std::ostream& output)
{
	for(unsigned int i{0}; i<m_modules.size(); i++) {
	    Module& m{m_modules[i]};
	    if (m.mparams.count("#parallel") > 0) {
	        create_fork_bridge(m);
	    }
	}
    boost::property_tree::ptree pt;
    boost::property_tree::ptree modules;
    boost::property_tree::ptree links;
    pt.put("source", m_source_mid);
    for (const auto& module : m_modules) {
        boost::property_tree::ptree data;
        data.put("type", module.mtype);
        data.put("#host", module.mhost);
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

    for (const auto& link : m_links) {
        boost::property_tree::ptree data;
        data.put("from", link.fid);
        data.put("to", link.tid);
        data.put("channel", link.channel);
        data.put("debug", link.debug);
        links.push_back(std::make_pair("", data));
    }

    pt.add_child("modules", modules);
    pt.add_child("links", links);
    boost::property_tree::write_json (output, pt, true);
}

void ParallelConfigGenerator::create_fork_bridge(Module m)
{
	Module fork;
	fork.mid = Module::unique("__parexecutor_");
	fork.mtype = "ParExecutor";
	fork.mhost = m.mhost;
	fork.mparams["#autogenerated"] = "true";
	m_modules.push_back(fork);
	Module joiner;
	joiner.mid = Module::unique("__joiner_");
	joiner.mtype = "Joiner";
	joiner.mhost = m.mhost;
    joiner.mparams["#autogenerated"] = "true";
	m_modules.push_back(joiner);
    // Fix links (that now go through the ParExecutor module
	for (auto& l : m_links) {
	    if (l.tid == m.mid) {
            // A->B becomes A->F
	        l.tid = fork.mid;
	    } else if (l.fid == m.mid) {
	        // B->C becomes
            l.fid = fork.mid;
	    }
	}
	// Create link from the ParExecutor to the original module
	Link templatefork;
	templatefork.fid = fork.mid;
	templatefork.tid = m.mid;
	templatefork.channel = "template";
	m_links.push_back(templatefork);
    // Create link from the original module to the Joiner
	Link joinerlink;
	joinerlink.fid = m.mid;
	joinerlink.tid = joiner.mid;
	joinerlink.channel = "default";
	m_links.push_back(joinerlink);
    // Create link from the ParExecutor to the original module
	Link backlink;
	backlink.fid = joiner.mid;
	backlink.tid = fork.mid;
	backlink.channel = "_join";
	m_links.push_back(backlink);
}
	
	
}
