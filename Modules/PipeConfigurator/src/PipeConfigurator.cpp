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

#include "PipeConfigurator.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <boost/property_tree/json_parser.hpp>
#include <sys/stat.h>
#include <errno.h>


DEFAULT_EXPORT_ALL(PipeConfigurator, "Pipeline configurator", "", false)

PipeConfigurator::PipeConfigurator(const std::string& mid) : poma::BaseModule<PomaDataType>(mid) {}

void PipeConfigurator::initialize()
{
    int ifd{-1}, ofd{-1};
    if ((ifd = mkfifo(m_inputfifo_path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        if (errno != EEXIST) {
            std::cerr << "Failed to open input FIFO " << m_inputfifo_path << std::endl;
            exit (1);
        }
    }

    if ((ofd = mkfifo(m_outputfifo_path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
        if (errno != EEXIST) {
            std::cerr << "Failed to open output FIFO " << m_inputfifo_path << std::endl;
            exit (1);
        }
    }
    m_thread = std::thread{&PipeConfigurator::process_fifo, this};
}

void PipeConfigurator::setup_cli(boost::program_options::options_description& desc) const
{
    boost::program_options::options_description pipeconf("PipeConfigurator options");
    pipeconf.add_options()
    ("inputpipe", boost::program_options::value<std::string>()->default_value("pomaconfigurator.input"), "FIFO input (command) path")
    ("outputpipe", boost::program_options::value<std::string>()->default_value("pomaconfigurator.output"), "FIFO output (result) path");
    desc.add(pipeconf);
}

void PipeConfigurator::process_cli(boost::program_options::variables_map& vm)
{
    m_inputfifo_path = vm["inputpipe"].as<std::string>();
    m_outputfifo_path = vm["outputpipe"].as<std::string>();
}


PipeConfigurator::~PipeConfigurator()
{
    m_instream.close();
    m_outstream.close();
    remove(m_inputfifo_path.c_str());
    remove(m_outputfifo_path.c_str());
}



void PipeConfigurator::process_fifo()
{

    m_instream.open(m_inputfifo_path.c_str(), std::ifstream::in);
    m_outstream.open(m_outputfifo_path.c_str(), std::ifstream::out);

    while(1) {
        std::string line;
        while (std::getline(m_instream, line)) {
            std::istringstream iss(line);
            boost::property_tree::ptree pt;
            boost::property_tree::ptree result;
            try {
                boost::property_tree::json_parser::read_json(iss, pt);
                std::string cmd = pt.get("command", "");
                std::cout << m_module_id << " command: " << cmd << std::endl;
                if (cmd == "get") {
                    std::string channel = pt.get("channel", "");
                    std::string name = pt.get("name", "");
                    result.put("result", read_property(name, channel) );
                } else if (cmd == "set") {
                    std::string channel = pt.get("channel", "");
                    std::string name = pt.get("name", "");
                    std::string value = pt.get("value", "");
                    result.put("result", write_property(name, value, channel) );
                } else if (cmd == "enumerate") {
                    std::string channel = pt.get("channel", "");
                    result.put("result", enumerate_properties(channel) );
                } else if (cmd == "channels") {
                    std::stringstream ss;
                    bool first{true};
                    for (const auto& c: get_channels()) {
                        if(!first) ss << ",";
                        first = false;
                        ss << c;
                    }
                    result.put("result", ss.str());
                } else {
                    result.put("result", "invalid");
                }
            } catch (...) {
                result.put("result", "bad");
            }

            std::stringstream ss;
            boost::property_tree::write_json(ss, result);
            std::cout << m_module_id << " result: " << ss.str() << std::endl;
            m_outstream << ss.str() << std::endl;
            m_instream.clear();
        }
        if (m_instream.eof()) {
            m_instream.clear();
        }
    }
}
