#include "db.h"
using namespace db;

#include "../ut/utils.h"

/* get layer by name */
Layer* Database::getLayer(const string& name)
{
    for (Layer* layer : layers) {
        if (layer->name() == name) {
            return layer;
        }
    }
    return nullptr;
}

/* get routing layer by index : 0=M1 */
Layer* Database::getRLayer(const int index)
{
    for (Layer* layer : layers) {
        if (layer->rIdx == index) {
            return layer;
        }
    }
    return nullptr;
}

/* get cut layer by index : 0=M1/2 */
const Layer* Database::getCLayer(const unsigned index) const
{
    for (const Layer* layer : layers) {
        if (layer->cIdx == (int)index) {
            return layer;
        }
    }
    return nullptr;
}

/* get cell type by name */
CellType* Database::getCellType(const string& name)
{
    unordered_map<string, CellType*>::iterator mi = name_celltypes.find(name);
    if (mi == name_celltypes.end()) {
        return nullptr;
    }
    return mi->second;
}

Cell* Database::getCell(const string& name) {
    unordered_map<string, Cell*>::iterator mi = name_cells.find(name);
    if (mi == name_cells.end()) {
        return nullptr;
    }
    return mi->second;
}

Net* Database::getNet(const string& name) {
    unordered_map<string, Net*>::iterator mi = name_nets.find(name);
    if (mi == name_nets.end()) {
        return nullptr;
    }
    return mi->second;
}

Net* Database::getNetHint(const string& name) {
    for (const auto & [ netname, net ] : name_nets) {
        if (name.find(netname)) {
            continue;
        } else if (name.size() == netname.size()) {
            return net;
        } else if (name[netname.size()] == '_') {
            return net;
        }
    }
    return nullptr;
}

SplitNet* Database::getSplitNet(const string& name)
{
    unordered_map<string, SplitNet*>::iterator mi = name_splitNets.find(name);
    if (mi == name_splitNets.end()) {
        return nullptr;
    }
    return mi->second;
}

Region* Database::getRegion(const string& name)
{
    for (Region* region : regions) {
        if (region->name() == name) {
            return region;
        }
    }
    return nullptr;
}

Region* Database::getRegion(const unsigned char id)
{
    if (id == Region::InvalidRegion) {
        return nullptr;
    }
    return regions[(int)id];
}

NDR* Database::getNDR(const string& name) const
{
    map<string, NDR*>::const_iterator mi = ndrs.find(name);
    if (mi == ndrs.end()) {
        return nullptr;
    }
    return mi->second;
}

IOPin* Database::getIOPin(const string& name) const
{
    unordered_map<string, IOPin*>::const_iterator mi = name_iopins.find(name);
    if (mi == name_iopins.end()) {
        return nullptr;
    }
    return mi->second;
}

Pin* Database::getPinFromConnection(const string& instance, const string& pinname) {
    if (instance == "PIN") {
        IOPin* iopin = getIOPin(pinname);
        if (iopin) return iopin->pin;
        printlog(LOG_ERROR, "IO pin is not found: %s", pinname.c_str());
        return nullptr;
    }
    Cell* cell = getCell(instance);
    if (!cell) {
        printlog(LOG_ERROR, "Cell is not found: %s", instance.c_str());
        return nullptr;
    }
    Pin* pin = cell->pin(pinname);
    if (pin) return pin;
    printlog(LOG_ERROR, "Pin is not found: %s", pinname.c_str());
    return nullptr;
}

ViaType* Database::getViaType(const string& name) const
{
    unordered_map<string, ViaType*>::const_iterator mi = name_viatypes.find(name);
    if (mi == name_viatypes.end()) {
        return nullptr;
    }
    return mi->second;
}

