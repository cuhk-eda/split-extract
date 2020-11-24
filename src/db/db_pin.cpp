#include "db.h"
using namespace db;

/***** PinSTA *****/
PinSTA::PinSTA() {
    capacitance = -1.0;
    for (int i = 0; i < 4; i++) {
        aat[i] = 0.0;
        rat[i] = 0.0;
        slack[i] = 0.0;
        nCriticalPaths[i] = 0;
    }
}
/***** IOPin *****/
IOPin::IOPin(const string& name, const string& netName, const char direction) : _netName(netName), name(name) {
    type = new PinType(name, nullptr, direction);
    pin = new Pin(this);
}

IOPin::~IOPin() {
    delete pin;
    delete type;
}

/***** Pin *****/

void Pin::getPinCenter(int& x, int& y) const {
    if (cell) {
        x = cell->lx() + type->cx();
        y = cell->ly() + type->cy();
    } else if (iopin) {
        x = iopin->x + type->cx();
        y = iopin->y + type->cy();
    } else {
        printlog(LOG_ERROR, "invalid pin %s:%d", __FILE__, __LINE__);
        x = INT_MIN;
        y = INT_MIN;
    }
}

unsigned Pin::manhattanDistance(const Pin* m, const Pin* n) {
    int mx = 0;
    int my = 0;
    int nx = 0;
    int ny = 0;
    m->getPinCenter(mx, my);
    n->getPinCenter(nx, ny);
    return abs(mx - nx) + abs(my - ny);
}

unsigned Pin::manhattanDistance(const vector<Pin*>& m, const vector<Pin*>& n) {
    //  unsigned d = UINT_MAX;
    unsigned d = 0;
    for (const Pin* mpin : m) {
        for (const Pin* npin : n) {
            d += manhattanDistance(mpin, npin);
        }
    }
    return static_cast<double>(d) / m.size() / n.size();
}

unsigned Pin::manhattanDistance(Net* m, Net* n) {
    Pin* mIPin = m->iPin();
    Pin* nIPin = n->iPin();
    if (mIPin) {
        if (nIPin) {
            return manhattanDistance(mIPin, nIPin);
        } else {
            return manhattanDistance({mIPin}, n->pins);
        }
    } else {
        if (nIPin) {
            return manhattanDistance(m->pins, {nIPin});
        } else {
            return manhattanDistance(m->pins, n->pins);
        }
    }
}

/***** Pin Type *****/

bool PinType::comparePin(vector<PinType*> pins1, vector<PinType*> pins2) {
    std::sort(pins1.begin(), pins1.end(), [](PinType* lhs, PinType* rhs) { return (*lhs < *rhs); });
    std::sort(pins2.begin(), pins2.end(), [](PinType* lhs, PinType* rhs) { return (*lhs < *rhs); });
    for (unsigned j = 0; j != pins1.size(); ++j) {
        if (pins1[j]->_use != Use::UseEnum::Power && pins1[j]->_use != Use::UseEnum::Ground && *pins1[j] != *pins2[j]) {
            return false;
        }
    }
    return true;
}

