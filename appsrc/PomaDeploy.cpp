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

#include <boost/program_options.hpp>
#include "DistributedConfigGenerator.h"
#include "ParallelConfigGenerator.h"
#include "ParallelOptimizerGenerator.h"
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <fstream>
#include <string>
#include <cstdlib>
#include "zhelpers.hpp"
#include "Common.h"
#include <algorithm>
#include <ParallelOptimizerGenerator.h>

std::string get_address(const std::string& host, unsigned int port=5232);
std::string get_reply(const std::string& address, const boost::property_tree::ptree& request);
bool request(const std::string& address, const boost::property_tree::ptree& request, const std::string& successvalue = "/ success /");
bool create_job(const std::string& host, unsigned int port, const std::string& jobid, const std::string& json);
bool check_modules(const std::string& host, unsigned int port, std::set<std::string> modules);
bool is_running(const std::string& host, unsigned int port, const std::string& jobid);
bool start_job(const std::string& host, unsigned int port, const std::string& jobid);
bool kill_job(const std::string& host, unsigned int port, const std::string& jobid);
bool clear_job(const std::string& host, unsigned int port, const std::string& jobid);
std::vector<std::string> list_jobs(const std::string& host, unsigned int port);

std::string get_address(const std::string& host, unsigned int port)
{
    std::stringstream ss;
    ss << "tcp://" << host << ":" << port;
    return ss.str();
}

std::string get_reply(const std::string& address, const boost::property_tree::ptree& request)
{
    try {
        std::stringstream ss;
        boost::property_tree::write_json(ss, request);
        zmq::context_t context {1};
        zmq::socket_t socket{context, ZMQ_REQ};
        int timeout = 1000;
        socket.setsockopt(ZMQ_SNDTIMEO, &timeout, sizeof(int));
        socket.setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(int));
        socket.setsockopt(ZMQ_LINGER, &timeout, sizeof(int));
        socket.connect(address.c_str());
        s_send(socket, ss.str());
        std::stringstream iss{s_recv(socket)};
        boost::property_tree::ptree rep;
        boost::property_tree::json_parser::read_json(iss, rep);
        std::string reply{rep.get("reply","")};
        return reply;
    } catch(...) {
        std::cerr << "Failed to communicate with " << address << std::endl;
        return "";
    }
}

bool request(const std::string& address, const boost::property_tree::ptree& request, const std::string& successvalue)
{
    return get_reply(address, request) == successvalue;
}

bool create_job(const std::string& host, unsigned int port, const std::string& jobid, const std::string& json)
{
    try {
        auto address = get_address(host, port);
        boost::property_tree::ptree req;
        req.put("command", "create");
        req.put("jobid", jobid);
        req.put("json", json);
        return request(address, req);
    } catch (...) {
        std::cerr << "Exception in create job " << std::endl;
        return false;
    }
}

bool check_modules(const std::string& host, unsigned int port, std::set<std::string> modules)
{
    try {
        auto address = get_address(host, port);
        boost::property_tree::ptree request;
        request.put("command", "modules");
        std::vector<std::string> remote_modules;
        std::string reply{get_reply(address, request)};
        boost::split(remote_modules, reply, boost::is_any_of(","));
        for (const auto& rmod : modules) {
            if(std::find(remote_modules.begin(), remote_modules.end(), rmod) == remote_modules.end()) {
                std::cerr << "Module " << rmod << " is not available on " << host << std::endl;
                return false;
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool is_running(const std::string& host, unsigned int port, const std::string& jobid)
{
    auto address = get_address(host, port);
    boost::property_tree::ptree req;
    req.put("command", "isrunning");
    req.put("jobid", jobid);
    return request(address, req, "true");
}

bool start_job(const std::string& host, unsigned int port, const std::string& jobid)
{
    auto address = get_address(host, port);
    boost::property_tree::ptree req;
    req.put("command", "start");
    req.put("jobid", jobid);
    return request(address, req);
}

bool kill_job(const std::string& host, unsigned int port, const std::string& jobid)
{
    auto address = get_address(host, port);
    boost::property_tree::ptree req;
    req.put("command", "kill");
    req.put("jobid", jobid);
    return request(address, req);
}

bool clear_job(const std::string& host, unsigned int port, const std::string& jobid)
{
    auto address = get_address(host, port);
    boost::property_tree::ptree req;
    req.put("command", "clear");
    req.put("jobid", jobid);
    return request(address, req);
}

std::vector<std::string> list_jobs(const std::string& host, unsigned int port)
{
    auto address = get_address(host, port);
    boost::property_tree::ptree request;
    request.put("command", "list");
    std::vector<std::string> remote_jobs;
    std::string reply{get_reply(address, request)};
    if (reply != "") {
        boost::split(remote_jobs, reply, boost::is_any_of(","));
    }
    return remote_jobs;
}

int main(int argc, char* argv[])
{
    boost::program_options::options_description desc("Poma Deploy Options");
    desc.add_options()
    ("cmd", boost::program_options::value<std::string>()->required(), "command to execute")
    ("host", boost::program_options::value<std::string>()->default_value("localhost"), "remote host")
    ("json", boost::program_options::value<std::string>(), "pipeline description file (json)")
    ("outputjson", boost::program_options::value<std::string>(), "pipeline output description file (json)")
    ("jobid", boost::program_options::value<std::string>(), "job unique identifier")
    ("port", boost::program_options::value<unsigned int>()->default_value(5232), "Service TCP port")
    ("baseport", boost::program_options::value<unsigned int>()->default_value(6000), "Base ZeroMQ sink port");

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);

    try {
        boost::program_options::notify(vm);
    } catch (std::exception &e) {
        std::cerr << desc << std::endl << e.what() << std::endl;
        std::exit(-1);
    }

    // Extract command line parameters
    std::string command{vm["cmd"].as<std::string>()};
    std::string host{vm["host"].as<std::string>()};
    unsigned int port = vm["port"].as<unsigned int>();
    unsigned int baseport = vm["baseport"].as<unsigned int>();
    std::string jobid;
    if (vm.count("jobid")) {
        jobid = vm["jobid"].as<std::string>();
    }
    
    if (command == "gen") {
         if (vm.count("json") == 0) {
            std::cerr << "deploy command requires json parameter" << std::endl;
            std::exit(-1);
        }
        if (vm.count("jobid") == 0) {
            boost::uuids::random_generator uuid_generator;
            boost::uuids::uuid juuid = uuid_generator();
            jobid = boost::uuids::to_string(juuid);
        }
        // Process pipeline
        std::string source_mid;
        std::map<std::string,poma::Module> modules;
        std::vector<poma::Link> links;
        std::ifstream ifs;
        ifs.open(vm["json"].as<std::string>());
        poma::load(ifs, source_mid, modules, links);
        poma::ParallelConfigGenerator pcg{source_mid, modules, links};
        pcg.process();
        poma::ParallelOptimizerGenerator pog{source_mid, modules, links};
        pog.process();
        std::stringstream oss;
        std::ofstream ofs;
        
        std::string genfile;
        if (vm.count("outputjson") == 0) {
            genfile = "output_" + jobid + "_" + vm["json"].as<std::string>() + "_" + host + ".json";
        } else {
            genfile = vm["outputjson"].as<std::string>();
        }
        ofs.open(genfile);      
        poma::generate(source_mid, modules, links, ofs);
        ofs.flush();
        ofs.close();
        std::cout << "Done generating local config:" << genfile << std::endl;
        std::exit(0);       
    } else if (command == "deploy" || command == "run") {
        if (vm.count("json") == 0) {
            std::cerr << "deploy command requires json parameter" << std::endl;
            std::exit(-1);
        }
        if (vm.count("jobid") == 0) {
            boost::uuids::random_generator uuid_generator;
            boost::uuids::uuid juuid = uuid_generator();
            jobid = boost::uuids::to_string(juuid);
        }
        // Process pipeline
        std::string source_mid;
        std::map<std::string,poma::Module> modules;
        std::vector<poma::Link> links;
        std::ifstream ifs;
        ifs.open(vm["json"].as<std::string>());
        poma::load(ifs, source_mid, modules, links);
        poma::ParallelConfigGenerator pcg{source_mid, modules, links};
        pcg.process();
        poma::DistributedConfigGenerator hcg{source_mid, modules, links, baseport};
        hcg.process();
        // Check remote modules
        std::cout << "Verifying modules" << std::endl;
        for (const auto& host : hcg.hosts()) {
            std::cout << "\t" << host << ":" << port << "...";
            if (!check_modules(host, port, hcg.modules(host))) {
                std::exit(-1);
            }
            std::cout << "OK" << std::endl;
        }
        std::vector<std::string> deployed_hosts;
        std::cout << "Deploying pipeline jobs" << std::endl;
        for (const auto& host : hcg.hosts()) {
            std::cout << "\t" << host << ":" << port << "...";
            std::string host_source_mid;
            std::map<std::string,poma::Module> host_modules;
            std::vector<poma::Link> host_links;
            hcg.get_config(host, host_source_mid, host_modules, host_links);
            poma::ParallelOptimizerGenerator pog{host_source_mid, host_modules, host_links};
            pog.process();
            std::stringstream oss;
            poma::generate(host_source_mid, host_modules, host_links, oss);
            if (create_job(host, port, jobid, oss.str())) {
                deployed_hosts.push_back(host);
                std::cout << "OK" << std::endl;
            } else {
                std::cout << "FAILED" << std::endl;
                break;
            }
        }
        // Start job
        if (deployed_hosts.size() == hcg.hosts().size()) {
            std::cout << "Starting pipeline jobs" << std::endl;
            for (const auto& host : hcg.hosts()) {
                if(start_job(host, port, jobid)) {
                    std::cout << host << "\t" << port << "\t" << jobid << "\tRUNNING" << std::endl;
                } else {
                    std::cout << host << "\t" << port << "\t" << jobid << "\tFAILED" << std::endl;
                }
            }
        } else {
            std::cout << "Pipeline job status" << std::endl;
            for (const auto& host : hcg.hosts()) {
                if (std::find(deployed_hosts.begin(), deployed_hosts.end(), host) != deployed_hosts.end()) {
                    std::cout << host << "\t" << port << "\t" << jobid << "\tSTALE" << std::endl;
                } else {
                    std::cout << host << "\t" << port << "\t" << jobid << "\tFAILED" << std::endl;
                }
            }
        }

        if (command == "run") {
            std::cout << "[Press enter to stop execution...]";
            std::cin.get();
            std::cout << "Killing pipeline jobs" << std::endl;
            for (const auto& host : hcg.hosts()) {
                if(kill_job(host, port, jobid)) {
                    std::cout << host << "\t" << port << "\t" << jobid << "\tSTOPPED" << std::endl;
                } else {
                    std::cout << host << "\t" << port << "\t" << jobid << "\tUNKNOWN" << std::endl;
                }
            }
            std::cout << "Clearing pipeline jobs" << std::endl;
            for (const auto& host : hcg.hosts()) {
                if(clear_job(host, port, jobid)) {
                    std::cout << host << "\t" << port << "\t" << jobid << "\tCLEARED" << std::endl;
                } else {
                    std::cout << host << "\t" << port << "\t" << jobid << "\tUNKNOWN" << std::endl;
                }
            }
        }
    } else if (command == "killall") {
        if (vm.count("json") == 0) {
            std::cerr << "killall command requires json parameter" << std::endl;
            std::exit(-1);
        }
        if (vm.count("jobid") == 0) {
            std::cerr << "killall command requires jobid parameter" << std::endl;
            std::exit(-1);
        }
        // Determine which hosts need to be informed
        std::ifstream ifs;
        ifs.open(vm["json"].as<std::string>());
        for (const auto& host : poma::extract_hosts(ifs)) {
            if(kill_job(host, port, jobid)) {
                std::cout << host << "\t" << port << "\t" << jobid << "\tSTOPPED" << std::endl;
            } else {
                std::cout << host << "\t" << port << "\t" << jobid << "\tUNKNOWN" << std::endl;
            }
        }
    } else if (command == "clear") {
        if (vm.count("json") == 0) {
            std::cerr << "clear command requires json parameter" << std::endl;
            std::exit(-1);
        }
        if (vm.count("jobid") == 0) {
            std::cerr << "clear command requires jobid parameter" << std::endl;
            std::exit(-1);
        }
        // Process pipeline
        std::ifstream ifs;
        ifs.open(vm["json"].as<std::string>());
        for (const auto& host : poma::extract_hosts(ifs)) {
            if(clear_job(host, port, jobid)) {
                std::cout << host << "\t" << port << "\t" << jobid << "\tCLEARED" << std::endl;
            } else {
                std::cout << host << "\t" << port << "\t" << jobid << "\tUNKNOWN" << std::endl;
            }
        }
    } else if (command == "list") {
        for (const auto& jid : list_jobs(host, port)) {
            if (is_running(host,port,jid)) {
                std::cout << host << "\t" << port << "\t" << jid << "\tRUNNING" << std::endl;
            } else {
                std::cout << host << "\t" << port << "\t" << jid << "\tSTALE" << std::endl;
            }
        }
    } else if (command == "kill") {
        if (vm.count("jobid") == 0) {
            std::cerr << "kill command requires jobid parameter" << std::endl;
            std::exit(-1);
        }
        if(kill_job(host, port, jobid)) {
            if (is_running(host,port,jobid)) {
                std::cout << host << "\t" << port << "\t" << jobid << "\tRUNNING" << std::endl;
            } else {
                std::cout << host << "\t" << port << "\t" << jobid << "\tSTALE" << std::endl;
            }
        } else {
            std::cout << host << "\t" << port << "\t" << jobid << "\tUNKNOWN" << std::endl;
        }
    } else if (command == "clear") {
        if (vm.count("jobid") == 0) {
            std::cerr << "clear command requires jobid parameter" << std::endl;
            std::exit(-1);
        }
        if(clear_job(host, port, jobid)) {
            std::cout << host << "\t" << port << "\t" << jobid << "\tCLEARED" << std::endl;
        } else {
            std::cout << host << "\t" << port << "\t" << jobid << "\tUNKNOWN" << std::endl;
        }
    } else if (command == "isrunning") {
        if (vm.count("jobid") == 0) {
            std::cerr << "isrunning command requires jobid parameter" << std::endl;
            std::exit(-1);
        }
        if(is_running(host, port, jobid)) {
            std::cout << host << "\t" << port << "\t" << jobid << "\tRUNNING" << std::endl;
        } else {
            std::cout << host << "\t" << port << "\t" << jobid << "\tSTALE" << std::endl;
        }
    } else if (command == "modules") {
        boost::property_tree::ptree request;
        request.put("command", "modules");
        std::vector<std::string> remote_modules;
        std::string reply{get_reply(get_address(host, port), request)};
        if (reply != "") {
            std::cout << reply << std::endl;
        }
    } else if (command == "dumpconfig") {
        if (vm.count("json") == 0) {
            std::cerr << "dumpconfig command requires json parameter" << std::endl;
            std::exit(-1);
        }
        if (vm.count("jobid") == 0) {
            boost::uuids::random_generator uuid_generator;
            boost::uuids::uuid juuid = uuid_generator();
            jobid = boost::uuids::to_string(juuid);
        }
        // Process pipeline
        std::string source_mid;
        std::map<std::string,poma::Module> modules;
        std::vector<poma::Link> links;
        std::ifstream ifs;
        ifs.open(vm["json"].as<std::string>());
        poma::load(ifs, source_mid, modules, links);
        poma::ParallelConfigGenerator pcg{source_mid, modules, links};
        pcg.process();
        poma::DistributedConfigGenerator hcg{source_mid, modules, links, baseport};
        hcg.process();
        for (const auto& host : hcg.hosts()) {
            std::ofstream ofs;
            ofs.open(jobid + "_" + vm["json"].as<std::string>() + "_" + host + ".json", std::ofstream::out);
            if (ofs.is_open()) {
                std::string host_source_mid;
                std::map<std::string,poma::Module> host_modules;
                std::vector<poma::Link> host_links;
                hcg.get_config(host, host_source_mid, host_modules, host_links);
                poma::ParallelOptimizerGenerator pog{host_source_mid, host_modules, host_links};
                pog.process();
                std::stringstream oss;
                poma::generate(host_source_mid, host_modules, host_links, oss);
                ofs << oss.str();
                ofs.flush();
                ofs.close();
            } else {
                std::cerr << "failed to open file for writing" << std::endl;
                std::exit(-1);
            }
        }
    } else {
        std::cerr << "Invalid command " << command << std::endl;
    }
}
