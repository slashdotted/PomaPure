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

#ifndef BUFFER_H
#define BUFFER_H

#include <thread>
#include <vector>
#include <algorithm>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "PomaDefault.h"

class Buffer : public poma::Module<Buffer,PomaDataType> {
public:
    Buffer(const std::string& mid);
    ~Buffer();
    Buffer(const Buffer& o);
    Buffer& operator=(const Buffer& o);
    void initialize() override;
    void on_incoming_data(PomaPacketType& dta, const std::string& channel) override;
    void setup_cli(boost::program_options::options_description& desc) const override;
    void process_cli(boost::program_options::variables_map& vm);
    void flush() override;

protected:
    void collector_fn();

private:
    std::atomic_int buffer_size{0};
    int incoming_dtu{0};
    std::vector<std::thread> m_thread_pool;
    std::condition_variable outgoing_data_available_cv;
    std::mutex m_outgoing_queue_mutex;
    std::queue<PomaPacketType> m_outgoing_queue;
    unsigned int m_warn_size{1024};
    int m_packetskip{1};
    bool m_dontcopy{false};
};

#endif
