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

#ifndef POMAMODULE_H
#define POMAMODULE_H

#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <string>
#include <sstream>
#include <set>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <typeinfo>
#include <thread>
#include <atomic>
#include <algorithm>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/dll/alias.hpp>
#include <boost/function.hpp>
#include <boost/config.hpp>
#include <type_traits>

namespace poma {

// *********************************************************************
// BASE PACKET TEMPLATE
// *********************************************************************
template<typename X>
struct Packet {
    X m_data;
    boost::property_tree::ptree m_properties;
};

// *********************************************************************
// SERIALIZATION STUFF
// *********************************************************************
struct Serializable {
    virtual void serialize(std::string& output) const = 0;
    virtual void deserialize(std::string::const_iterator& from) = 0;
};

#define ASSERT_IS_POMA_SERIALIZABLE(X) static_assert(std::is_base_of<poma::Serializable, decltype(X)>::value, "X must inherit from poma::Serializable");

void pack(const std::string& source_data_string, std::string& packed_data_string)
{
    // Warning: assume a maximum length of 2^32 bytes
    uint32_t data_payload_size{(uint32_t) source_data_string.size()};
    // big endian
    packed_data_string.push_back((data_payload_size >> 24) & 0xFF);
    packed_data_string.push_back((data_payload_size >> 16) & 0xFF);
    packed_data_string.push_back((data_payload_size >> 8) & 0xFF);
    packed_data_string.push_back(data_payload_size & 0xFF);
    packed_data_string.insert(packed_data_string.end(), source_data_string.begin(), source_data_string.end());
}

std::string unpack(std::string::const_iterator& from)
{
	unsigned char b0 = (unsigned char) *from++;
    unsigned char b1 = (unsigned char) *from++;
    unsigned char b2 = (unsigned char) *from++;
    unsigned char b3 = (unsigned char) *from++;
	uint32_t data_payload_size{(uint32_t) ((b0 << 24)+(b1 << 16)+(b2 << 8)+ b3)};
	std::string result{std::string{from, from + data_payload_size}};
	from = from + data_payload_size;
	return result;
}

template<typename T>
void deserialize(Packet<T>& dta, const std::string& data_string)
{
	std::string::const_iterator str_iter{data_string.begin()};
	std::string json_string{unpack(str_iter)};
	std::istringstream iss{json_string};
    boost::property_tree::json_parser::read_json(iss, dta.m_properties);
    dta.m_data.deserialize(str_iter);
}

template<typename T>
void serialize(const Packet<T>& dta, std::string& data_string)
{
    ASSERT_IS_POMA_SERIALIZABLE(dta.m_data);
    std::stringstream ss;
    boost::property_tree::write_json(ss, dta.m_properties);
    auto json_string{ss.str()};
    pack(json_string, data_string);
    dta.m_data.serialize(data_string);
}

// *********************************************************************
// BASE LINK TEMPLATE
// *********************************************************************

/* Forward declaration */
template<typename T>
class BaseModule;

template<typename T>
struct Link {
    Link(std::shared_ptr<BaseModule<T>> module, bool do_debug)
        : m_module{module}, m_debug{do_debug} {};
    std::shared_ptr<BaseModule<T>> m_module;
    bool m_debug {false};
};

#define FATAL(msg) std::cerr << msg << std::endl; assert(0);

// *********************************************************************
// PROPERTY MANAGEMENT HELPER MACROS AND FUNCTIONS
// *********************************************************************
template<typename M>
bool set_field_value_from_string(M* field, std::string value)
{
    std::istringstream ss{value};
    M tmp;
    ss >> std::skipws >> std::boolalpha >> tmp;
    if (!ss.fail()) {
        *field = tmp;
        return true;
    } else {
        return false;
    }
}

template<>
bool set_field_value_from_string(bool* field, std::string value)
{
    if (value == "true") {
        *field = true;
    } else if (value == "false") {
        *field = false;
    } else {
        return false;
    }
    return true;
}

template<>
bool set_field_value_from_string(std::string* field, std::string value)
{
    *field = value;
    return true;
}

#define return_readable_property(rname, prop) \
if (rname == #prop) { std::stringstream tmp; tmp << std::boolalpha << prop; return tmp.str(); }

#define return_writable_property(rname, rvalue, prop) \
if (rname == #prop) { \
	if (poma::set_field_value_from_string(&(prop), rvalue)) set_reconfigure_module(); \
	std::stringstream outss; \
	outss << prop; \
	return outss.str(); \
}

#define RETURN_READABLE_PROPERTY(r, data, field) return_readable_property(name, field);
#define RETURN_WRITABLE_PROPERTY(r, data, field) return_writable_property(name, value, field);
#define GET_QUOTED_PROPERTY(field) #field
#define ENUMERATE_PROPERTY(r, data, i, field) BOOST_PP_COMMA_IF(i) GET_QUOTED_PROPERTY(field)


#define DECLARE_READABLE_PROPERTIES(props) \
	std::string on_read_property(const std::string& name) const override { \
		BOOST_PP_SEQ_FOR_EACH(RETURN_READABLE_PROPERTY,, props) \
	}

#define DECLARE_WRITABLE_PROPERTIES(props) \
	std::string on_write_property(const std::string& name, const std::string& value) override { \
		synchronized_scope(); \
		BOOST_PP_SEQ_FOR_EACH(RETURN_WRITABLE_PROPERTY,, props) \
	}

#define DECLARE_ENUMERATABLE_PROPERTIES(props) \
	std::string on_enumerate_properties() const override { \
		std::vector<std::string> properties { BOOST_PP_SEQ_FOR_EACH_I(ENUMERATE_PROPERTY, , props) }; \
		return boost::algorithm::join(properties, ","); \
	}

#define DECLARE_PROPERTIES(props) \
	DECLARE_READABLE_PROPERTIES(props) \
	DECLARE_WRITABLE_PROPERTIES(props) \
	DECLARE_ENUMERATABLE_PROPERTIES(props)

// *********************************************************************
// BASE MODULE TEMPLATE (Cloneable when subclassed)
// *********************************************************************
template<typename T>
class BaseModule {
public:
    BaseModule(const std::string& mid)
        : m_module_mutex{new std::mutex},
    m_module_id {mid}
    {
        sm_instances.push_back(this);
    }

    BaseModule(const BaseModule& o)
        : m_module_mutex{new std::mutex},
    m_module_id{o.m_module_id},
    m_sinks{o.m_sinks} {}

    BaseModule& operator=(const BaseModule& o)
    {
        if (m_module_mutex == nullptr) {
            delete m_module_mutex;
        }
        m_module_mutex = new std::mutex;
        m_sinks = o.m_sinks;
        m_module_id = o.m_module_id;
    }

    virtual ~BaseModule()
    {
        sm_instances.erase(std::remove(sm_instances.begin(), sm_instances.end(), this), sm_instances.end());
    }

    /* Module locking methods */
    void lock_module()
    {
        m_module_mutex->lock();
    }

    void unlock_module()
    {
        m_module_mutex->unlock();
    }

    bool check_reconfigure_module()
    {
        if (m_do_reconfigure_module) {
            m_do_reconfigure_module = false;
            return true;
        } else {
            return false;
        }
    }

    void set_reconfigure_module()
    {
        m_do_reconfigure_module = true;
    }

    void clear_reconfigure_module()
    {
        m_do_reconfigure_module = false;
    }

#define synchronized_scope() std::unique_lock<std::mutex> ___lock(*m_module_mutex);

    /* Global module functions */
    static void initialize_all()
    {
        for (auto i : sm_instances) {
            try {
                i->initialize();
            } catch (const boost::exception& e) {
                throw std::runtime_error(boost::diagnostic_information(e));
            } catch (std::exception e) {
                throw std::runtime_error (std::string{"Failed to initialize module "} + typeid(*i).name() + ":" + e.what());
            } catch (...) {
                throw std::runtime_error (std::string{"Failed to initialize module: "} + typeid(*i).name());
            }
        }
    }

    static void display_help_all()
    {
        // Only the help of not yet initialized modules will be "shown"
        boost::program_options::options_description desc;
        for (auto i : sm_instances) {
            std::string cname {i->getType()};
            if (sm_instances_classes.count(cname) == 0) {
                sm_instances_classes.insert(cname);
                i->setup_cli(desc);
            }
        }
        std::cerr << desc << std::endl;
    }

    static boost::program_options::variables_map setup_cli_all(int argc, char* argv[])
    {
        boost::program_options::options_description desc;
        for (auto i : sm_instances) {
            std::string cname {i->getType()};
            if (sm_instances_classes.count(cname) == 0) {
                sm_instances_classes.insert(cname);
                i->setup_cli(desc);
            }
        }
        boost::program_options::variables_map vm;
        try {
            boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
            boost::program_options::notify(vm);
        } catch (const boost::exception& e) {
            throw std::runtime_error(boost::diagnostic_information(e));
        } catch (std::exception &e) {
            throw std::runtime_error(std::string{"Failed to setup command line arguments: "} + e.what());
        }
        for (auto i : sm_instances) {
            i->process_cli(vm);
        }
        return vm;
    }

    /* Data processing method */
    void submit_data(Packet<T>& dta, const std::string& channel = "default")
    {
        auto it{m_sinks.find(channel)};
        if (it != m_sinks.end()) {
            for(auto& s : it->second) {
                try {
                    if (s.m_debug) {
                        auto before = std::chrono::high_resolution_clock::now();
                        std::cerr << "DEBUG: submit (CALL) from " << m_module_id << " : " << typeid(*this).name()
                                  << " to " << s.m_module->m_module_id << " : " << typeid(*(s.m_module)).name()
                                  << " on " << channel << std::endl;
                        try {
                            s.m_module->on_incoming_data(dta, channel);
                        } catch (const boost::exception& e) {
                            throw std::runtime_error(boost::diagnostic_information(e));
                        } catch(const std::exception& e) {
                            throw std::runtime_error(std::string{"DEBUG: Exception "} + e.what() + " while processing data in module " + s.m_module->m_module_id);
                        } catch(...) {
                            throw std::runtime_error(std::string{"DEBUG: Exception ??? while processing data in module "} + s.m_module->m_module_id);
                        }
                        auto after = std::chrono::high_resolution_clock::now();
                        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
                        std::cerr << "DEBUG: submit (RETURN) from " << m_module_id << " : " << typeid(*this).name()
                                  << " to " << s.m_module->m_module_id << " : " << typeid(*(s.m_module)).name()
                                  << " on " << channel
                                  << " duration " << elapsed << " ms" << std::endl;
                    } else {
                        s.m_module->on_incoming_data(dta, channel);
                    }
                } catch (const boost::exception& e) {
                    throw std::runtime_error(boost::diagnostic_information(e));
                } catch(const std::exception& e) {
                    throw std::runtime_error(std::string{"Exception "} + e.what() + " while processing data on channel " + channel + " in module " + s.m_module->m_module_id + " (sent from " + m_module_id + ")");
                } catch(...) {
                    throw std::runtime_error(std::string{"Unknown exception while processing data on channel "} + channel + " in module " + s.m_module->m_module_id + " (sent from " + m_module_id + ")");
                }
            }
        }
    }


    /* Dynamic property management methods */
    std::string read_property(const std::string& name, const std::string& channel = "default") const
    {
        std::stringstream ss;
        auto it{m_sinks.find(channel)};
        if (it != m_sinks.end()) {
            bool first{true};
            for(auto s : it->second) {
                try {
                    if (!first) ss << ",";
                    std::string val{s.m_module->on_read_property(name)};
                    if (val != "") {
                        first = false;
                        ss << val;
                    }
                } catch (const boost::exception& e) {
                    throw std::runtime_error(boost::diagnostic_information(e));
                } catch(const std::exception& e) {
                    throw std::runtime_error(std::string{"Exception "} + e.what() + " while reading property " + name + " on channel " + channel + " in module " + s.m_module->m_module_id + " (sent from " + m_module_id + ")");
                } catch(...) {
                    throw std::runtime_error(std::string{"Unknown exception while reading property "} + name + " on channel " + channel + " in module " + s.m_module->m_module_id + " (sent from " + m_module_id + ")");
                }
            }

        }
        return ss.str();
    }

    std::string write_property(const std::string& name, const std::string& value, const std::string& channel = "default")
    {
        std::stringstream ss;
        bool first{true};
        auto it{m_sinks.find(channel)};
        if (it != m_sinks.end()) {
            for(auto s : it->second) {
                try {
                    if (!first) ss << ",";
                    std::string val{s.m_module->on_write_property(name, value)};
                    if (val != "") {
                        first = false;
                        ss << val;
                    }
                } catch (const boost::exception& e) {
                    throw std::runtime_error(boost::diagnostic_information(e));
                } catch(const std::exception& e) {
                    throw std::runtime_error(std::string{"Exception "} + e.what() + " while writing value " + value + " to property " + name + " on channel " + channel + " in module " + s.m_module->m_module_id + " (sent from " + m_module_id + ")");
                } catch(...) {
                    throw std::runtime_error(std::string{"Unknown exception while writing value "} + value + " to property " + name + " on channel " + channel + " in module " + s.m_module->m_module_id + " (sent from " + m_module_id + ")");
                }
            }
        }
        return ss.str();
    }

    std::string enumerate_properties(const std::string& channel = "default") const
    {
        std::stringstream ss;
        bool first{true};
        auto it{m_sinks.find(channel)};
        if (it != m_sinks.end()) {
            for(auto s : it->second) {
                try {
                    if (!first) ss << ",";
                    std::string data{s.m_module->on_enumerate_properties()};
                    if (data != "") {
                        ss << data;
                        first = false;
                    }
                } catch (const boost::exception& e) {
                    throw std::runtime_error(boost::diagnostic_information(e));
                } catch(const std::exception& e) {
                    throw std::runtime_error(std::string{"Exception "} + e.what() + " while enumerating properties on channel " + channel + " in module " + s.m_module->m_module_id + " (sent from " + m_module_id + ")");
                } catch(...) {
                    throw std::runtime_error(std::string{"Unknown exception while enumerating properties on channel "} + channel + " in module " + s.m_module->m_module_id + " (sent from " + m_module_id + ")");
                }
            }
        }
        return ss.str();
    }

    virtual void flush() {}


    /* Pipeline construction methods */
    std::vector<std::string> get_channels() const
    {
        std::vector<std::string> keys;
        for(auto kv : m_sinks) {
            keys.push_back(kv.first);
        }
        return keys;
    }

    void clear_channels()
    {
        m_sinks.clear();
    }

    BaseModule<T>& operator >> (std::shared_ptr<BaseModule<T> > sink)
    {
        return connect_sink(sink);
    }

    std::shared_ptr<BaseModule<T> > connect_sink(std::shared_ptr<BaseModule<T> > m, const std::string& channel = "default", bool do_debug = false)
    {
        Link<T> link{m, do_debug};
        m_sinks[channel].push_back(link);
        return m;
    }

    /* Pipeline duplication methods */

    /* Create a clone of this object */
    virtual std::shared_ptr<BaseModule<T> > clone() const
    {
        throw std::runtime_error (std::string{"Clone not correctly implemented for this module: "} + m_module_id);
        return nullptr;
    };

    std::string get_module_id() const
    {
        return m_module_id;
    }

    /* Methods to be overridden */
    virtual void construct(std::vector<std::string> parameters) {};
    virtual void setup_cli(boost::program_options::options_description& desc) const {};
    virtual void process_cli(boost::program_options::variables_map& vm) {};
    virtual void initialize() {};
    virtual void start_processing()
    {
        throw std::runtime_error("Processing method must be overridden");
    };
    virtual void on_incoming_data(Packet<T>& dta, const std::string& channel)
    {
        submit_data(dta, channel);
    };
    virtual std::string on_read_property(const std::string& name) const
    {
        return "";
    };
    // Note: each module is responsible for making the following call thread safe
    virtual std::string on_write_property(const std::string& name, const std::string& value)
    {
        return "";
    };
    virtual std::string on_enumerate_properties() const
    {
        return "";
    };

    virtual std::string getType() const
    {
        return "Generic BaseModule";
    };

protected:
    static std::vector<BaseModule<T>*> sm_instances;
    static std::set<std::string> sm_instances_classes;

    std::mutex* m_module_mutex{nullptr};
    std::string m_module_id;

    std::unordered_map<std::string,std::vector<Link<T> > > m_sinks;

    bool m_do_reconfigure_module{true};
};

/* Static fields initialization */
template<typename T>
std::vector<BaseModule<T>*> BaseModule<T>::sm_instances;

template<typename T>
std::set<std::string> BaseModule<T>::sm_instances_classes;

/* Cloneable module */
template <typename D, typename T>
class Module : public BaseModule<T> {
public:
    Module(const std::string& mid) : BaseModule<T>(mid) {}
    
    std::shared_ptr<BaseModule<T> > clone() const override
    {
        auto ptr = new D {static_cast<D const&>(*this)};
        if (ptr == nullptr) {
            throw std::runtime_error (std::string{"Failed to allocate memory for cloned object in module "} + this->get_module_id());
            return nullptr;
        } else {
            auto head = std::shared_ptr<BaseModule<T> > {ptr};
            head->clear_channels();
            for (const auto& c : BaseModule<T>::get_channels()) {
                if ((c.length() >= 1) && (c.at(0) == '_')) { // channel starting with _
                    for (const auto& s : BaseModule<T>::m_sinks.find(c)->second) {
                        head->connect_sink(s.m_module, c);
                    }
                    continue;
                }
                for (const auto& s : BaseModule<T>::m_sinks.find(c)->second) {
                    head->connect_sink(s.m_module->clone(), c);
                }
            }
            return head;
        }
    }

    std::string getType() const override
    {
        return typeid(D).name();
    }
};

// *********************************************************************
// PARALLEL PROCESSOR MODULE TEMPLATE
// *********************************************************************
template<typename J>
class ParallelProcessorBaseModule : public BaseModule<J> {
public:
    ParallelProcessorBaseModule(const std::string& mid) : BaseModule<J>(mid) {}

    ~ParallelProcessorBaseModule()
    {
        for (auto& t : m_thread_pool) {
            t.detach();
        }
    }

    std::shared_ptr<BaseModule<J> > clone() const
    {
        throw std::runtime_error ("Cannot clone Parallel Processor object");
        return nullptr;
    }

    void start_processing()
    {
        unsigned long sink_count{this->m_sinks["default"].size()};
        if (sink_count) {
            for(auto s : this->m_sinks["default"]) {
                m_thread_pool.push_back(std::thread([=] {processor_fn(s.m_module);}));
            }
        }
        for(;;) {};
    }

    std::string getType()
    {
        return typeid(ParallelProcessorBaseModule).name();
    }

protected:
    void processor_fn(std::shared_ptr<BaseModule<J>> mod)
    {
        mod->start_processing();
    }

private:
    std::vector<std::thread> m_thread_pool;
};


// *********************************************************************
// FORK MODULE TEMPLATE
// *********************************************************************
template<typename J, bool stateless = false>
class ForkBaseModule : public BaseModule<J> {

public:
    ForkBaseModule(const std::string& mid) : BaseModule<J>(mid) {}

    ~ForkBaseModule()
    {
        assert(m_incoming_queue.size() == 0);
        assert(m_outgoing_queue.size() == 0);
        for (auto& t : m_thread_pool) {
            t.detach();
        }
    }

    void initialize()
    {
        int n_threads {(int) std::thread::hardware_concurrency()};
        if (m_thread_limit < 0) m_thread_limit = n_threads;
        n_threads = std::min(n_threads, m_thread_limit);
        std::shared_ptr<BaseModule<J> > head;
        if (n_threads <= 0) {
            // do nothing
            std::cerr << "Sequential operation in module " << this->get_module_id() << std::endl;
        } else if (this->m_sinks["template"].size() == 1) {
            head = this->m_sinks["template"][0].m_module;
            m_thread_pool.push_back(std::thread {&ForkBaseModule::collector_fn, this});
            std::cerr << "Parallel operation in module " << this->get_module_id() << ": using " << n_threads << " threads" << std::endl;
            for (int i {1}; i<n_threads; i++) {
                try {
                    if (stateless) {
                        m_thread_pool.push_back(std::thread {&ForkBaseModule::executor_fn, this, head});
                    } else {
                        std::shared_ptr<BaseModule<J> > chead = head->clone();
                        m_thread_pool.push_back(std::thread {&ForkBaseModule::executor_fn, this, chead});
                    }
                } catch (const boost::exception& e) {
                    throw std::runtime_error(boost::diagnostic_information(e));
                } catch(const std::exception& e) {
                    throw std::runtime_error (std::string{"Failed to initialize fork thread in module "} + this->get_module_id() + ":" + e.what());
                } catch (...) {
                    throw std::runtime_error (std::string{"Failed to initialize fork thread in module "} + this->get_module_id());
                }
            }
            m_thread_pool.push_back(std::thread {&ForkBaseModule::executor_fn, this, head});
        } else if (this->m_sinks["template"].size() == 0) {
            throw std::runtime_error (std::string{"Pipeline template channel is empty in module "} + this->get_module_id());
        } else {
            throw std::runtime_error (std::string{"Pipeline template channel is invalid in module "} + this->get_module_id());
        }
    }

    void on_incoming_data(Packet<J>& dta, const std::string& channel)
    {
        if (channel == "default") {
            if (m_thread_limit <= 0) {
                this->submit_data(dta, "template");
            } else {
                Packet<J> copy;
                copy.m_data = dta.m_data;
                copy.m_properties = dta.m_properties;
                {
                    m_incoming_buffer_size++;
                    std::unique_lock<std::mutex> lock(m_incoming_queue_mutex);
                    m_incoming_queue.push(copy);
                }
                incoming_data_available_cv.notify_one();
            }
        } else if (channel == "_join") {
            if (m_thread_limit <= 0) {
                this->submit_data(dta);
            } else {
                {
                    m_outgoing_buffer_size++;
                    std::unique_lock<std::mutex> lock(m_outgoing_queue_mutex);
                    m_outgoing_queue.push(dta);
                }
                outgoing_data_available_cv.notify_one();
            }
        }
    }

    std::string getType()
    {
        return typeid(ForkBaseModule).name();
    }

    std::shared_ptr<BaseModule<J> > clone()
    {
        throw std::runtime_error ("Cannot clone fork base module");
    }

    void setup_cli(boost::program_options::options_description& desc) const
    {
        boost::program_options::options_description pex("Parallel fork executor options");
        pex.add_options()
        ("threads", boost::program_options::value<int>()->default_value(-1), "force number of threads");
        desc.add(pex);
    }

    void process_cli(boost::program_options::variables_map& vm)
    {
        m_thread_limit = vm["threads"].as<int>();
    }

    void flush() override
    {
        while(m_incoming_buffer_size != 0 || m_outgoing_buffer_size != 0) {
            std::this_thread::yield();
        };
    }


protected:
    void executor_fn(std::shared_ptr<BaseModule<J> > head)
    {
        for(;;) {
            Packet<J> du;
            {
                std::unique_lock<std::mutex> lock(m_incoming_queue_mutex);
                incoming_data_available_cv.wait(lock, [&] { return !m_incoming_queue.empty(); });
                du = m_incoming_queue.front();
                m_incoming_queue.pop();
            }
            head->on_incoming_data(du, "default");
            m_incoming_buffer_size--;
        }
    }

    void collector_fn()
    {
        for(;;) {
            Packet<J> du;
            {
                std::unique_lock<std::mutex> lock(m_outgoing_queue_mutex);
                outgoing_data_available_cv.wait(lock, [&] { return !m_outgoing_queue.empty(); });
                du = m_outgoing_queue.front();
                m_outgoing_queue.pop();

            }
            this->submit_data(du, "default");
            m_outgoing_buffer_size--;
        }
    }

private:
    std::atomic_int m_incoming_buffer_size{0}, m_outgoing_buffer_size{0};
    int m_thread_limit {-1};
    std::vector<std::thread> m_thread_pool;
    std::condition_variable incoming_data_available_cv;
    std::mutex m_incoming_queue_mutex;
    std::queue<Packet<J> > m_incoming_queue;
    std::condition_variable outgoing_data_available_cv;
    std::mutex m_outgoing_queue_mutex;
    std::queue<Packet<J> > m_outgoing_queue;
};

// *********************************************************************
// JOIN MODULE TEMPLATE
// *********************************************************************

template<typename J>
class JoinBaseModule : public Module<JoinBaseModule<J>, J> {
public:
    JoinBaseModule(const std::string& mid) : Module<JoinBaseModule, J>(mid) {}

    void on_incoming_data(Packet<J>& dta, const std::string& channel)
    {
        this->submit_data(dta, "_join");
    }
};

// *********************************************************************
// CALLBACK SINK MODULE TEMPLATE
// *********************************************************************

template<typename J>
class CallbackSinkModule : public Module<CallbackSinkModule<J>, J> {
public:
    CallbackSinkModule(const std::string& mid) : Module<CallbackSinkModule, J>(mid) {}

    void on_incoming_data(Packet<J>& dta, const std::string& channel)
    {
        if (m_cb_function != nullptr) {
            m_cb_function(dta, channel);
        }
    }

    void set_callback(void (*cb_function) (Packet<J>& dta, const std::string& channel))
    {
        m_cb_function = cb_function;
    }

protected:
    void (*m_cb_function) (Packet<J>& dta, const std::string& channel);
};

// *********************************************************************
// MODULE FACTORY TYPE
// *********************************************************************
template<typename T>
struct ModuleFactory {
    virtual std::string name() const;
    virtual std::string description() const;
    virtual std::string construction_parameters() const;
    virtual bool can_be_source() const;
    virtual T* create(const std::string& name) const;
};

// *********************************************************************
// MODULE DEFINITION HELPER MACROS
// *********************************************************************

#define DEFAULT_DEFINITIONS(thedatatype) \
	using PomaDataType = thedatatype; \
	using PomaModuleType = poma::BaseModule<thedatatype>; \
	using PomaPacketType = poma::Packet<thedatatype>;

#define DEFAULT_EXPORT_ALL(themoduletype, themoduledescription, themoduleconstructionparameters, canbesource) \
	struct LocalFactory : public poma::ModuleFactory<PomaModuleType> { \
		std::string name() const override { \
			return #themoduletype; \
		} \
		std::string description() const override { \
			return themoduledescription; \
		} \
		std::string construction_parameters() const override { \
			return themoduleconstructionparameters; \
		} \
		bool can_be_source() const override { \
			return canbesource; \
		} \
		PomaModuleType* create(const std::string& name) const override { \
			return new themoduletype(name); \
		} \
	}; \
	extern "C" BOOST_SYMBOL_EXPORT LocalFactory factory; \
	LocalFactory factory;


#endif

}
