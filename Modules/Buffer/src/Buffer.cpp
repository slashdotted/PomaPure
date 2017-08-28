

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
*/#include "Buffer.h"

DEFAULT_EXPORT_ALL(Buffer, "Buffering module", "", false)

Buffer::Buffer(const std::string& mid) : poma::Module<Buffer,PomaDataType>(mid) {}

Buffer::~Buffer()
{
    assert(m_outgoing_queue.size() == 0);
    for (auto& t : m_thread_pool) {
        t.detach();
    }
}

Buffer::Buffer(const Buffer& o) : poma::Module<Buffer,PomaDataType>(o.m_module_id)
{
    m_warn_size = o.m_warn_size;
    m_packetskip = o.m_packetskip;
}

Buffer& Buffer::operator=(const Buffer& o)
{
    m_module_id = o.m_module_id;
    m_warn_size = o.m_warn_size;
    m_packetskip = o.m_packetskip;
    return *this;
}

void Buffer::initialize()
{
    m_thread_pool.push_back(std::thread {&Buffer::collector_fn, this});
}

void Buffer::on_incoming_data(PomaPacketType& dta, const std::string& channel)
{
    if (incoming_dtu % m_packetskip != 0) {
        return;
    }
    {
        std::unique_lock<std::mutex> lock(m_outgoing_queue_mutex);
        buffer_size++;
        m_outgoing_queue.push(dta);
        if (++incoming_dtu % 256 == 0) {
            std::cerr << ">>>> Queue " << m_module_id << ", size: " << m_outgoing_queue.size() << std::endl;
        }
        if (m_outgoing_queue.size() > m_warn_size) {
            std::cerr << ">>>> Warning " << m_module_id << ": queue size is << " << m_outgoing_queue.size() << std::endl;
        }
    }
    outgoing_data_available_cv.notify_one();
}

void Buffer::setup_cli(boost::program_options::options_description& desc) const
{
    boost::program_options::options_description buf("Buffer options");
    buf.add_options()
    ("warnsize", boost::program_options::value<int>()->default_value(1024), "buffer warn size")
    ("packetskip", boost::program_options::value<int>()->default_value(1), "number of skip packets");
    desc.add(buf);
}

void Buffer::process_cli(boost::program_options::variables_map& vm)
{
    m_warn_size = (unsigned int) vm["warnsize"].as<int>();
    m_packetskip = vm["packetskip"].as<int>();
}

void Buffer::flush()
{
    while(buffer_size != 0) {
        std::this_thread::yield();
    };
}

void Buffer::collector_fn()
{
    while(true) {
        PomaPacketType du;
        {
            std::unique_lock<std::mutex> lock(m_outgoing_queue_mutex);
            while (m_outgoing_queue.empty()) {
                outgoing_data_available_cv.wait(lock, [&] { return !m_outgoing_queue.empty(); });
            }
            du = m_outgoing_queue.front();
            m_outgoing_queue.pop();
        }
        submit_data(du);
        buffer_size--;
    }
}
