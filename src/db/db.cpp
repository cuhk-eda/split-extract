#include "db.h"
#include "../global.h"
#include "../io/io.h"
#include "../sta/sta.h"

#include <future>

using namespace db;

Database database;

/***** Database *****/
Database::Database() {
    clear();
    _buffer = new char[_bufferCapacity];
}

Database::~Database() {
    delete[] _buffer;
    _buffer = nullptr;
    clear();
    // for regions.push_back(new Region("default"));
    CLEAR_POINTER_LIST(regions);
}

void Database::clear() {
    clearDesign();
    clearLibrary();
    clearTechnology();
}

void Database::clearTechnology() {
    CLEAR_POINTER_LIST(layers);
    CLEAR_POINTER_LIST(viatypes);
    CLEAR_POINTER_LIST(layers);
    CLEAR_POINTER_MAP(ndrs);
}

void Database::clearDesign() {
    CLEAR_POINTER_LIST(cells);
    CLEAR_POINTER_LIST(iopins);
    CLEAR_POINTER_LIST(nets);
    CLEAR_POINTER_LIST(rows);
    CLEAR_POINTER_LIST(regions);
    CLEAR_POINTER_LIST(snets);

    DBU_Micron = -1.0;
    designName = "";
    _placements.resize(1);
    _activePlacement = 0;
    regions.push_back(new Region("default"));
}

long long Database::getCellArea(Region* region) const {
    long long cellArea = 0;
    for (const Cell* cell : cells) {
        if (region && cell->region != region) {
            continue;
        }
        int w = cell->width() / siteW;
        int h = cell->height() / siteH;
        cellArea += w * h;
    }
    return cellArea;
}

long long Database::getFreeArea(Region* region) const {
    unsigned nRegions = getNumRegions();
    long long freeArea = 0;
    for (unsigned i = 0; i != nRegions; ++i) {
        if (region && region != regions[i]) {
            continue;
        }
        freeArea += siteMap.nRegionSites[i];
    }
    return freeArea;
}

bool Database::placed() {
    for (Cell* cell : cells) {
        if (!cell->placed()) {
            return false;
        }
    }
    return true;
}

bool Database::detailedRouted() {
    for (Net* net : nets) {
        if (!net->detailedRouted()) {
            return false;
        }
    }
    return true;
}

bool Database::globalRouted() {
    for (Net* net : nets) {
        if (!net->globalRouted()) {
            return false;
        }
    }
    return true;
}

void Database::setupSplitNets() {
    for (Net* net : nets) {
        unsigned numComps = 0;
        for (unsigned i = 0; i != net->nodes.size(); ++i) {
            queue<unsigned> q;
            q.push(i);
            while (q.size()) {
                NetRouteNode& node = net->nodes[q.front()];
                q.pop();
                if (node.layer()->rIdx >= static_cast<int>(DBModule::Metal) || node.comp() >= 0) continue;
                node.comp(numComps);
                for (const int adj : node.adjs) {
                    q.push(net->segments[adj].fromNode);
                    q.push(net->segments[adj].toNode);
                }
            }
            if (net->nodes[i].comp() == static_cast<int>(numComps)) ++numComps;
        }
        for (unsigned i = 0; i != numComps; ++i) {
            const string& name = net->name() + "_split_" + to_string(i);
            SplitNet* splitNet = addSplitNet(name, net->ndr, net);
            for (unsigned j = 0; j != net->nodes.size(); ++j) {
                NetRouteNode& node = net->nodes[j];
                if (node.comp() != static_cast<int>(i)) continue;

                Pin* pin = node.pin();
                if (pin) {
                    pin->splitNet(splitNet);
                    splitNet->addPin(pin);
                }
                splitNet->addNode(node, false);
                for (const unsigned adj : node.adjs) {
                    const NetRouteSegment& segment = net->segments[adj];
                    if (net->nodes[segment.fromNode].comp() == -1 || net->nodes[segment.toNode].comp() == -1) {
                        splitNet->addUpVia(node);
                        continue;
                    }
                    const unsigned toNodeIdx = min(segment.fromNode, segment.toNode);
                    if (toNodeIdx == j) continue;

                    const NetRouteNode& toNode = net->nodes[toNodeIdx];
                    splitNet->addWire(node, toNode, segment.width());
                }
            }
            for (NetRouteUpNode& node : splitNet->upVias) node.setDir(splitNet);
        }
    }

    const Layer* splitLayer = nullptr;
    for (const Layer* layer : layers) {
        if (layer->rIdx + 1 == static_cast<int>(DBModule::Metal)) {
            splitLayer = layer;
            break;
        }
    }

    for (IOPin* iopin : iopins) {
        if (iopin->pin->splitNet() || !iopin->pin->net() || !iopin->pin->net()->len()) continue;

        addSplitNet(iopin->pin, splitLayer);
    }

    for (const Cell* cell : cells) {
        for (unsigned i = 0; i != cell->numPins(); ++i) {
            Pin* pin = cell->pin(i);
            switch (pin->type->use()) {
                case Use::UseEnum::Ground:
                case Use::UseEnum::Power:
                    continue;
                default:
                    break;
            }
            if (pin->splitNet() || !pin->net() || !pin->net()->len()) continue;

            addSplitNet(pin, splitLayer);
        }
    }

    printlog(LOG_INFO,
             "default capacitance is %f default max capacitance is %f",
             sta::STALibraryIPin::default_capacitance,
             sta::STALibraryOPin::default_max_capacitance);
    if (sta::STALibraryIPin::default_capacitance && sta::STALibraryOPin::default_max_capacitance) {
        for (SplitNet* splitNet : splitNets)
            splitNet->setCapacitance(sta::STALibraryIPin::default_capacitance,
                                     sta::STALibraryOPin::default_max_capacitance);
    }
    for (SplitNet* splitNet : splitNets) {
        splitNet->eraseBackslashInName();
    }
}

unsigned Database::setupImage(const unsigned imgIdx) {
    unsigned ret = 0;
    while (true) {
        unsigned idx = 0;
        {
            lock_guard<mutex> lock(snIdxMtx);
            idx = snIdx;
            ++snIdx;
        }
        if (idx >= splitNets.size()) return ret;
        images[imgIdx].setRouting(splitNets[idx], 0);
        ++ret;
    }
}

void Database::setupImages() {
    double xOrigin = 0;
    double yOrigin = 0;
    double xStep = 1;
    double yStep = 1;
    unsigned xNum = 0;
    unsigned yNum = 0;
    for (const Layer* layer : layers) {
        if (layer->rIdx == static_cast<int>(DBModule::Metal) || layer->rIdx + 1 == static_cast<int>(DBModule::Metal)) {
            switch (layer->direction) {
                case 'h': {
                    yStep = layer->pitch / 4.0;
                    const double start = layer->track.start - yStep / 2;
                    yOrigin = start - ceil((start - ly()) / yStep) * yStep;
                    yNum = static_cast<int>(ceil((hy() - yOrigin) / yStep) + 0.5);
                    break;
                }
                case 'v': {
                    xStep = layer->pitch / 4.0;
                    const double start = layer->track.start - xStep / 2;
                    xOrigin = start - ceil((start - lx()) / xStep) * xStep;
                    xNum = static_cast<int>(ceil((hx() - xOrigin) / xStep) + 0.5);
                    break;
                }
                default:
                    printlog(
                        LOG_ERROR, "unidentified layer direction %c in %s : %d", layer->direction, __FILE__, __LINE__);
                    return;
            }
        }
    }
    images.resize(DBModule::CPU, Image(xOrigin, yOrigin, xStep, yStep, xNum, yNum));
    vector<future<unsigned>> futs;
    snIdx = 0;
    for (unsigned i = 0; i != DBModule::CPU; ++i) futs.push_back(async(&Database::setupImage, this, i));
    for (future<unsigned>& fut : futs) fut.get();
    for (unsigned i = 1; i < DBModule::CPU; ++i) images[0] += images[i];
    images.resize(1);
}

void Database::setupGraph() {
    if (splitNets.empty()) {
        printlog(LOG_ERROR, "no split net %s : %d", __FILE__, __LINE__);
        return;
    }

    vector<SplitNet*> cleanSplitNets;
    for (SplitNet* splitNet : splitNets) {
        if (splitNet->upVias.empty() || (!splitNet->isSource() && !splitNet->isSink())) continue;

        cleanSplitNets.push_back(splitNet);
    }

    if (cleanSplitNets.empty()) {
        printlog(LOG_ERROR, "no clean split net %s : %d", __FILE__, __LINE__);
        return;
    }

    unsigned dir = 0;
    Point pitch;
    for (const Layer* layer : layers) {
        if (layer->rIdx + 1 == static_cast<int>(DBModule::Metal)) {
            switch (layer->direction) {
                case 'h':
                    dir = 0;
                    pitch.y(layer->pitch);
                    break;
                case 'v':
                    dir = 1;
                    pitch.x(layer->pitch);
                    break;
                default:
                    printlog(
                        LOG_ERROR, "unidentified layer direction %c in %s : %d", layer->direction, __FILE__, __LINE__);
                    return;
            }
        } else if (layer->rIdx == static_cast<int>(DBModule::Metal)) {
            switch (layer->direction) {
                case 'h':
                    pitch.y(layer->pitch);
                    break;
                case 'v':
                    pitch.x(layer->pitch);
                    break;
                default:
                    printlog(
                        LOG_ERROR, "unidentified layer direction %c in %s : %d", layer->direction, __FILE__, __LINE__);
                    return;
            }
        }
    }
    graph.run(cleanSplitNets, images[0], *this, pitch, dir, io::IOModule::DefPlacement);
}

void Database::setup() {
    setupSplitNets();
    setupImages();
    if (io::IOModule::NetDetail.length()) database.readNetDetail(io::IOModule::NetDetail);
    if (io::IOModule::TimePath.length()) database.readTimePath(io::IOModule::TimePath, true);
    if (io::IOModule::TimeUnconstrain.length()) database.readTimePath(io::IOModule::TimeUnconstrain, false);
    setupGraph();
}

long long Database::getHPWL() {
    long long hpwl = 0;
    int nNets = getNumNets();
    for (int i = 0; i < nNets; i++) {
        int nPins = nets[i]->pins.size();
        if (nPins < 2) {
            continue;
        }
        int lx = INT_MAX;
        int ly = INT_MAX;
        int hx = INT_MIN;
        int hy = INT_MIN;
        for (int j = 0; j < nPins; j++) {
            const Pin* pin = nets[i]->pins[j];
            int x, y;
            pin->getPinCenter(x, y);
            lx = min(lx, x);
            ly = min(ly, y);
            hx = max(hx, x);
            hy = max(hy, y);
        }
        hpwl += (hx - lx) + (hy - ly);
    }
    return hpwl;
}

string DBModule::_name = "db";

unsigned DBModule::CPU = 1;
bool DBModule::EdgeSpacing = true;
bool DBModule::EnableFence = true;
bool DBModule::EnablePG = true;
unsigned DBModule::Metal = 0;
unsigned DBModule::NumCands = 31;

void DBModule::showOptions() {
    printlog(LOG_INFO, "cpu             : %u", CPU);
    printlog(LOG_INFO, "edgeSpacing     : %s", EdgeSpacing ? "true" : "false");
    printlog(LOG_INFO, "enableFence     : %s", EnableFence ? "true" : "false");
    printlog(LOG_INFO, "enablePG        : %s", EnablePG ? "true" : "false");
    printlog(LOG_INFO, "metal           : %u", Metal);
    printlog(LOG_INFO, "numCands        : %u", NumCands);
}

bool DBModule::setup() {
    showOptions();
    database.setup();
    return true;
}
