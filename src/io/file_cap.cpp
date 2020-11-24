#include "../db/db.h"
#include "../global.h"

using namespace db;

bool Database::readNetDetail(const std::string& file)
{
    ifstream fs(file.c_str());
    if (!fs.good()) {
        printlog(LOG_ERROR, "cannot open net detail file: %s", file.c_str());
        return false;
    }

    printlog(LOG_INFO, "reading %s", file.c_str());
    string line = "";
    while (getline(fs, line)) {
        istringstream iss(line);
        string buffer = "";
        string name = "";
        iss >> buffer;
        if (buffer != "|") continue;
        iss >> name >> buffer >> buffer;
        if (name.empty() || name == "|" || buffer == "|") continue;
        SplitNet* net = getSplitNet(name);
        if (!net) {
            printlog(LOG_ERROR, "split net %s not found in line %s", name.c_str(), line.c_str());
            break;
        }
        for (unsigned i = 0; i != 6;) {
            iss >> buffer;
            if (buffer == "|") ++i;
        }
        double cap = 0;
        iss >> cap;
        net->totalCap(cap);
    }

    fs.close();
    return true;
}

bool Database::readTimePath(const std::string& file, bool isConstrained) {
    ifstream fs(file.c_str());
    if (!fs.good()) {
        printlog(LOG_ERROR, "cannot open time path file: %s", file.c_str());
        return false;
    }

    printlog(LOG_INFO, "reading %s", file.c_str());
    string line = "";
    while (getline(fs, line)) {
        istringstream iss(line);
        string buffer = "";
        string cellName = "";
        string pinName = "";
        iss >> buffer;
        if (buffer != "|") continue;
        iss >> cellName >> buffer >> buffer >> buffer;
        if (cellName.empty() || cellName == "|" || buffer == "|") continue;
        Cell* cell = getCell(cellName);
        if (!cell) {
            printlog(LOG_ERROR, "cannot find cell: %s", cellName.c_str());
            return false;
        }
        double delay = 0;
        double arrive = 0;
        iss >> buffer >> pinName >> buffer >> buffer >> buffer >> buffer >> delay >> buffer >> arrive >> buffer;
        if (pinName.empty() || pinName == "|") continue;
        Pin* pin = cell->pin(pinName);
        if (!pin) {
            printlog(LOG_ERROR, "cannot find pin %s in cell: %s", pinName.c_str(), cellName.c_str());
            return false;
        }
        if (!pin->splitNet()) continue;
        pin->delay(max(pin->delay(), delay));
        pin->arrive(max(pin->arrive(), arrive));
        if (!isConstrained) continue;
        double require = 0;
        iss >> require;
        if (pin->require()) pin->require(min(pin->require(), require));
        else pin->require(require);
    }

    return true;
}

