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
#include <boost/program_options.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/dll/import.hpp>
#include <boost/any.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <exception>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <dlfcn.h>
#include <vector>
#include "PomaLoader.h"

namespace poma {

void Loader::load_library(const std::string& so_path)
{
    boost::filesystem::path lib_path{so_path.c_str()};
    boost::shared_ptr<ModuleFactory<PomaModuleType>> plugin_module;
    plugin_module = boost::dll::import<ModuleFactory<PomaModuleType>>(
                        lib_path.c_str(),
                        "factory",
                        boost::dll::load_mode::append_decorations
                    );
    m_factories[plugin_module->name()] = plugin_module;
}

void Loader::enumerate_libraries(const std::string& lib_path, const std::string& extension)
{
    boost::filesystem::path path {lib_path};
    boost::filesystem::recursive_directory_iterator it {path};
    while (it != boost::filesystem::recursive_directory_iterator()) {
        if (boost::filesystem::is_directory(it->path())) {
            std::string fname{it->path().filename().string()};
            if (fname[0] == '.') {
                it.no_push();
                ++it;
                continue;
            }
        }
        boost::filesystem::path file_ext = it->path().extension();
        if (file_ext == extension) {
            load_library(it->path().string());
        }
        ++it;
    }
}

boost::property_tree::ptree Loader::parse_json(const char* file)
{
    std::ifstream ifs;
    ifs.open(file);
    return parse_json(ifs);
}

boost::property_tree::ptree Loader::parse_json(std::istream& stream)
{
    boost::property_tree::ptree pt;
    boost::property_tree::json_parser::read_json(stream, pt);
    return pt;
}

std::shared_ptr<PomaModuleType> Loader::get_instance(const std::string& type, const std::string& name)
{
    if (m_factories.count(type) == 1) {
        return std::shared_ptr<PomaModuleType> {m_factories[type]->create(name)};
    } else {
        die("Cannot find module: " + type);
        return nullptr;
    }
}

void Loader::die(const std::string& msg)
{
    throw std::runtime_error{msg};
}

void Loader::parse_json_config(std::istream& stream)
{
    // Parse JSON pipeline description
    boost::property_tree::ptree jpt {parse_json(stream)};
    boost::property_tree::ptree jmodules = jpt.get_child("modules.");
    std::string source_mid{jpt.get("source", "")};

    for(auto &m : jmodules) {
        std::string mid {m.first.data()};
        if (mid.find("#") == 0) continue; // disabled modules
        std::string mtype;
        std::vector<std::string> cparams;
        std::map<std::string,std::string> mparams;
        for (auto &md : m.second) {
            if (md.first == "type") {
                mtype = md.second.data();
            } else if (md.first == "cparams") {
                for (auto &cp : md.second) {
                    cparams.push_back(cp.second.data());
                }
            } else {
                if (md.first.find("#") == 0) continue; // comments
                mparams[md.first] = md.second.data();
            }
        }
        // Prepare args
        std::string prefix {"--"};
        std::vector<std::string> sargv;
        sargv.push_back("loader");
        for (auto& it : mparams) {
            sargv.push_back(prefix + it.first);
            sargv.push_back(it.second);
        }
        std::vector<const char *> largv;
        largv.reserve(sargv.size());
        for (auto& s : sargv) {
            largv.push_back(s.c_str());
        }
        // Construct module
        boost::program_options::variables_map vm;
        boost::program_options::options_description desc("");
        auto mod = get_instance(mtype, mid);
        mod->construct(cparams);
        m_modules[mid] = mod;
        std::cout << "Registered module '" << mid << "'" << std::endl;
        if (m_source == nullptr) m_source = mod.get();
        mod->setup_cli(desc);
        boost::program_options::store(boost::program_options::parse_command_line(largv.size(), &largv[0], desc), vm);
        boost::program_options::notify(vm);
        mod->process_cli(vm);
    }
    std::cout << m_modules.size() << " modules registered" << std::endl;
    boost::property_tree::ptree jlinks = jpt.get_child("links.");
    for(auto &l : jlinks) {
        std::string from;
        std::string to;
        std::string channel {"default"};
        bool do_debug{false};
        for (auto &ld : l.second) {
            if (ld.first == "from") {
                from = ld.second.data();
            } else if (ld.first == "to") {
                to = ld.second.data();
            } else if (ld.first == "channel") {
                channel = ld.second.data();
            } else if (ld.first == "debug") {
                if (ld.second.data() == "true") {
                    do_debug = true;
                }
            }
        }

        if (from.find("#") == 0 || to.find("#") == 0) continue; // commented modules

        if (from == "" || m_modules.count(from) == 0) {
            die("invalid link definition: invalid or no source specified: " + from);
        }
        if (to == "" || m_modules.count(to) == 0) {
            die("invalid link definition: invalid or no destination specified: " + to);
        }
        if (channel == "") {
            die("invalid link definition: channel cannot be empty");
        }
        m_modules[from]->connect_sink(m_modules[to], channel, do_debug);
    }

    if (source_mid != "") {
        if (m_modules.count(source_mid) == 0) {
            die("Invalid source definition: module not found " + source_mid);
        } else {
            m_source = m_modules[source_mid].get();
            std::cout << "Source set to " << source_mid << std::endl;
        }
    }
}

void Loader::parse_json_config(boost::program_options::variables_map& vm, int argc, char* argv[])
{
    parse_json_config(vm["json"].as<std::string>());
}

void Loader::parse_json_config(const std::string& filename)
{
    std::ifstream ifs;
    ifs.open(filename.c_str());
    parse_json_config(ifs);
}

void Loader::parse_cli_config(boost::program_options::variables_map& vm, int argc, char* argv[])
{
    std::string line;
    std::ifstream infile;
    infile.open(vm["pipeline"].as<std::string>().c_str());
    if (!infile.good()) die("Cannot open pipeline description file");
    while (std::getline(infile, line)) {
        boost::trim(line);
        if (line.length() == 0) continue;
        std::vector<std::string> values;
        if (line.find("#") == 0) continue; // comments
        boost::split(values, line, boost::is_any_of("|"));
        if (values.size() < 3) die(line + ": invalid module definition");
        if (values.at(0)=="module") {
            auto name = values.at(1);
            auto mod = get_instance(values.at(2), name);
            values.erase(values.begin(), values.begin()+3);
            mod->construct(values);
            m_modules[name] = mod;
            if (m_source == nullptr) m_source = mod.get();
        } else if (values.at(0)=="link") {
            if (values.size() < 4) die(line + ": invalid link definition");
            auto from = values.at(1);
            auto channel = values.at(2);
            auto to = values.at(3);
            if (channel == "") channel = "default";
            if ((m_modules.count(from) > 0) && (m_modules.count(to) > 0)) {
                m_modules[from]->connect_sink(m_modules[to], channel);
            } else die(line + ": invalid link definition (missing module)");
        } else die(line + ": invalid pipeline directive");
    }
    infile.close();
    PomaModuleType::setup_cli_all(argc, argv);
}

void Loader::start_processing()
{
    if (m_source != nullptr) {
        std::cout << "start processing from " << m_source->get_module_id() << std::endl;
        m_source->start_processing();
        std::cout << "end processing" << std::endl;
    } else {
        std::cerr << "no source module" << std::endl;
    }
}

void Loader::initialize()
{
    for (const auto& m : m_modules) {
        m.second->initialize();
    }
}

void Loader::flush()
{
    std::cerr << "flushing pipeline" << std::endl;
    for (unsigned int i=0; i < m_modules.size(); i++) {
        for (const auto& m : m_modules) {
            //std::cerr << "\t" << m.first << "...";
            m.second->flush();
            std::cerr << ".";
            //std::cerr << "ok" << std::endl;
        }
    }
    std::cerr << "done" << std::endl;
	for (const auto& m : m_modules) {
		m.second->finalize();
	}
}

void Loader::configure(int argc, char* argv[])
{
    /* Parse command line */
    boost::program_options::options_description desc("Pipeline Options");
    desc.add_options()
    ("pipeline", boost::program_options::value<std::string>(), "pipeline description file")
    ("json", boost::program_options::value<std::string>(), "pipeline description file (json)")
    ("help", "show module help")
    ("config", "show editor config")
    ("html", "show editor documentation (HTML)")
    ("plugindir", boost::program_options::value<std::string>(), "plugin directory");
    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    try {
        boost::program_options::notify(vm);
    } catch (const std::exception& e) {
        std::cerr << desc << std::endl << e.what() << std::endl;
        exit(-1);
    }

    if (vm.count("plugindir") == 0) {
        if (boost::filesystem::is_directory(boost::filesystem::status("Modules"))) {
            enumerate_libraries("Modules");
        } else {
            std::cerr << "Invalid plugin directory" << std::endl;
        }
    } else {
        if (boost::filesystem::is_directory(boost::filesystem::status(vm["plugindir"].as<std::string>()))) {
            enumerate_libraries(vm["plugindir"].as<std::string>());
        } else {
            std::cerr << "Invalid plugindir" << std::endl;
        }
    }

    if ((vm.count("json") == 1) && (vm.count("pipeline") == 0)) {
        parse_json_config(vm, argc, argv);
    } else if ((vm.count("json") == 0) && (vm.count("pipeline") == 1)) {
        parse_cli_config(vm, argc, argv);
    } else if (vm.count("help") != 0) {
        for (const auto& it : m_factories) {
            it.second->create(it.first);
            std::cout << "================== " << it.first << " ==================" << std::endl;
            PomaModuleType::display_help_all();
            std::cout << std::endl;
        }
        exit(0);
    } else if (vm.count("config") != 0) {
        std::cout << "[" << std::endl;
        unsigned int idx{0};
        for (auto& factory : m_factories) {
            idx++;
            boost::program_options::options_description desc;
            std::cout << "  {" << std::endl;
            std::cout << "   \"type\" : \"" << factory.second->name() << "\"," << std::endl;
            std::cout << "   \"description\" : \"" << factory.second->description() << "\"," << std::endl;
            std::cout << "   \"cparams\" : \"" << factory.second->construction_parameters() << "\"," << std::endl;
            std::cout << "   \"canBeSource\" : " << std::boolalpha <<factory.second->can_be_source() << ", "<< std::endl;
            std::cout << "   \"imageURL\" : \"" << factory.second->name() << "\"," << std::endl;
            std::shared_ptr<PomaModuleType> mod{get_instance(factory.second->name(), factory.second->name())};
            mod->setup_cli(desc);
            std::vector<std::string> mandatory_parameters;
            std::vector<std::string> optional_parameters;
            for (auto& od : desc.options()) {
                std::stringstream data;
                boost::any def_value;
                bool has_default{od->semantic()->apply_default(def_value)};
                data << "{ \"name\": \"" << od->long_name() << "\", ";
                data << "\"description\": \"" << od->description() << "\", ";
                do {
                    try {
                        auto value{boost::any_cast<const char*>(def_value)};
                        if (has_default) {
                            data << "\"default\": \"" << value << "\", ";
                        }
                        data << "\"type\": \"String\"";
                        break;
                    } catch (const boost::bad_any_cast &) {
                    }
                    try {
                        auto value{boost::any_cast<double>(def_value)};
                        if (has_default) {
                            data << "\"default\": " << value << ", ";
                        }
                        data << "\"type\": \"Double\"";
                        break;
                    } catch (const boost::bad_any_cast &) {
                    }
                    try {
                        auto value{boost::any_cast<long>(def_value)};
                        if (has_default) {
                            data << "\"default\": " << value << ", ";
                        }
                        data << "\"type\": \"Long\"";
                        break;
                    } catch (const boost::bad_any_cast &) {
                    }
                    try {
                        auto value{boost::any_cast<int>(def_value)};
                        if (has_default) {
                            data << "\"default\": " << value << ", ";
                        }
                        data << "\"type\": \"Integer\"";
                        break;
                    } catch (const boost::bad_any_cast &) {
                    }
                    try {
                        bool value{boost::any_cast<bool>(def_value)};
                        if (has_default) {
                            data << "\"default\": " << std::boolalpha << value << ", ";
                        }
                        data << "\"type\": \"Boolean\"";
                        break;
                    } catch (const boost::bad_any_cast &) {
                    }
                    try {
                        auto value{boost::any_cast<std::string>(def_value)};
                        if (has_default) {
                            data << "\"default\": \"" << value << "\", ";
                        }
                        data << "\"type\": \"String\" ";
                        break;
                    } catch (const boost::bad_any_cast &) {
                    }
                    data << "\"type\": \"String\" ";
                } while(false);
                data << "}";

                if (od->semantic()->is_required()) {
                    mandatory_parameters.push_back(data.str());
                } else {
                    optional_parameters.push_back(data.str());
                }
            }
            std::cout << "   \"mandatoryParams\" : [" << std::endl;
            for (unsigned int i{0}; i<mandatory_parameters.size(); i++) {
                std::cout << "        " << mandatory_parameters[i];
                if (i < mandatory_parameters.size()-1) {
                    std::cout << ",";
                }
                std::cout << std::endl;
            }
            std::cout << "   ]," << std::endl;
            std::cout << "   \"optParams\" : [" << std::endl;
            for (unsigned int i{0}; i<optional_parameters.size(); i++) {
                std::cout << "        " << optional_parameters[i];
                if (i < optional_parameters.size()-1) {
                    std::cout << ",";
                }
                std::cout << std::endl;
            }
            std::cout << "   ]" << std::endl;
            if (idx < m_factories.size()) {
                std::cout << "  }," << std::endl;
            } else {
                std::cout << "  }" << std::endl;
            }
        }
        std::cout << "]" << std::endl;
        exit(0);
    } else if (vm.count("html") != 0) {
        std::cout << "<html><head><title>Poma Doc</title>" << std::endl;
        std::cout << "<style>" << std::endl;
        std::cout << R"MYRAW(
			table {
				border-collapse: collapse;
				border: 1px solid black;
				width: 98%;
				margin-left: auto;
				margin-right: auto;
			}
			
			body {
				background-color: #ffffff;
			}

			th, td {
				text-align: left;
				padding: 8px;
			}

			tr:nth-child(even){background-color: #f2f2f2}

			th {
				background-color: #336699;
				color: white;
			}
			
			th.modtitle {
				background-color: #696969;
				color: white;
			}
			
			th.paraminfo {
				background-color: #708090;
				color: white;
			}
)MYRAW";
		std::cout << std::endl;
        std::cout << "</style></head><body><div style=\"overflow-x:auto;\">" << std::endl;
        int idx{0};

        for (auto& factory : m_factories) {
            idx++;
            boost::program_options::options_description desc;
            std::cout << "  <table>" << std::endl;
            std::cout << "   <tr><th class=\"modtitle\" colspan=\"4\"><b>" << factory.second->name() << "</b></th></tr>" << std::endl;
            std::cout << "   <tr><td colspan=\"4\">" << factory.second->description() << "</td></tr>" << std::endl;
            if (factory.second->can_be_source()) {
				std::cout << "   <tr><td colspan=\"4\"><i>This module can be used as a pipeline source</i></td></tr>"<< std::endl;
			}
			if (factory.second->construction_parameters() != "") {
				std::cout << "   <tr><th class=\"paraminfo\" colspan=\"4\"><b>Construction parameters (cparams)</b></th></tr>" << std::endl;
				std::cout << "   <tr><td colspan=\"4\">" << factory.second->construction_parameters() << "</td></tr>"<< std::endl;
			}
            std::shared_ptr<PomaModuleType> mod{get_instance(factory.second->name(), factory.second->name())};
            mod->setup_cli(desc);
            std::string dyn_properties = mod->on_enumerate_properties();
            if (dyn_properties != "") {
				boost::replace_all(dyn_properties, ",", ", ");
				std::cout << "   <tr><th class=\"paraminfo\" colspan=\"4\"><b>Dynamic properties</b></th></tr>" << std::endl;
				std::cout << "   <tr><td colspan=\"4\">" << dyn_properties << "</td></tr>"<< std::endl;
			}
            std::vector<std::string> mandatory_parameters;
            std::vector<std::string> optional_parameters;
            for (auto& od : desc.options()) {
                std::stringstream data;
                boost::any def_value;
                bool has_default{od->semantic()->apply_default(def_value)};
                data << "<tr><td>" << od->long_name() << "</td>";
                do {
                    try {
                        auto value{boost::any_cast<const char*>(def_value)};
                        data << "<td>String</td>";
                        if (has_default) {
							data << "<td>" << od->description() << "</td>";
                            data << "<td>" << value << "</td>";
                        } else {
							data << "<td colspan=\"2\">" << od->description() << "</td>";
						}
                        break;
                    } catch (const boost::bad_any_cast &) {
                    }
                    try {
                        auto value{boost::any_cast<double>(def_value)};
                        data << "<td>Double</td>";
                        if (has_default) {
							data << "<td>" << od->description() << "</td>";
                            data << "<td>" << value << "</td>";
                        } else {
							data << "<td colspan=\"2\">" << od->description() << "</td>";
						}
                        break;
                    } catch (const boost::bad_any_cast &) {
                    }
                    try {
                        auto value{boost::any_cast<long>(def_value)};
                        data << "<td>Long</td>";
                        if (has_default) {
							data << "<td>" << od->description() << "</td>";
                            data << "<td>" << value << "</td>";
                        } else {
							data << "<td colspan=\"2\">" << od->description() << "</td>";
						}
                        break;
                    } catch (const boost::bad_any_cast &) {
                    }
                    try {
                        auto value{boost::any_cast<int>(def_value)};
                        data << "<td>Integer</td>";
                        if (has_default) {
							data << "<td>" << od->description() << "</td>";
                            data << "<td>" << value << "</td>";
                        } else {
							data << "<td colspan=\"2\">" << od->description() << "</td>";
						}
                        break;
                    } catch (const boost::bad_any_cast &) {
                    }
                    try {
                        auto value{boost::any_cast<bool>(def_value)};
                        data << "<td>Boolean</td>";
                        if (has_default) {
							data << "<td>" << od->description() << "</td>";
                            data << "<td>" << std::boolalpha << value << "</td>";
                        } else {
							data << "<td colspan=\"2\">" << od->description() << "</td>";
						}
                        break;
                    } catch (const boost::bad_any_cast &) {
                    }
                    try {
                        auto value{boost::any_cast<std::string>(def_value)};
                        data << "<td>String</td>";
                        if (has_default) {
							data << "<td>" << od->description() << "</td>";
                            data << "<td>" << value << "</td>";
                        } else {
							data << "<td colspan=\"2\">" << od->description() << "</td>";
						}
                        break;
                    } catch (const boost::bad_any_cast &) {
                    }
                    data << "<td>String</td>";
					data << "<td colspan=\"2\">" << od->description() << "</td>";
                } while(false);
                data << "</tr>";

                if (od->semantic()->is_required()) {
                    mandatory_parameters.push_back(data.str());
                } else {
                    optional_parameters.push_back(data.str());
                }
            }
            if (mandatory_parameters.size()) {
				std::cout << "   <tr><th class=\"paraminfo\" colspan=\"4\"><b>Mandatory parameters</b></th></tr>" << std::endl;
				std::cout << "   <tr><th>Name</th><th>Type</th><th colspan=\"2\">Description</th></tr>" << std::endl;
				for (unsigned int i{0}; i<mandatory_parameters.size(); i++) {
					std::cout << mandatory_parameters[i] << std::endl;
				}
				std::cout << "   </tr>" << std::endl;
			}
			if (optional_parameters.size()) {
				std::cout << "   <tr><th class=\"paraminfo\" colspan=\"4\"><b>Optional parameters</b></th></tr>" << std::endl;
				std::cout << "   <tr><th>Name</th><th>Type</th><th>Description</th><th>Default value</th></tr>" << std::endl;
				for (unsigned int i{0}; i<optional_parameters.size(); i++) {
					std::cout << optional_parameters[i] << std::endl;
				}
				std::cout << "   </tr>" << std::endl;
			}
			std::cout << "</table><br/><br/>" << std::endl;
        }
        std::cout << "</div></body></html>" << std::endl;
        exit(0);
    } else {
		std::cerr << desc << std::endl;
        die("invalid command line arguments");
    }
}

PomaModuleType* Loader::get_source()
{
    return m_source;
}

std::shared_ptr<PomaModuleType> Loader::get_module(const std::string& identifier)
{
    return m_modules[identifier];
}

}
