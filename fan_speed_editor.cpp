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
    std::map<std::string, int> adresses;
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
            adresses[std::string(key)] = std::stoul(std::string(value), nullptr, 16);

        if (config.contains("saveable_params"))
            for (auto& item : config["saveable_params"])
                if (adresses.find(std::string(item)) != adresses.end())
                    saveable_params.insert(std::string(item));

        if (config.contains("changeable_params"))
            for (auto& item : config["changeable_params"])
                if (adresses.find(std::string(item)) != adresses.end())
                    changeable_params.insert(std::string(item));

        if (config.contains("categorical_params"))
            for (auto& [param, categs] : config["categorical_params"].items())
            {
                if (adresses.find(std::string(param)) == adresses.end())
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
        assert(config->adresses.find(param) != config->adresses.end() && "ERROR: parameter not found");
        return (int)_ec->readByte(config->adresses[param]);
    }

    int getParam(std::string param1, std::string param2)
    {
        int v1 = getParam(param1);
        int v2 = getParam(param2);
        return (v1 << 8) | v2;
    }

    void setParam(std::string paramName, int paramValue)
    {
        _ec->writeByte(config->adresses[paramName], (BYTE)paramValue);
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

class FanSpeedReader
{
private:
    EmbeddedControllerWrapper::Ptr _ecw;

public:
    FanSpeedReader() : _ecw(EmbeddedControllerWrapper::instance())
    {

    }

    void Show()
    {
        int cpu_temp = _ecw->getParam("realtime_cpu_temp");
        int gpu_temp = _ecw->getParam("realtime_gpu_temp");

        int cpu_fan_prc = _ecw->getParam("realtime_cpu_fan_speed");
        int gpu_fan_prc = _ecw->getParam("realtime_gpu_fan_speed");

        int cpu_fan_t1 = _ecw->getParam("cpu_fan_speed_t1");
        int cpu_fan_t2 = _ecw->getParam("cpu_fan_speed_t2");
        int cpu_fan_t3 = _ecw->getParam("cpu_fan_speed_t3");
        int cpu_fan_t4 = _ecw->getParam("cpu_fan_speed_t4");
        int cpu_fan_t5 = _ecw->getParam("cpu_fan_speed_t5");
        int cpu_fan_t6 = _ecw->getParam("cpu_fan_speed_t6");
        int cpu_fan_t7 = _ecw->getParam("cpu_fan_speed_t7");

        int cpu_temp_t1 = _ecw->getParam("cpu_temp_t1");
        int cpu_temp_t2 = _ecw->getParam("cpu_temp_t2");
        int cpu_temp_t3 = _ecw->getParam("cpu_temp_t3");
        int cpu_temp_t4 = _ecw->getParam("cpu_temp_t4");
        int cpu_temp_t5 = _ecw->getParam("cpu_temp_t5");
        int cpu_temp_t6 = _ecw->getParam("cpu_temp_t6");

        int gpu_fan_t1 = _ecw->getParam("gpu_fan_speed_t1");
        int gpu_fan_t2 = _ecw->getParam("gpu_fan_speed_t2");
        int gpu_fan_t3 = _ecw->getParam("gpu_fan_speed_t3");
        int gpu_fan_t4 = _ecw->getParam("gpu_fan_speed_t4");
        int gpu_fan_t5 = _ecw->getParam("gpu_fan_speed_t5");
        int gpu_fan_t6 = _ecw->getParam("gpu_fan_speed_t6");
        int gpu_fan_t7 = _ecw->getParam("gpu_fan_speed_t7");

        int gpu_temp_t1 = _ecw->getParam("gpu_temp_t1");
        int gpu_temp_t2 = _ecw->getParam("gpu_temp_t2");
        int gpu_temp_t3 = _ecw->getParam("gpu_temp_t3");
        int gpu_temp_t4 = _ecw->getParam("gpu_temp_t4");
        int gpu_temp_t5 = _ecw->getParam("gpu_temp_t5");
        int gpu_temp_t6 = _ecw->getParam("gpu_temp_t6");

        int cpu_fan_r = _ecw->getParam("realtime_cpu_fan_rpm_b1", "realtime_cpu_fan_rpm_b2");
        int cpu_fan = cpu_fan_r ? 478000 / cpu_fan_r : 0;

        int gpu_fan_r = _ecw->getParam("realtime_gpu_fan_rpm_b1", "realtime_gpu_fan_rpm_b2");
        int gpu_fan = gpu_fan_r ? 478000 / gpu_fan_r : 0;

        bool gpu_integrated = !gpu_temp ? true : false;

        auto& cp = config->categorical_params;

        int shft_i = _ecw->getParam("shift_mode");
        std::string shift_mode = (cp.find("shift_mode") != cp.end() && cp["shift_mode"].find(shft_i) != cp["shift_mode"].end()) ? cp["shift_mode"][shft_i] : "undefined";

        int fnmd_i = _ecw->getParam("fan_mode");
        std::string fan_mode = (cp.find("fan_mode") != cp.end() && cp["fan_mode"].find(fnmd_i) != cp["fan_mode"].end()) ? cp["fan_mode"][fnmd_i] : "undefined";

        int usbpwr_i = _ecw->getParam("usb_power_share");
        std::string usb_power_share = (cp.find("usb_power_share") != cp.end() && cp["usb_power_share"].find(usbpwr_i) != cp["usb_power_share"].end()) ? cp["usb_power_share"][usbpwr_i] : "undefined";

        std::cout << "gpu_integrated: " << gpu_integrated << std::endl;
        std::cout << "cpu: " << cpu_temp << "C, " << cpu_fan << "rpm (" << cpu_fan_prc << "%)" << std::endl;
        std::cout << "gpu: " << gpu_temp << "C, " << gpu_fan << "rpm (" << gpu_fan_prc << "%)" << std::endl;

        std::cout << "cpu_tmp_thr: " << "00C    ";
        std::cout << cpu_temp_t1 << "C    ";
        std::cout << cpu_temp_t2 << "C    ";
        std::cout << cpu_temp_t3 << "C    ";
        std::cout << cpu_temp_t4 << "C    ";
        std::cout << cpu_temp_t5 << "C    ";
        std::cout << cpu_temp_t6 << "C    ";
        std::cout << std::endl;

        std::cout << "cpu_fan_thr: " << "    ";
        std::cout << cpu_fan_t1 << "%    ";
        std::cout << cpu_fan_t2 << "%    ";
        std::cout << cpu_fan_t3 << "%    ";
        std::cout << cpu_fan_t4 << "%    ";
        std::cout << cpu_fan_t5 << "%    ";
        std::cout << cpu_fan_t6 << "%    ";
        std::cout << cpu_fan_t7 << "%    ";
        std::cout << std::endl;

        std::cout << "gpu_tmp_thr: " << "00C    ";
        std::cout << gpu_temp_t1 << "C    ";
        std::cout << gpu_temp_t2 << "C    ";
        std::cout << gpu_temp_t3 << "C    ";
        std::cout << gpu_temp_t4 << "C    ";
        std::cout << gpu_temp_t5 << "C    ";
        std::cout << gpu_temp_t6 << "C    ";
        std::cout << std::endl;

        std::cout << "gpu_fan_thr: " << "    ";
        std::cout << gpu_fan_t1 << "%    ";
        std::cout << gpu_fan_t2 << "%    ";
        std::cout << gpu_fan_t3 << "%    ";
        std::cout << gpu_fan_t4 << "%    ";
        std::cout << gpu_fan_t5 << "%    ";
        std::cout << gpu_fan_t6 << "%    ";
        std::cout << gpu_fan_t7 << "%    ";
        std::cout << std::endl;

        std::cout << "shift mode: " << shift_mode << std::endl;
        std::cout << "fan mode: " << fan_mode << std::endl;
        std::cout << "usb power share: " << usb_power_share << std::endl;
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
    FanSpeedReader fsr;

    if (argc > 1)
    {
        if (!strcmp(argv[1], "-p"))
            fsr.Show();
        else if (!strcmp(argv[1], "-s") && argc == 3)
            fsr.Save(argv[2]);
        else if (!strcmp(argv[1], "-s"))
            fsr.Save();
        else if (!strcmp(argv[1], "-l") && argc == 3)
        {
            fsr.Load(argv[2]);
            fsr.Show();
        }
        else if (!strcmp(argv[1], "-l"))
        {
            fsr.Load();
            fsr.Show();
        }
        else if (!strcmp(argv[1], "-c") && argc == 4)
        {
            fsr.SetParam(argv[2], argv[3]);
            fsr.Show();
        }
        else if (!strcmp(argv[1], "-pc"))
            fsr.ShowChangeableParams();
        else
            PrintUsage();
    }
    else
    {
        PrintUsage();
    }

    return 0;
}