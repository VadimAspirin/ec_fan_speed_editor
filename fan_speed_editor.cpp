#include <iostream>
#include <map>
#include <set>
#include <windows.h>
#include <memory>
#include <fstream>
#include <string>

#include "3rdparty/nlohmann/json.hpp"
#include "3rdparty/EmbeddedController/ec.hpp"

using json = nlohmann::json;

#undef NDEBUG

#include <cassert>

struct Config
{
    std::map<std::string, int> addresses;
    std::map<std::string, int> addresses_dual;
    std::set<std::string> saveable_params;
    std::set<std::string> changeable_params;
    std::map<std::string, std::map<int, std::string>> categorical_params;

    Config()
    {
        std::string dataDir{ "data/" };

        std::string configPath = dataDir + "config.json";
        std::ifstream configFile(configPath);
        assert(configFile.is_open() && "Config file does not exist");
        json config = json::parse(configFile);

        assert(config.contains("address_file") && "address_file option does not exist in config");
        std::string adressPath = dataDir + std::string(config["address_file"]);
        std::ifstream adressFile(adressPath);
        assert(adressFile.is_open() && "Adress file does not exist");
        json addrs = json::parse(adressFile);

        for (auto& [key, value] : addrs.items())
        {
            if (addrs[key].is_array())
            {
                assert(addrs[key].size() == 2 && "array type params can only have size equal to two");
                addresses[std::string(key)] = -2;
                addresses_dual[std::string(key) + "_b1"] = std::stoul(std::string(value[0]), nullptr, 16);
                addresses_dual[std::string(key) + "_b2"] = std::stoul(std::string(value[1]), nullptr, 16);
            }
            else
            {
                addresses[std::string(key)] = std::stoul(std::string(value), nullptr, 16);
            }
        }

        if (config.contains("saveable_params"))
            for (auto& item : config["saveable_params"])
                if (addresses.find(std::string(item)) != addresses.end())
                    saveable_params.insert(std::string(item));

        if (config.contains("changeable_params"))
            for (auto& item : config["changeable_params"])
                if (addresses.find(std::string(item)) != addresses.end())
                    changeable_params.insert(std::string(item));

        if (config.contains("categorical_params"))
            for (auto& [param, categs] : config["categorical_params"].items())
            {
                if (addresses.find(std::string(param)) == addresses.end())
                    continue;

                categorical_params[std::string(param)] = std::map<int, std::string>();
                for (auto& [code, name] : categs.items())
                    categorical_params[std::string(param)][std::stoul(std::string(code), nullptr, 16)] = std::string(name);
            }
    }
};

std::shared_ptr<Config> config = std::make_shared<Config>();

class EmbeddedControllerWrapper
{
public:
    typedef std::shared_ptr<EmbeddedControllerWrapper> Ptr;

private:
    std::shared_ptr<EmbeddedController> _ec;
    inline static EmbeddedControllerWrapper::Ptr _ecw;

    EmbeddedControllerWrapper()
    {
        _ec = std::make_shared<EmbeddedController>();

        assert(_ec->driverFileExist && "ERROR: driver not found");
        assert(_ec->driverLoaded && "ERROR: driver not loaded");
    }

public:

    int getParam(std::string param)
    {
        assert(config->addresses.find(param) != config->addresses.end() && "ERROR: parameter not found");
        if (config->addresses[param] != -2)
        {
            return (int)_ec->readByte(config->addresses[param]);
        }
        else
        {
            assert(config->addresses_dual.find(param + "_b1") != config->addresses_dual.end() && "ERROR: parameter not found");
            assert(config->addresses_dual.find(param + "_b2") != config->addresses_dual.end() && "ERROR: parameter not found");
            int v1 = (int)_ec->readByte(config->addresses_dual[param + "_b1"]);
            int v2 = (int)_ec->readByte(config->addresses_dual[param + "_b2"]);
            return (v1 << 8) | v2;
        }
    }

    void setParam(std::string paramName, int paramValue)
    {
        _ec->writeByte(config->addresses[paramName], (BYTE)paramValue);
    }

    static EmbeddedControllerWrapper::Ptr instance()
    {
        if (!_ecw)
            _ecw = EmbeddedControllerWrapper::Ptr(new EmbeddedControllerWrapper());
        return _ecw;
    }

    ~EmbeddedControllerWrapper()
    {
        if (_ec)
            _ec->close();
    }
};

class FanSpeedEditor
{
private:
    EmbeddedControllerWrapper::Ptr _ecw;

public:
    FanSpeedEditor() : _ecw(EmbeddedControllerWrapper::instance())
    {

    }

    void Show()
    {
        std::set<std::string> used_params;

        auto keys_is_exist = [&](std::vector<std::string> param_list) -> bool
        {
            for (const auto& p : param_list)
                if (config->addresses.find(p) == config->addresses.end())
                    return false;
            for (const auto& p : param_list)
                used_params.insert(p);
            return true;
        };

        if(keys_is_exist({ "realtime_cpu_temp" , "realtime_cpu_fan_rpm" , "realtime_cpu_fan_speed" }))
        {
            int cpu_temp = _ecw->getParam("realtime_cpu_temp");
            int cpu_fan = _ecw->getParam("realtime_cpu_fan_rpm");
            int cpu_fan_prc = _ecw->getParam("realtime_cpu_fan_speed");

            cpu_fan = cpu_fan ? 478000 / cpu_fan : 0;
            std::cout << "cpu: " << cpu_temp << "C, " << cpu_fan << "rpm (" << cpu_fan_prc << "%)" << std::endl;
        }

        if (keys_is_exist({ "realtime_gpu_temp" , "realtime_gpu_fan_rpm" , "realtime_gpu_fan_speed" }))
        {
            int gpu_temp = _ecw->getParam("realtime_gpu_temp");
            int gpu_fan = _ecw->getParam("realtime_gpu_fan_rpm");
            int gpu_fan_prc = _ecw->getParam("realtime_gpu_fan_speed");

            gpu_fan = gpu_fan ? 478000 / gpu_fan : 0;
            std::cout << "gpu: " << gpu_temp << "C, " << gpu_fan << "rpm (" << gpu_fan_prc << "%)" << std::endl;
        }

        if (keys_is_exist({ "cpu_temp_t1" , "cpu_temp_t2" , "cpu_temp_t3" , "cpu_temp_t4" , "cpu_temp_t5" , "cpu_temp_t6" ,
            "cpu_fan_speed_t1", "cpu_fan_speed_t2", "cpu_fan_speed_t3", "cpu_fan_speed_t4", "cpu_fan_speed_t5", "cpu_fan_speed_t6", "cpu_fan_speed_t7"}))
        {
            std::cout << "cpu_tmp_thr: " << "00C    ";
            std::cout << _ecw->getParam("cpu_temp_t1") << "C    ";
            std::cout << _ecw->getParam("cpu_temp_t2") << "C    ";
            std::cout << _ecw->getParam("cpu_temp_t3") << "C    ";
            std::cout << _ecw->getParam("cpu_temp_t4") << "C    ";
            std::cout << _ecw->getParam("cpu_temp_t5") << "C    ";
            std::cout << _ecw->getParam("cpu_temp_t6") << "C    ";
            std::cout << std::endl;

            std::cout << "cpu_fan_thr: " << "    ";
            std::cout << _ecw->getParam("cpu_fan_speed_t1") << "%    ";
            std::cout << _ecw->getParam("cpu_fan_speed_t2") << "%    ";
            std::cout << _ecw->getParam("cpu_fan_speed_t3") << "%    ";
            std::cout << _ecw->getParam("cpu_fan_speed_t4") << "%    ";
            std::cout << _ecw->getParam("cpu_fan_speed_t5") << "%    ";
            std::cout << _ecw->getParam("cpu_fan_speed_t6") << "%    ";
            std::cout << _ecw->getParam("cpu_fan_speed_t7") << "%    ";
            std::cout << std::endl;
        }

        if (keys_is_exist({ "gpu_temp_t1" , "gpu_temp_t2" , "gpu_temp_t3" , "gpu_temp_t4" , "gpu_temp_t5" , "gpu_temp_t6" ,
            "gpu_fan_speed_t1", "gpu_fan_speed_t2", "gpu_fan_speed_t3", "gpu_fan_speed_t4", "gpu_fan_speed_t5", "gpu_fan_speed_t6", "gpu_fan_speed_t7" }))
        {
            std::cout << "gpu_tmp_thr: " << "00C    ";
            std::cout << _ecw->getParam("gpu_temp_t1") << "C    ";
            std::cout << _ecw->getParam("gpu_temp_t2") << "C    ";
            std::cout << _ecw->getParam("gpu_temp_t3") << "C    ";
            std::cout << _ecw->getParam("gpu_temp_t4") << "C    ";
            std::cout << _ecw->getParam("gpu_temp_t5") << "C    ";
            std::cout << _ecw->getParam("gpu_temp_t6") << "C    ";
            std::cout << std::endl;

            std::cout << "gpu_fan_thr: " << "    ";
            std::cout << _ecw->getParam("gpu_fan_speed_t1") << "%    ";
            std::cout << _ecw->getParam("gpu_fan_speed_t2") << "%    ";
            std::cout << _ecw->getParam("gpu_fan_speed_t3") << "%    ";
            std::cout << _ecw->getParam("gpu_fan_speed_t4") << "%    ";
            std::cout << _ecw->getParam("gpu_fan_speed_t5") << "%    ";
            std::cout << _ecw->getParam("gpu_fan_speed_t6") << "%    ";
            std::cout << _ecw->getParam("gpu_fan_speed_t7") << "%    ";
            std::cout << std::endl;
        }

        auto& cp = config->categorical_params;
        for (auto& [k, _] : config->addresses)
        {
            if (used_params.find(k) != used_params.end())
                continue;

            int v = _ecw->getParam(k);
            std::cout << k << ": ";
            std::cout << ((cp.find(k) != cp.end() && cp[k].find(v) != cp[k].end()) ? cp[k][v] : std::to_string(v));
            std::cout << std::endl;
        }
    }

    void ShowChangeableParams()
    {
        for (const auto& param : config->changeable_params)
            std::cout << param << std::endl;
    }

    void SetParam(std::string paramName, std::string paramValue)
    {
        std::cout << paramName << ": " << paramValue << " | ";
        assert(config->changeable_params.find(paramName) != config->changeable_params.end() && "ERROR: parameter not found");

        int paramValueInt = -1;
        try
        {
            paramValueInt = std::stoi(paramValue);
        }
        catch (...)
        {
            if (config->categorical_params.find(paramName) != config->categorical_params.end())
            {
                for (auto& [addr, name] : config->categorical_params[paramName])
                    if (paramValue == name)
                    {
                        paramValueInt = addr;
                        break;
                    }
            }
        }
        assert(paramValueInt != -1 && "ERROR: parameter label not found");

        if (_ecw->getParam(paramName) == paramValueInt)
            std::cout << "NOT CHANGED" << std::endl;
        else
        {
            _ecw->setParam(paramName, paramValueInt);
            std::cout << "LOADED" << std::endl;
        }
    }

    void Save(std::string profileName = "profile.ini")
    {
        std::ofstream os(profileName);
        for (const auto& param : config->saveable_params)
        {
            os << param << '\n';
            auto paramValue = _ecw->getParam(param);
            if (config->categorical_params.find(param) != config->categorical_params.end() &&
                config->categorical_params[param].find(paramValue) != config->categorical_params[param].end())
            {
                os << config->categorical_params[param][paramValue] << '\n';
            }
            else
            {
                os << paramValue << '\n';
            }
        }
        os.close();
        std::cout << "Save success\n";
    }

    void Load(std::string profileName = "profile.ini")
    {
        std::ifstream profileFile(profileName, std::ios::in);
        assert(profileFile.is_open() && "Adress file does not exist");

        std::string paramName, paramValue;
        while (profileFile >> paramName >> paramValue)
            SetParam(paramName, paramValue);
        std::cout << "Load success\n";
    }
};

void PrintUsage()
{
    std::cout << "-p - print state\n";
    std::cout << "-s [file_name] - save profile\n";
    std::cout << "-l [file_name] - load profile\n";
    std::cout << "-pc - print changeable params\n";
    std::cout << "-c <param_name> <param_value> - change param\n";
}

// MSI Center - User Scenario:
// 1. Extream Performance: (Turbo, Advanced), (Turbo, Auto)
// 2. Balanced: (Comfort, Auto)
// 3. Silent: (Comfort, Silent)
// 4. Super Battery: (Eco, Auto)


int main(int argc, char** argv)
{
    FanSpeedEditor fse;

    if (argc > 1)
    {
        if (!strcmp(argv[1], "-p"))
            fse.Show();
        else if (!strcmp(argv[1], "-s") && argc == 3)
            fse.Save(argv[2]);
        else if (!strcmp(argv[1], "-s"))
            fse.Save();
        else if (!strcmp(argv[1], "-l") && argc == 3)
        {
            fse.Load(argv[2]);
            fse.Show();
        }
        else if (!strcmp(argv[1], "-l"))
        {
            fse.Load();
            fse.Show();
        }
        else if (!strcmp(argv[1], "-c") && argc == 4)
        {
            fse.SetParam(argv[2], argv[3]);
            fse.Show();
        }
        else if (!strcmp(argv[1], "-pc"))
            fse.ShowChangeableParams();
        else
            PrintUsage();
    }
    else
    {
        PrintUsage();
    }

    return 0;
}