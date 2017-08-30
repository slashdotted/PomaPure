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

#include "PomaService.h"
#include "PomaDefault.h"
#include <fstream>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/dll/import.hpp>
#include "zhelpers.hpp"
#include "base64.h"

namespace poma {

PomaJob::PomaJob(const boost::filesystem::path& loader,
                 const boost::filesystem::path& modules,
                 const boost::filesystem::path& output_path,
                 const std::string& job_id,
                 const std::string& json_data)
    : m_loader_path{loader.string()}, m_modules_path{modules.string()}, m_job_id{job_id}
{
    m_json_file = (output_path / (job_id + ".json")).string();
    m_stdout_file = (output_path / (job_id + ".out")).string();
    m_stderr_file = (output_path / (job_id + ".err")).string();

    std::ofstream jsonfile;
    jsonfile.open(m_json_file);
    jsonfile  << json_data;
    jsonfile.flush();
    jsonfile.close();

    std::cerr << "New job =>" << std::endl
              << "\tid=" << job_id << std::endl
              << "\tjson=" << m_json_file << std::endl
              << "\tstdout=" << m_stdout_file << std::endl
              << "\tstderr=" << m_stderr_file << std::endl
              << "\tloader=" << m_loader_path << std::endl
              << "\tmodules=" << m_modules_path << std::endl;
}

PomaJob::~PomaJob()
{
    kill();
    clear();
}

pid_t PomaJob::pid() const
{
    if (m_job != nullptr && m_job->running()) {
        return m_job->id();
    } else {
        return 0;
    }
}

bool PomaJob::kill()
{
    if (m_job != nullptr && m_job->running()) {
        m_job->terminate();
        delete m_job;
        m_job = nullptr;
        return true;
    } else {
        return false;
    }

}

bool PomaJob::is_running() const
{
    if (m_job != nullptr && m_job->running()) {
        return true;
    } else {
        return false;
    }
}

bool PomaJob::clear() const
{
    if (m_job == nullptr) {
        boost::filesystem::remove(m_stdout_file);
        boost::filesystem::remove(m_stderr_file);
        return true;
    } else {
        return false;
    }
}

bool PomaJob::start()
{
    auto env = boost::this_process::environment();
    m_job = new boost::process::child(m_loader_path, "--json", m_json_file, "--plugindir", m_modules_path, env,
                                      boost::process::std_out > m_stdout_file, boost::process::std_err > m_stderr_file);
    return is_running();
}

PomaService::PomaService(const boost::filesystem::path& loader_path,
                         const boost::filesystem::path& modules_path,
                         const boost::filesystem::path& output_path, unsigned int port)
    : m_loader_path{loader_path}, m_modules_path{modules_path},
      m_output_path{output_path}, m_port{port}
{

}

void PomaService::start_serving()
{
    zmq::context_t context {1};
    zmq::socket_t socket{context, ZMQ_REP};
    std::stringstream address;
    address << "tcp://*:" << m_port;
    std::cerr << "Service listening on " << address.str() << "..." << std::endl;
    socket.bind(address.str().c_str());
    for (;;) {
        s_send(socket, process_request(s_recv(socket)));
    }
}

std::string PomaService::build_reply(const std::string& msg)
{
    return std::string{"{ \"reply\" : \""} + msg + "\" }";
}

std::string PomaService::process_request(const std::string& req)
{
    try {
        std::stringstream iss{req};
        boost::property_tree::ptree message;
        boost::property_tree::json_parser::read_json(iss, message);

        std::string command{message.get("command", "")};
        std::string jobid{message.get("jobid", "")};
        std::string json{message.get("json", "")};
        if (command == "create") {
            if (jobid != "" && json != "" && m_jobs.count(jobid) == 0) {
                PomaJob* job{new PomaJob{
                        m_loader_path,
                        m_modules_path,
                        m_output_path,
                        jobid,
                        json}};
                m_jobs[jobid] = job;
                return build_reply("/ success /");
            }
        } else if (command == "start") {
            if (jobid != "" && m_jobs.count(jobid) == 1) {
                PomaJob* job{m_jobs[jobid]};
                if(job->start()) {
                    return build_reply("/ success /");
                }
            }
        } else if (command == "kill") {
            if (jobid != "" && m_jobs.count(jobid) == 1) {
                PomaJob* job{m_jobs[jobid]};
                if(job->kill()) {
                    return build_reply("/ success /");
                }
            }
        } else if (command == "isrunning") {
            if (jobid != "" && m_jobs.count(jobid) == 1) {
                PomaJob* job{m_jobs[jobid]};
                if(job->is_running()) {
                    return build_reply("true");
                } else {
                    return build_reply("false");
                }
            }
        } else if (command == "list") {
            std::stringstream ss;
            for(auto const& mid: m_jobs) {
                ss << mid.first << ",";
            }
            return build_reply(ss.str().substr(0,ss.str().size()-1));
        } else if (command == "modules") {
            std::stringstream ss;
            boost::filesystem::recursive_directory_iterator it {m_modules_path};
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
                if (file_ext == ".so") {
                    auto plugin_module = boost::dll::import<ModuleFactory<PomaModuleType>>(
                                             it->path().c_str(),
                                             "factory",
                                             boost::dll::load_mode::append_decorations
                                         );
                    ss << plugin_module->name() << ",";
                }
                ++it;
            }
            return build_reply(ss.str().substr(0,ss.str().size()-1));
        } else if (command == "clear") {
            if (jobid != "" && m_jobs.count(jobid) == 1) {
                PomaJob* job{m_jobs[jobid]};
                if(job->clear()) {
                    return build_reply("/ success /");
                }
            }
        } else {
            throw std::runtime_error(std::string{"Invalid command: "} + command);
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception while processing request: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception while processing request"<< std::endl;
    }
    return build_reply("/ fail /");
}

}

int main(int argc, char* argv[])
{
    /* Parse command line */
    boost::program_options::options_description desc("Poma Service Options");
    desc.add_options()
    ("loaderpath", boost::program_options::value<std::string>()->required(), "PomaLoader path")
    ("plugindir", boost::program_options::value<std::string>()->required(), "Plugin directory")
    ("check", boost::program_options::value<std::string>(), "JSON file for testing parameters")
    ("port", boost::program_options::value<unsigned int>()->default_value(5232), "Service TCP port")
    ("outputdir", boost::program_options::value<std::string>()->required(), "Output directory (json, log and error files)");
    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    try {
        boost::program_options::notify(vm);
    } catch (std::exception &e) {
        std::cerr << "Failed to start service " << std::endl << desc << std::endl << e.what() << std::endl;
        exit(-1);
    }
    try {
        boost::filesystem::path loader{vm["loaderpath"].as<std::string>()};
        boost::filesystem::path modules{vm["plugindir"].as<std::string>()};
        boost::filesystem::path outpath{vm["outputdir"].as<std::string>()};
        if (!boost::filesystem::exists(loader)) {
            throw std::runtime_error("Invalid PomaLoader path");
        }
        if (!boost::filesystem::exists(modules)) {
            throw std::runtime_error("Invalid plugin directory path");
        }
        if (!boost::filesystem::exists(outpath)) {
            throw std::runtime_error("Invalid output directory path");
        }
        loader = boost::filesystem::canonical(loader);
        modules = boost::filesystem::canonical(modules);
        outpath = boost::filesystem::canonical(outpath);
        if (vm.count("check")) {
            std::string testfile{vm["check"].as<std::string>()};
            std::stringstream ss;
            std::ifstream jsonfile;
            jsonfile.open(testfile);
            std::string jsondata((std::istreambuf_iterator<char>(jsonfile)),
                                 std::istreambuf_iterator<char>());
            jsonfile.close();
            std::cerr << "Testing with file " << testfile << ":" << jsondata << std::endl;
            poma::PomaJob job{loader, modules, outpath, "testid", jsondata};
            job.start();
            std::cout << "Press Enter to terminate...";
            std::cin.get();
            job.kill();
            job.clear();
        } else {
            poma::PomaService ps{loader, modules, outpath, vm["port"].as<unsigned int>()};
            ps.start_serving();
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to start service: " << e.what() << std::endl;
        exit(-1);
    } catch (const boost::exception& e) {
        std::cerr << "Failed to start service: " << boost::diagnostic_information(e) << std::endl;
        exit(-1);
    }
}

