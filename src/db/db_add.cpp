#include "db.h"
using namespace db;

#include "../ut/utils.h"

Layer* Database::addLayer(const string& name, const char type) {
    Layer* newlayer = new Layer(name, type);
    if (layers.size() > 0) {
        Layer* oldlayer = layers[layers.size() - 1];
        oldlayer->_above = newlayer;
        newlayer->_below = oldlayer;
        if (type == 'r') {
            newlayer->rIdx = oldlayer->cIdx + 1;
        } else {
            newlayer->cIdx = oldlayer->rIdx;
        }
    } else {
        newlayer->_below = nullptr;
        if (type == 'r') {
            newlayer->rIdx = 0;
        } else {
            newlayer->cIdx = 0;
        }
    }
    layers.push_back(newlayer);
    if (type == 'r') ++nLayers;
    return newlayer;
}

ViaType* Database::addViaType(const string& name, bool isDef) {
    ViaType* viatype = getViaType(name);
    if (viatype) {
        printlog(LOG_WARN, "via type re-defined: %s", name.c_str());
        return viatype;
    }
    viatype = new ViaType(name, isDef);

#ifdef _GNUC_4_8_
    name_viatypes.emplace(name, viatype);
#else
    name_viatypes[name] = viatype;
#endif

    viatypes.push_back(viatype);
    return viatype;
}

CellType* Database::addCellType(const string& name, unsigned libcell) {
    CellType* celltype = getCellType(name);
    if (celltype) {
        printlog(LOG_WARN, "cell type re-defined: %s", name.c_str());
        return celltype;
    }
    celltype = new CellType(name, libcell);
#ifdef _GNUC_4_8_
    name_celltypes.emplace(name, celltype);
#else
    name_celltypes[name] = celltype;
#endif
    celltypes.push_back(celltype);
    return celltype;
}

Cell* Database::addCell(const string& name, CellType* type) {
    Cell* cell = getCell(name);
    if (cell) {
        printlog(LOG_WARN, "cell re-defined: %s", name.c_str());
        if (!cell->ctype()) {
            cell->ctype(type);
        }
        return cell;
    }
    cell = new Cell(name, type);
#ifdef _GNUC_4_8_
    name_cells.emplace(name, cell);
#else
    name_cells[name] = cell;
#endif
    cells.push_back(cell);
    return cell;
}

IOPin* Database::addIOPin(const string& name, const string& netName, const char direction) {
    IOPin* iopin = getIOPin(name);
    if (iopin) {
        printlog(LOG_WARN, "IO pin re-defined: %s", name.c_str());
        return iopin;
    }
    iopin = new IOPin(name, netName, direction);
#ifdef _GNUC_4_8_
    name_iopins.emplace(name, iopin);
#else
    name_iopins[name] = iopin;
#endif
    iopins.push_back(iopin);
    return iopin;
}

Net* Database::addNet(const string& name, const NDR* ndr, const unsigned nLayers) {
    string n = name;
    replace(n.begin(), n.end(), '/', '_');
    Net* net = getNet(n);
    if (net) {
        printlog(LOG_WARN, "Net re-defined: %s", n.c_str());
        return net;
    }
    net = new Net(n, ndr, Use::UseEnum::Signal, nLayers);
#ifdef _GNUC_4_8_
    name_nets.emplace(n, net);
#else
    name_nets[n] = net;
#endif
    nets.push_back(net);
    return net;
}

SplitNet* Database::addSplitNet(const string& name, const NDR* ndr, const Net* net) {
    SplitNet* splitNet = getSplitNet(name);
    if (splitNet) {
        printlog(LOG_WARN, "Split Net re-defined: %s", name.c_str());
        return splitNet;
    }
    if (!net) net = getNetHint(name);
    if (!net) {
        printlog(LOG_ERROR, "Parent Net non-defined: %s", name.c_str());
        return nullptr;
    }
    splitNet = new SplitNet(name, ndr, net);
    name_splitNets[name] = splitNet;
    splitNets.push_back(splitNet);
    return splitNet;
}

SplitNet* Database::addSplitNet(Pin* pin, const Layer* splitLayer) {
    string name;
    if (pin->cell) {
        name = pin->cell->name() + "_" + pin->type->name();
    } else if (pin->iopin) {
        name = pin->iopin->name;
    } else {
        printlog(LOG_ERROR, "invalid pin %s:%d", __FILE__, __LINE__);
    }

    SplitNet* splitNet = getSplitNet(name);
    if (splitNet) {
        printlog(LOG_WARN, "Split Net re-defined: %s", name.c_str());
        return splitNet;
    }

    splitNet = new SplitNet(name, nullptr, pin->net());
    pin->splitNet(splitNet);
    splitNet->addPin(pin);
    int x = INT_MIN;
    int y = INT_MIN;
    pin->getPinCenter(x, y);

    const NetRouteNode nrn(splitLayer, x, y, -1);
    splitNet->addNode(nrn, false);
    splitNet->addUpVia(nrn);

    name_splitNets[name] = splitNet;
    splitNets.push_back(splitNet);

    return splitNet;
}

Row* Database::addRow(const string& name,
                      const string& macro,
                      const int x,
                      const int y,
                      const unsigned xNum,
                      const unsigned yNum,
                      const bool flip,
                      const unsigned xStep,
                      const unsigned yStep) {
    Row* newrow = new Row(name, macro, x, y, xNum, yNum, flip, xStep, yStep);
    rows.push_back(newrow);
    return newrow;
}

Region* Database::addRegion(const string& name, const char type) {
    Region* region = getRegion(name);
    if (region) {
        printlog(LOG_WARN, "Region re-defined: %s", name.c_str());
        return region;
    }
    region = new Region(name, type);
    regions.push_back(region);
    return region;
}

NDR* Database::addNDR(const string& name, const bool hardSpacing) {
    NDR* ndr = getNDR(name);
    if (ndr) {
        printlog(LOG_WARN, "NDR re-defined: %s", name.c_str());
        return ndr;
    }
    ndr = new NDR(name, hardSpacing);
    ndrs.emplace(name, ndr);
    return ndr;
}

Net* Database::addSNet(const string& name, const Use::UseEnum use) {
    Net* newsnet = new Net(name, nullptr, use, 0);
    snets.push_back(newsnet);
    return newsnet;
}
