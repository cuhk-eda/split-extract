#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "db.h"

using namespace db;

#include "../ut/utils.h"

/***** Cell *****/

Cell::~Cell() {
    for (Pin* pin : _pins) {
        delete pin;
    }
}

Pin* Cell::pin(const std::string& name) const {
    for (Pin* pin : _pins) {
        if (pin->type->name() == name) {
            return pin;
        }
    }
    return nullptr;
}

void Cell::ctype(CellType* t) {
    if (!t) {
        return;
    }
    if (_type) {
        printlog(LOG_ERROR, "type of cell %s already set", _name.c_str());
        return;
    }
    _type = t;
    ++(_type->usedCount);
    _pins.resize(_type->pins.size(), nullptr);
    for (unsigned i = 0; i != _pins.size(); ++i) {
        _pins[i] = new Pin(this, i);
    }
}

int Cell::lx() const { return database.placement().x(const_cast<Cell*>(this)); }
int Cell::ly() const { return database.placement().y(const_cast<Cell*>(this)); }

bool Cell::r90() const { return database.placement().r90(const_cast<Cell*>(this)); }
bool Cell::flipX() const { return database.placement().flipX(const_cast<Cell*>(this)); }
bool Cell::flipY() const { return database.placement().flipY(const_cast<Cell*>(this)); }

int Cell::siteWidth() const { return width() / database.siteW; }

int Cell::siteHeight() const { return height() / database.siteH; }

bool Cell::placed() const { return database.placement().placed(const_cast<Cell*>(this)); }

void Cell::place(int x, int y) {
    if (_fixed) {
        printlog(LOG_WARN, "moving fixed cell %s to (%d,%d)", _name.c_str(), x, y);
    }
    database.placement().place(this, x, y);
}

void Cell::place(int x, int y, bool r90, bool flipX, bool flipY) {
    if (_fixed) printlog(LOG_WARN, "moving fixed cell %s to (%d,%d)", _name.c_str(), x, y);
    database.placement().place(this, x, y, r90, flipX, flipY);
}

void Cell::unplace() {
    if (_fixed) {
        printlog(LOG_WARN, "unplace fixed cell %s", _name.c_str());
    }
    database.placement().place(this);
}

void Cell::transform(Geometry& geo) const {
    if (r90()) {
        geo.r90(_type->height);
        if (flipX()) geo.flipX(_type->height);
        if (flipY()) geo.flipY(_type->width);
    } else {
        if (flipX()) geo.flipX(_type->width);
        if (flipY()) geo.flipY(_type->height);
    }
    geo.shift(lx(), ly());
}

/***** Cell Type *****/

CellType::CellType(const string& name, int libcell) {
    this->name = name;
    _libcell = libcell;

    std::vector<std::string> tokens;
    boost::split(tokens, name, boost::is_any_of("_"));
    // convert gate type to ASCII value
    for (unsigned i = 0; i < tokens[0].size(); i++) {
        gType += tokens[0][i];
    }
    //  gStrength = boost::lexical_cast<int>(tokens[1].substr(1, tokens[1].size() - 1));
}

CellType::~CellType() {
    for (PinType* pin : pins) {
        delete pin;
    }
}

PinType* CellType::addPin(const string& name, const NDR* ndr, const char direction, const Use::UseEnum use) {
    PinType* newpintype = new PinType(name, ndr, direction, use);
    pins.push_back(newpintype);
    return newpintype;
}

const PinType* CellType::getPin(const string& name) const {
    for (const PinType* pin : pins) {
        if (pin->name() == name) {
            return pin;
        }
    }
    return nullptr;
}

void CellType::setOrigin(const int x, const int y) {
    _originX = x;
    _originY = y;
}

bool CellType::operator==(const CellType& r) const {
    if (width != r.width || height != r.height) {
        return false;
    } else if (_originX != r.originX() || _originY != r.originY() || _symmetry != r.symmetry() ||
               pins.size() != r.pins.size()) {
        return false;
    } else if (edgetypeL != r.edgetypeL || edgetypeR != r.edgetypeR) {
        return false;
    } else {
        //  return PinType::comparePin(pins, r.pins);
        for (unsigned i = 0; i != pins.size(); ++i) {
            if (pins[i]->use() != Use::UseEnum::Power && pins[i]->use() != Use::UseEnum::Ground &&
                *pins[i] != *r.pins[i]) {
                return false;
            }
        }
    }
    return true;
}
