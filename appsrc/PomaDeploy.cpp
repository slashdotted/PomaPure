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
#include "HostConfigGenerator.h"
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <fstream>
#include <string>
#include "zhelpers.hpp"
#include "base64.h"
#include <algorithm>

std::string get_reply(const std::string& address, const boost::property_tree::ptree& request)
{
    std::stringstream ss;
    boost::property_tree::write_json(ss, request);
    zmq::context_t context {1};
    zmq::socket_t socket{context, ZMQ_REQ};
    socket.connect(address.c_str());
    s_send(socket, ss.str());
    std::stringstream iss{s_recv(socket)};
    boost::property_tree::ptree rep;
    boost::property_tree::json_parser::read_json(iss, rep);
    std::string reply{rep.get("reply","")};
    return reply;
}

bool request(const std::string& address, const boost::property_tree::ptree& request, const std::string& successvalue = "/ success /")
{
    return get_reply(address, request) == successvalue;
}

bool create_job(const std::string& address, const std::string& jobid, const std::string& json)
{
    try {
        boost::property_tree::ptree req;
        req.put("command", "create");
        req.put("jobid", jobid);
        unsigned char *data = new unsigned char[json.size() + 1];
        strcpy((char *) data, json.c_str());
        std::string json64 = base64_encode(data, json.size() + 1);
        req.put("json", json64);
        return request(address, req);
    } catch (...) {
        return false;
    }
}

bool check_modules(const std::string& address, std::set<std::string> modules)
{
    try {
        boost::property_tree::ptree request;
        request.put("command", "modules");
        std::vector<std::string> remote_modules;
        std::string reply{get_reply(address, request)};
        boost::split(remote_modules, reply, boost::is_any_of(","));
        for (const auto& rmod : modules) {
            if(std::find(remote_modules.begin(), remote_modules.end(), rmod) == remote_modules.end()) {
                std::cerr << "Module " << rmod << " is not available on " << address << std::endl;
                std::cerr << "Available modules are :" << reply << std::endl;
                return false;
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool is_running(const std::string& address, const std::string& jobid)
{
    boost::property_tree::ptree req;
    req.put("command", "isrunning");
    req.put("jobid", jobid);
    return request(address, req);
}

bool start_job(const std::string& address, const std::string& jobid)
{
    boost::property_tree::ptree req;
    req.put("command", "start");
    req.put("jobid", jobid);
    return request(address, req);
}

bool kill_job(const std::string& address, const std::string& jobid)
{
    boost::property_tree::ptree req;
    req.put("command", "kill");
    req.put("jobid", jobid);
    return request(address, req);
}

bool clear_job(const std::string& address, const std::string& jobid)
{
    boost::property_tree::ptree req;
    req.put("command", "clear");
    req.put("jobid", jobid);
    return request(address, req);
}

std::vector<std::string> list_jobs(const std::string& address)
{
    boost::property_tree::ptree request;
    request.put("command", "list");
    std::vector<std::string> remote_jobs;
    std::string reply{get_reply(address, request)};
    boost::split(remote_jobs, reply, boost::is_any_of(","));
    return remote_jobs;
}

int main(int argc, char* argv[])
{
    /* Parse command line */
    boost::program_options::options_description desc("Poma Deploy Options");
    desc.add_options()
    ("command", boost::program_options::value<std::string>()->required(), "command to execute")
    ("address", boost::program_options::value<std::string>()->default_value("tcp://localhost:5232"), "remote address")
    ("json", boost::program_options::value<std::string>(), "pipeline description file (json)")
    ("jobid", boost::program_options::value<std::string>(), "job unique identifier")
    ("port", boost::program_options::value<unsigned int>()->default_value(5232), "Service TCP port")
    ("baseport", boost::program_options::value<unsigned int>()->default_value(6000), "Base ZeroMQ sink port");
    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    try {
        boost::program_options::notify(vm);
    } catch (std::exception &e) {
        std::cerr << desc << std::endl << e.what() << std::endl;
        exit(-1);
    }
    boost::uuids::random_generator uuid_generator;
    std::string jobid;
    if (vm.count("jobid") == 0) {
        boost::uuids::uuid juuid = uuid_generator();
        jobid = boost::uuids::to_string(juuid);
    } else {
        jobid = vm["jobid"].as<std::string>();
    }

    std::string command{vm["command"].as<std::string>()};
    std::string address{vm["address"].as<std::string>()};
    unsigned int port = vm["port"].as<unsigned int>();
    unsigned int baseport = vm["baseport"].as<unsigned int>();

    if (command == "deploy") {
        std::cout << "Deploying job " << jobid << std::endl;
        std::ifstream ifs;
        ifs.open(vm["json"].as<std::string>());
        poma::HostConfigGenerator hcg{ifs, baseport};
        for (const auto& host : hcg.hosts()) {
            std::cout << "Checking compatibility for " << host << std::endl;
            std::stringstream ss;
            ss << "tcp://" << host << ":" << port;
            if (check_modules(ss.str(), hcg.modules(host))) {
                std::cout << "Host " << host << " provides the required modules" << std::endl;
            } else {
                std::cerr << "Host " << host << " does not provide the required modules" << std::endl;
                exit(-1);
            }
        }
        std::vector<std::string> deployed_hosts;
        bool deploy_succeeded{true};
        for (const auto& host : hcg.hosts()) {
            std::cout << "Deploying on host " << host << " :" << hcg[host] << std::endl;
            std::stringstream ss;
            ss << "tcp://" << host << ":" << port;
            if (create_job(ss.str(), jobid, hcg[host])) {
                std::cout << "Deployment on host " << host << " succeeded!" << std::endl;
                deployed_hosts.push_back(ss.str());
            } else {
                std::cerr << "Deployment on host " << host << " failed!" << std::endl;
                deploy_succeeded = false;
                break;
            }
        }
        // Start job
        if (deploy_succeeded) {
            for (const auto& host : deployed_hosts) {
                if(start_job(host, jobid)) {
                    std::cout << "Successfully started job on " << host << std::endl;
                } else {
                    std::cerr << "Failed to start job on " << host << std::endl;
                }
            }
        }
        std::cout << "Press Enter to end execution...";
        std::cin.get();
        // Clear job
        for (const auto& host : deployed_hosts) {
            if(kill_job(host, jobid)) {
                std::cout << "Successfully killed job on " << host << std::endl;
            } else {
                std::cerr << "Failed to kill job on " << host << std::endl;
            }
        }
        for (const auto& host : deployed_hosts) {
            if(clear_job(host, jobid)) {
                std::cout << "Successfully cleared job on " << host << std::endl;
            } else {
                std::cerr << "Failed to clear job on " << host << std::endl;
            }
        }
    } else if (command == "list") {
        std::cout << "Jobs on " << address << ":" << std::endl;
        for (const auto& jid : list_jobs(address)) {
            std::cout << jid << std::endl;
        }
    } else if (command == "kill") {
        if(kill_job(address, jobid)) {
            std::cout << "Successfully killed job on " << address << std::endl;
        } else {
            std::cerr << "Failed to kill job on " << address << std::endl;
        }
    } else if (command == "clear") {
        if(clear_job(address, jobid)) {
            std::cout << "Successfully cleared job on " << address << std::endl;
        } else {
            std::cerr << "Failed to clear job on " << address << std::endl;
        }
    } else if (command == "isrunning") {
        if(is_running(address, jobid)) {
            std::cout << "Job " << jobid << " is running on " << address << std::endl;
        } else {
            std::cout << "Job " << jobid << " is not running on " << address << std::endl;
        }
    } else {
        std::cerr << "Invalid command " << command << std::endl;
    }
}
