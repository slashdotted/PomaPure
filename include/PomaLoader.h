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

#ifndef POMALOADER_H
#define POMALOADER_H

#include <unordered_map>
#include <string>
#include <istream>
#include "PomaDefault.h"

namespace poma {

class Loader {
public:
    void start_processing();

    PomaModuleType* get_source();
    std::shared_ptr<PomaModuleType> get_module(const std::string& identifier);

    void enumerate_libraries(const std::string& lib_path, const std::string& extension = ".so");
    void load_library(const std::string& so_path);

    void parse_json_config(const std::string& pipeline);
    void parse_json_config(std::istream& stream);

    void configure(int argc, char* argv[]);
    void initialize();
    void flush();
private:
    void parse_cli_config(boost::program_options::variables_map& vm, int argc, char* argv[]);
    void parse_json_config(boost::program_options::variables_map& vm, int argc, char* argv[]);

    boost::property_tree::ptree parse_json(const char* file);
    boost::property_tree::ptree parse_json(std::istream& stream);

    void die(const std::string& msg);
    std::shared_ptr<PomaModuleType> get_instance(const std::string& type, const std::string& name);

    PomaModuleType* m_source {nullptr};

    std::map<std::string, boost::shared_ptr<ModuleFactory<PomaModuleType>>> m_factories;
    std::unordered_map<std::string, std::shared_ptr<PomaModuleType>> m_modules;
};

}

#endif

