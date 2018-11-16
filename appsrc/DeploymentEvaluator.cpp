
#include "DeploymentEvaluator.h"
#include <iostream>
#include <sstream>
#include <string>
#include <boost/property_tree/json_parser.hpp>

namespace poma {

    Machine::Machine(const std::string& hostname)
            : m_hostname{hostname} {
    }

    void Machine::add_flag(const std::string& flag) {
        m_flags.insert(flag);
    }
    void Machine::add_resource(const std::string& name, double amount) {
        m_consumable_resources.insert({name, amount});
    }

    bool Machine::assign_module(const Module &m) {
        // Check flags
        // Check requirements


        if (m.mtype == "ZeroMQSink") {
            // Netsink modules affect the network score (counting the bandwidth)

        } else {
            //
        }
    }

    std::vector<std::string> Machine::assigned_modules() {

    }

    void DeploymentEvaluator::loadMachineData(std::istream &is) {
        boost::property_tree::ptree pt;
        boost::property_tree::json_parser::read_json(is, pt);
        boost::property_tree::ptree jmodules{pt.get_child("machines.")};
       /* p_source_mid = pt.get("source", "");
        // Process modules
        for(auto &m : jmodules) {
            Module mod;
            mod.mid = m.first.data();
            if (mod.mid.find("#") == 0) continue; // disabled modules
            for (auto &md : m.second) {
                if (md.first == "type") {
                    mod.mtype = md.second.data();
                } else if (md.first == "cparams") {
                    for (auto &cp : md.second) {
                        mod.cparams.push_back(cp.second.data());
                    }
                } else if (md.first == "#host") {
                    mod.mhost = md.second.data();
                } else if (md.first == "#flags") {
                    mod.flags =
                } else {
                    mod.mparams[md.first] = md.second.data();
                }
            }
            if (mod.mtype == "") {
                die("invalid module type definition: " + mod.mtype);
            }
            p_modules[mod.mid] = mod;
        }*/




    }
}