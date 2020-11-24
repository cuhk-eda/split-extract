#include "db.h"
using namespace db;

#include "../ut/utils.h"

/***** NetRouteNode *****/

void NetRouteNode::shift(const int dx, const int dy) {
    _x += dx;
    _y += dy;
}

/***** NetRouteUpNode *****/

bool NetRouteUpNode::dirnp(const unsigned dir) const {
    switch (dir) {
        case 0:
            return _dirnp;
        case 1:
            return _dirpn;
        default:
            printlog(LOG_ERROR, "unidentified dir %d", dir);
            return true;
    }
}

void NetRouteUpNode::setDir(const Net* net) {
    if (net->len()) {
        NetRouteNode viann = *this;
        NetRouteNode vianp = *this;
        NetRouteNode viapn = *this;
        NetRouteNode viapp = *this;
        viann.shift(-1, -1);
        vianp.shift(-1, 1);
        viapn.shift(1, -1);
        viapp.shift(1, 1);
        const vector<tuple<int, int, int, char>>& hp = net->halfPlane(*this);
        _dirnn = Net::isDirectNet(viann, hp);
        _dirnp = Net::isDirectNet(vianp, hp);
        _dirpn = Net::isDirectNet(viapn, hp);
        _dirpp = Net::isDirectNet(viapp, hp);
    }
}

/***** NetRouting *****/

void NetRouting::addNode(const NetRouteNode& n, const bool updatePin) {
    for (const NetRouteNode& node : nodes) {
        if (node == n) {
            return;
        }
    }
    nodes.push_back(n);
    if (updatePin) {
        const int rIdx = n.layer()->rIdx;
        if (rIdx < 0 || rIdx >= static_cast<int>(rtrees.size())) {
            printlog(LOG_ERROR,
                     "From node ( %s %d %d ) above rtrees in net %s",
                     n.layer()->name().c_str(),
                     n.x(),
                     n.y(),
                     _name.c_str());
        } else {
            const boostBox box({n.x(), n.y()}, {n.x(), n.y()});
            vector<pair<boostBox, Pin*>> results;
            rtrees[n.layer()->rIdx].query(bg::index::intersects(box), back_inserter(results));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
            for (const auto& [box, pin] : results) {
#pragma GCC diagnostic pop
                nodes.back().pin(pin);
                if (pin->nrn().layer()) {
                    addWire(pin->nrn(), n, -1, 'P');
                } else {
                    pin->nrn(n);
                }
                break;
            }
        }
    }
    unionWith(n);
}

void NetRouting::addWire(const NetRouteNode& fromNode, const NetRouteNode& toNode, const int width, const char dir) {
    int fromi = -1;
    int toi = -1;
    for (unsigned i = 0; i != nodes.size(); ++i) {
        if (nodes[i] == fromNode) {
            fromi = i;
        }
        if (nodes[i] == toNode) {
            toi = i;
        }
    }

    if (fromi < 0) {
        printlog(LOG_ERROR,
                 "From node ( %s %d %d ) is not found in net %s",
                 fromNode.layer()->name().c_str(),
                 fromNode.x(),
                 fromNode.y(),
                 _name.c_str());
        return;
    }

    if (toi < 0) {
        printlog(LOG_ERROR,
                 "To node ( %s %d %d ) is not found in net %s",
                 toNode.layer()->name().c_str(),
                 toNode.x(),
                 toNode.y(),
                 _name.c_str());
        return;
    }

    if (fromi == toi) return;

    char d = dir;
    unsigned len = 0;
    if (!d) {
        if (fromNode.layer()->rIdx > toNode.layer()->rIdx) {
            d = 'D';
            len = 0;
            ++_numVias;
            ++nVias[toNode.layer()->rIdx];
        } else if (fromNode.layer()->rIdx < toNode.layer()->rIdx) {
            d = 'U';
            len = 0;
            ++_numVias;
            ++nVias[fromNode.layer()->rIdx];
        } else {
            if (fromNode.x() < toNode.x()) {
                d = 'E';
                len = toNode.x() - fromNode.x();
            } else if (fromNode.x() > toNode.x()) {
                d = 'W';
                len = fromNode.x() - toNode.x();
            } else if (fromNode.y() < toNode.y()) {
                d = 'N';
                len = toNode.y() - fromNode.y();
            } else if (fromNode.y() > toNode.y()) {
                d = 'S';
                len = fromNode.y() - toNode.y();
            }
            _len += len;
            lens[fromNode.layer()->rIdx] += len;
            const double pitchLen = len / static_cast<double>(fromNode.layer()->pitch);
            _pitchLen += pitchLen;
            pitchLens[fromNode.layer()->rIdx] += pitchLen;
        }
    }
    segments.emplace_back(fromi, toi, d, len, width);
    const unsigned adj = segments.size() - 1;
    nodes[fromi].adjs.push_back(adj);
    nodes[toi].adjs.push_back(adj);
}

unsigned NetRouting::len(const int rIdx) const {
    if (rIdx < 0) return _len;
    return lens[rIdx];
}

double NetRouting::pitchLen(const int rIdx) const {
    if (rIdx < 0) return _pitchLen;
    return pitchLens[rIdx];
}

unsigned NetRouting::numVias(const int cIdx) const {
    if (cIdx < 0) return _numVias;
    return nVias[cIdx];
}

bool NetRouting::hasNodeOnLayer(const Layer* layer) const {
    for (const NetRouteNode& node : nodes) {
        if (node.layer()->rIdx >= layer->rIdx) return true;
    }
    return false;
}

unsigned NetRouting::manhattanDistance(const NetRouting* m, const NetRouting* n, const Layer* layer) {
    unsigned minDistance = UINT_MAX;
    unsigned tempDistance = 0;
    if (layer) {
        for (const auto& nodeM : m->nodes) {
            if (nodeM.layer()->rIdx >= layer->rIdx) {
                for (const auto& nodeN : n->nodes) {
                    if (nodeN.layer()->rIdx >= layer->rIdx) {
                        tempDistance = abs(nodeM.x() - nodeN.x()) + abs(nodeM.y() - nodeN.y());
                        if (minDistance > tempDistance) {
                            minDistance = tempDistance;
                        }
                    }
                }
            }
        }
    } else {
        for (const auto& nodeM : m->nodes) {
            for (const auto& nodeN : n->nodes) {
                tempDistance = abs(nodeM.x() - nodeN.x()) + abs(nodeM.y() - nodeN.y());
                if (minDistance > tempDistance) {
                    minDistance = tempDistance;
                }
            }
        }
    }
    return minDistance;
}

/***** Net *****/

void Net::addPin(Pin* pin) {
    for (const Pin* p : pins) {
        if (p == pin) return;
    }
    pins.push_back(pin);
    switch (pin->type->direction()) {
        case 'i':
            ++_numOPins;
            break;
        case 'o':
            _iPin = pin;
            break;
        case 'p':
            break;
        default:
            printlog(
                LOG_WARN, "Direction %c is not identified for pin %s", pin->type->direction(), pin->name().c_str());
            break;
    }
    const IOPin* iopin = pin->iopin;
    if (iopin) {
        ++_numIOPins;
        const vector<Geometry>& shapes = iopin->type->shapes;
        if (shapes.empty()) {
            printlog(LOG_WARN, "IOPin %s has no shape", iopin->name.c_str());
        } else if (shapes[0].layer->rIdx >= static_cast<int>(DBModule::Metal)) {
            _upPin = pin;
        }
    }
    if (rtrees.empty()) return;
    for (Geometry geo : pin->type->shapes) {
        const int rIdx = geo.layer->rIdx;
        if (rIdx < 0 || rIdx >= static_cast<int>(rtrees.size())) continue;
        if (pin->cell) {
            pin->cell->transform(geo);
        } else if (iopin) {
            iopin->transform(geo);
        } else {
            printlog(LOG_ERROR, "invalid pin %s:%d", __FILE__, __LINE__);
        }
        rtrees[rIdx].insert({{{geo.lx(), geo.ly()}, {geo.hx(), geo.hy()}}, pin});
    }
}

vector<NetRouteNode> Net::getNodes(const Layer* layer) const {
    std::vector<NetRouteNode> temp;
    for (const NetRouteNode& node : nodes) {
        if (node.layer()->rIdx == layer->rIdx) temp.push_back(node);
    }
    return temp;
}

int Net::getPinId(const Pin* pin) const {
    for (unsigned i = 0; i != pins.size(); ++i) {
        if (pin == pins[i]) {
            return i;
        }
    }
    return -1;
}

double Net::setCapacitance(const double default_capacitance, const double default_max_capacitance) {
    _capacitance = 0;

    if (_iPin) {
        for (const Pin* pin : pins) {
            const PinType* type = pin->type;
            switch (type->direction()) {
                case 'i':
                    _capacitance -= type->capacitance();
                    continue;
                case 'o':
                    _capacitance += type->capacitance();
                    continue;
                default:
                    printlog(LOG_WARN, "pin type %s direction is %c", type->direction());
                    continue;
            }
        }
        return _capacitance;
    } else if (_numOPins) {
        for (const Pin* pin : pins) {
            const PinType* type = pin->type;
            if (type->direction() == 'i') {
                _capacitance += type->capacitance();
            }
        }
        if (_capacitance <= 0)
            printlog(LOG_ERROR,
                     "Sink net %s has infeasible cap %f with default cap %f",
                     name().c_str(),
                     _capacitance,
                     default_capacitance);
        return _capacitance;
    }

    return 0;
}

int Net::iArea() const {
    if (!_iPin) return 0;
    const Cell* cell = _iPin->cell;
    if (cell) return cell->width() * cell->height();
    const IOPin* iopin = _iPin->iopin;
    if (iopin) {
        int ret = 0;
        for (const Geometry& geo : iopin->type->shapes) ret += (geo.w() * geo.h());
        return ret;
    }
    printlog(LOG_WARN, "pin %s neither cell pin nor io pin in %s", _iPin->type->name().c_str(), __func__);
    return 0;
}

int Net::oArea() const {
    int ret = 0;
    for (const Pin* pin : pins) {
        const char dir = pin->type->direction();
        switch (dir) {
            case 'i':
                break;
            case 'o':
                continue;
            default:
                printlog(LOG_WARN, "pin type dir %c in %s", dir, __func__);
                continue;
        }
        const Cell* cell = pin->cell;
        if (cell) {
            ret += (cell->width() * cell->height());
            continue;
        }
        const IOPin* iopin = pin->iopin;
        if (iopin) {
            for (const Geometry& geo : iopin->type->shapes) ret += (geo.w() * geo.h());
            continue;
        }
        printlog(LOG_WARN, "pin %s neither cell pin nor io pin in %s", pin->type->name().c_str(), __func__);
    }
    return ret;
}

const Point Net::meanPin() const {
    double retx = 0;
    double rety = 0;
    for (const Pin* pin : pins) {
        retx += pin->type->cx();
        rety += pin->type->cy();
        const Cell* cell = pin->cell;
        if (cell) {
            retx += cell->lx();
            rety += cell->ly();
            continue;
        }
        const IOPin* iopin = pin->iopin;
        if (iopin) {
            retx += iopin->x;
            rety += iopin->y;
            continue;
        }
        printlog(LOG_WARN, "pin %s neither cell pin nor io pin in %s", pin->type->name().c_str(), __func__);
    }
    return {static_cast<int>(retx / pins.size()), static_cast<int>(rety / pins.size())};
}

vector<tuple<int, int, int, char>> Net::halfPlane(const NetRouteNode& upvia) const {
    vector<tuple<int, int, int, char>> planeSet;
    for (const NetRouteSegment& segment : segments) {
        const char dir = segment.dir();
        if (dir == 'U' || dir == 'D') {
            planeSet.emplace_back(0, 0, 0, 'E');
            continue;
        }
        char direction = '\0';
        bool startPoint = false;
        const NetRouteNode& fmNode = nodes[segment.fromNode];
        const NetRouteNode& toNode = nodes[segment.toNode];
        if (upvia.x() == fmNode.x() && upvia.y() == fmNode.y()) {
            direction = dir;
            startPoint = true;
        } else if (upvia.x() == toNode.x() && upvia.y() == toNode.y()) {
            direction = dir;
            startPoint = false;
        } else {
            continue;
        }
        if ((startPoint && direction == 'N') || (!startPoint && direction == 'S')) {
            planeSet.emplace_back(0, 1, -upvia.y(), 'L');
        } else if ((startPoint && direction == 'S') || (!startPoint && direction == 'N')) {
            planeSet.emplace_back(0, 1, -upvia.y(), 'G');
        } else if ((startPoint && direction == 'E') || (!startPoint && direction == 'W')) {
            planeSet.emplace_back(1, 0, -upvia.x(), 'L');
        } else if ((startPoint && direction == 'W') || (!startPoint && direction == 'E')) {
            planeSet.emplace_back(1, 0, -upvia.x(), 'G');
        }
    }
    return planeSet;
}

bool Net::isDirectNet(const NetRouteNode& upvia, const vector<tuple<int, int, int, char>>& planeSet) {
    for (const tuple<int, int, int, char>& temp : planeSet) {
        /*
        if (upvia.x == 40470 && upvia.y == 18340 && get<2>(temp) == -14140 && get<3>(temp) != 'E') {
            printlog(LOG_INFO, "%d * %d + %d * %d + %d %c=", get<0>(temp), upvia.x,get<1>(temp), upvia.y, get<2>(temp),
        get<3>(temp));
        }
        */
        switch (get<3>(temp)) {
            case 'L':
                if (get<0>(temp) * upvia.x() + get<1>(temp) * upvia.y() + get<2>(temp) <= 0) {
                    return true;
                }
                break;
            case 'G':
                if (get<0>(temp) * upvia.x() + get<1>(temp) * upvia.y() + get<2>(temp) >= 0) {
                    return true;
                }
                break;
            case 'E':
                break;
                //  return true;
            default:
                printlog(LOG_ERROR, "plane set 3rd element %c not identified", get<3>(temp));
                break;
        }
    }
    return false;
}

bool Net::isDirection(const NetRouteUpNode& upviaM, const NetRouteUpNode& upviaN) {
    if (upviaM.x() <= upviaN.x()) {
        if (upviaM.y() <= upviaN.y()) {
            if (upviaM.dirpp() || upviaN.dirnn()) return true;
        }
        if (upviaM.y() >= upviaN.y()) {
            if (upviaM.dirnp(1) || upviaN.dirnp(0)) return true;
        }
    }
    if (upviaM.x() >= upviaN.x()) {
        if (upviaM.y() <= upviaN.y()) {
            if (upviaM.dirnp(0) || upviaN.dirnp(1)) return true;
        }
        if (upviaM.y() >= upviaN.y()) {
            if (upviaM.dirnn() || upviaN.dirpp()) return true;
        }
    }
    return false;
}

bool Net::isLoop(Net* inet, const Net* onet) {
    for (const Pin* pin : onet->pins) {
        if (Pin::isLoop(pin, inet->iPin())) return true;
    }
    return false;
}

/***** SplitNet *****/

void SplitNet::addUpVia(const NetRouteNode& n) {
    for (const NetRouteNode& node : upVias) {
        if (node == n) {
            return;
        }
    }
    upVias.push_back(n);
}

void SplitNet::write(
    ostream& os, const string& design, const Rectangle& die, const Point& pitch, const unsigned dir) const {
    for (unsigned viaIdx = 0; viaIdx != upVias.size(); ++viaIdx) {
        const NetRouteUpNode& via = upVias[viaIdx];
        os << design << ',' << _parent->name() << ',' << _name << ',' << viaIdx << ','
           << via[dir] / static_cast<double>(pitch[dir]) << ',' << via[1 - dir] / static_cast<double>(pitch[1 - dir])
           << ',' << (via[dir] - die[dir].lo()) / static_cast<double>(die[dir].range()) << ','
           << (via[1 - dir] - die[1 - dir].lo()) / static_cast<double>(die[1 - dir].range()) << ',' << via.dirnn()
           << ',' << via.dirnp(dir) << ',' << via.dirnp(1 - dir) << ',' << via.dirpp() << ',' << _numOPins << ',';
        if (_upPin) {
            os << "1,";
        } else {
            os << "0,";
        }
        os << pitchLen() << ',' << numVias() << ',' << pitchLen(DBModule::Metal - 1);
        for (int i = static_cast<int>(DBModule::Metal) - 2; i != static_cast<int>(DBModule::Metal) - 5; --i) {
            if (i >= 0) {
                os << ',' << numVias(i) << ',' << pitchLen(i);
            } else {
                os << ',' << 0 << ',' << 0;
            }
        }
        os << endl;
    }
}

bool SplitNet::isSeparate(const SplitNet* m, const SplitNet* n) {
    if ((!m) || (!n)) {
        printlog(LOG_WARN, "nullptr split net in %s", __func__);
        return true;
    }
    if (!(m->_parent)) {
        printlog(LOG_WARN, "nullptr parent net of %s", m->name().c_str());
        return true;
    }
    if (!(n->_parent)) {
        printlog(LOG_WARN, "nullptr parent net of %s", n->name().c_str());
        return true;
    }
    return !Rectangle::hasIntersect(*m->_parent, *n->_parent);
}
