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
    
#include "Common.h"
#include <iostream>
#include <cstdlib>
#include <boost/property_tree/json_parser.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

namespace poma {

int Module::counter{0};

std::set<std::string> extract_hosts(std::istream& pipeline) {
    std::set<std::string> result;
    boost::property_tree::ptree pt;
    boost::property_tree::json_parser::read_json(pipeline, pt);
    boost::property_tree::ptree jmodules{pt.get_child("modules.")};
    // Process modules
    for(auto &m : jmodules) {
        Module mod;
        mod.mid = m.first.data();
        if (mod.mid.find("#") == 0) continue; // disabled modules
        for (auto &md : m.second) {
            if (md.first == "#host") {
                result.insert(md.second.data());
            }
        }
    }
    return result;
}

void FlagBag::clear() {
    m_required.clear();
    m_forbidden.clear();
    m_less_than.clear();
    m_greater_than.clear();
    m_equal_to.clear();
}

void FlagBag::parse_flags(const std::string flags) {
    typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
    boost::char_separator<char> sep{",!<>="};
    tokenizer tok{flags, sep};
    for (tokenizer::iterator ti = tok.begin();
         ti != tok.end();) {
        auto &token{*ti};
        if (token == ",") {
            ++ti;
        } else if (token == "!") {
            ++ti;
            if (ti == tok.end()) {
                die("Unexpected end of input in flag declaration");
            } else if (*ti == "," || *ti == ">" || *ti == "<" || *ti == "=") {
                die("Expecting flag label after negation");
            } else {
                m_required.push_back(*ti);
                ++ti;
                if (ti != tok.end() || *ti != ",") {
                    die("Malformed flag declaration");
                }
            }
        } else if (*ti == "," || *ti == ">" || *ti == "<" || *ti == "=") {
            die("Malformed flag declaration");
        } else {
            std::string flag{*ti};
            ++ti;
            if (token == ",") {
                m_required.push_back(flag);
                ++ti;
            } else if (*ti == ">" || *ti == "<" || *ti == "=") {
                std::string comparator{*ti};
                ++ti;
                if (ti != tok.end() || *ti != "," || *ti == ">" || *ti == "<" || *ti == "=") {
                    die("Malformed flag declaration");
                }
                double value{0};
                try {
                    value = boost::lexical_cast<double>(*ti);
                }
                catch (const boost::bad_lexical_cast &) {
                    die("Expecting numerical value in flag declaration");
                }
                if (comparator == ">") {
                    m_greater_than[flag] = value;
                } else if (comparator == "<") {
                    m_less_than[flag] = value;
                } else {
                    m_equal_to[flag] = value;
                }
                ++ti;
            }
        }
    }
}

void load(std::istream& pipeline, std::string& p_source_mid, std::map<std::string,Module>& p_modules, std::vector<Link>& p_links) {
    boost::property_tree::ptree pt;
    boost::property_tree::json_parser::read_json(pipeline, pt);
    boost::property_tree::ptree jmodules{pt.get_child("modules.")};
    p_source_mid = pt.get("source", "");
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
            } else if (md.first == "#flags") {
                mod.mflags = FlagBag{};
            } else {
                mod.mparams[md.first] = md.second.data();
            }
        }
        if (mod.mtype == "") {
            die("invalid module type definition: " + mod.mtype);
        }
        p_modules[mod.mid] = mod;
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
            } else if (ld.first == "bandwidth") {
                lnk.bandwidth = ld.second.data();
            }
        }
        if (lnk.fid.find("#") == 0 || lnk.tid.find("#") == 0) continue; // commented modules

        if (lnk.channel == "") {
            die("invalid link definition: channel cannot be empty");
        }
        p_links.push_back(lnk);
    }
}

void generate(const std::string& p_source_mid, const std::map<std::string,Module>& p_modules, const std::vector<Link>& p_links, std::ostream& output) {
    boost::property_tree::ptree pt;
    boost::property_tree::ptree modules;
    boost::property_tree::ptree links;
    pt.put("source", p_source_mid);
    for (const auto &it : p_modules) {
        const Module &module{it.second};
        boost::property_tree::ptree data;
        data.put("type", module.mtype);
        data.put("#host", module.mhost);
        for (auto &pit : module.mparams) {
            data.put(pit.first, pit.second);
        }
        if (module.cparams.size() > 0) {
            boost::property_tree::ptree cpar;
            for (const auto &v : module.cparams) {
                boost::property_tree::ptree value;
                value.put("", v);
                cpar.push_back(std::make_pair("", value));
            }
            data.add_child("cparams", cpar);
        }
        modules.add_child(module.mid, data);
    }

    for (const auto &link : p_links) {
        boost::property_tree::ptree data;
        data.put("from", link.fid);
        data.put("to", link.tid);
        data.put("channel", link.channel);
        data.put("debug", link.debug);
        data.put("bandwidth", link.bandwidth);
        links.push_back(std::make_pair("", data));
    }

    pt.add_child("modules", modules);
    pt.add_child("links", links);
    boost::property_tree::write_json(output, pt, true);
}

void die(const std::string& msg)
{
    std::cerr << msg << std::endl;
    std::exit(-1);
}

}
