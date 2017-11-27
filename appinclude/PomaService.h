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

#ifndef POMASERVICE_H
#define POMASERVICE_H

#include <boost/process.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <boost/filesystem.hpp>

namespace poma {

class PomaJob {
public:
    PomaJob(const boost::filesystem::path& loader,
            const boost::filesystem::path& modules,
            const boost::filesystem::path& output_path,
            const std::string& job_id,
            const std::string& json_data);
	PomaJob(const boost::filesystem::path& loader,
                 const boost::filesystem::path& modules,
                 const boost::filesystem::path& output_path,
                 const std::string& job_id);            
    virtual ~PomaJob();
    pid_t pid() const;
    bool kill();
    bool is_running() const;
    bool clear() const;
    bool start();
private:
    std::string m_loader_path;
    std::string m_modules_path;
    std::string m_job_id;
    std::string m_json_file;
    std::string m_stdout_file;
    std::string m_stderr_file;
    boost::process::child* m_job{nullptr};
};

class PomaService {
public:
    PomaService(const boost::filesystem::path& loader_path,
                const boost::filesystem::path& modules_path,
                const boost::filesystem::path& output_path, unsigned int port = 5232);
    void start_serving();
    std::string process_request(const std::string& req);

private:
    std::string build_reply(const std::string& msg);
    boost::filesystem::path m_loader_path;
    boost::filesystem::path m_modules_path;
    boost::filesystem::path m_output_path;
    unsigned int m_port{5232};
    std::unordered_map<std::string,PomaJob*> m_jobs;
};

}

#endif
