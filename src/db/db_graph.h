#ifndef _DB_GRAPH_H_
#define _DB_GRAPH_H_

#include "../io/io.h"

namespace db {

template <typename T>
class GraphArc {
private:
    const T* _onet = nullptr;
    const T* _inet = nullptr;
    unsigned _onodeIdx = 0;
    unsigned _inodeIdx = 0;
    double _cost = 0;

public:
    GraphArc(const T* onet, const T* inet, const unsigned onodeIdx, const unsigned inodeIdx, const double cost)
        : _onet(onet), _inet(inet), _onodeIdx(onodeIdx), _inodeIdx(inodeIdx), _cost(cost) {}

    const T* inet() const { return _inet; }
    double cost() const { return _cost; }

    unsigned write(
        ostream& os, const string& design, const Rectangle& die, const Point& pitch, const unsigned dir) const;

    bool operator<(const GraphArc<T>& rhs) { return _cost < rhs._cost; }
};

template <typename T>
unsigned GraphArc<T>::write(
    ostream& os, const string& design, const Rectangle& die, const Point& pitch, const unsigned dir) const {
    const NetRouteUpNode& onode = _onet->upVias[_onodeIdx];
    const NetRouteUpNode& inode = _inet->upVias[_inodeIdx];
    const int dx = onode[dir] - inode[dir];
    const int dy = onode[1 - dir] - inode[1 - dir];
    const double px = dx / static_cast<double>(pitch[dir]);
    const double py = dy / static_cast<double>(pitch[1 - dir]);
    const double rx = dx / static_cast<double>(die[dir].range());
    const double ry = dy / static_cast<double>(die[1 - dir].range());
    const unsigned numOPins = _onet->numOPins();
    os << _onet->name() << ',' << _inet->name() << ',' << _onodeIdx << ',' << _inodeIdx << ',' << px << ',' << py << ','
       << abs(px) << ',' << abs(py) << ',' << rx << ',' << ry << ',' << abs(rx) << ',' << abs(ry) << ',' << numOPins
       << ',' << _inet->numOPins() << ',';

    if (_onet->upPin()) {
        os << "1,";
    } else {
        os << "0,";
    }

    if (_inet->upPin()) {
        os << "1,";
    } else {
        os << "0,";
    }

    os << _onet->pitchLen() << ',' << _onet->numVias() << ',' << _onet->pitchLen(DBModule::Metal - 1) << ',';
    for (int i = static_cast<int>(DBModule::Metal) - 2; i != static_cast<int>(DBModule::Metal) - 5; --i) {
        if (i >= 0) {
            os << _onet->numVias(i) << ',' << _onet->pitchLen(i) << ',';
        } else {
            os << 0 << ',' << 0 << ',';
        }
    }
    os << _inet->pitchLen() << ',' << _inet->numVias() << ',' << _inet->pitchLen(DBModule::Metal - 1) << ',';
    for (int i = static_cast<int>(DBModule::Metal) - 2; i != static_cast<int>(DBModule::Metal) - 5; --i) {
        if (i >= 0) {
            os << _inet->numVias(i) << ',' << _inet->pitchLen(i) << ',';
        } else {
            os << 0 << ',' << 0 << ',';
        }
    }

    if (_onet->parent() == _inet->parent()) {
        os << "1\n";
        return numOPins;
    } else {
        os << "0\n";
        return 0;
    }
}

template <typename T>
class Graph {
private:
    static constexpr double INFTY =
        numeric_limits<double>::has_infinity ? numeric_limits<double>::infinity() : numeric_limits<double>::max();

    unsigned _nONodes = 0;
    unsigned _nINodes = 0;
    unsigned _nOPins = 0;

    using CostType = long long;
    using UCostType = unsigned long long;

    vector<T*> _onodes;

public:
    static vector<T*> inodes;
    static mutex selMtx;
    static unsigned totalNumONetsMissed;
    static unsigned nFlows;
    static unsigned nCoveredNets;
    static unsigned nCoveredPins;
    static ofstream selOfs;

private:
    void addNode(T* splitNet);
    static void writeSel(
        const T* onet, const string& design, const Rectangle& die, const Point& pitch, const unsigned dir);
    static void writeImg(
        const T* splitNet, const vector<T*>& splitNets, const Image& image, const string& path, const unsigned dir);

public:
    bool run(const vector<T*>& splitNets,
             const Image& image,
             const Rectangle& die,
             const Point& pitch,
             const unsigned dir,
             const string& file);
};

template <typename T>
vector<T*> Graph<T>::inodes;
template <typename T>
mutex Graph<T>::selMtx;
template <typename T>
unsigned Graph<T>::totalNumONetsMissed;
template <typename T>
unsigned Graph<T>::nFlows;
template <typename T>
unsigned Graph<T>::nCoveredNets;
template <typename T>
unsigned Graph<T>::nCoveredPins;
template <typename T>
ofstream Graph<T>::selOfs;

template <typename T>
void Graph<T>::addNode(T* splitNet) {
    if (splitNet->isSource()) {
        inodes.push_back(splitNet);
        ++_nINodes;
    } else if (splitNet->isSink()) {
        _onodes.push_back(splitNet);
        ++_nONodes;
        _nOPins += splitNet->numOPins();
    }
}

template <typename T>
void Graph<T>::writeSel(
    const T* onet, const string& design, const Rectangle& die, const Point& pitch, const unsigned dir) {
    bool isMissed = true;
    bool isCovered = true;
    vector<GraphArc<T>> arcs;
    for (T* inet : inodes) {
        double icap = 0;
        const bool isLoop = T::isLoop(inet, onet);
        if (!isLoop) {
            unsigned onodeIdx = 0;
            unsigned inodeIdx = 0;
            UCostType cost = ULLONG_MAX;
            bool select = false;
            const int dieW = die[dir].range();
            for (unsigned i = 0; i != onet->upVias.size(); ++i) {
                const NetRouteUpNode& on = onet->upVias[i];
                const int ox = on[dir] - die[dir].lo();
                const bool ob = ox < dieW * 0.08 || ox > dieW * 0.92;
                for (unsigned j = 0; j != inet->upVias.size(); ++j) {
                    const NetRouteUpNode& in = inet->upVias[j];
                    const int ix = in[dir] - die[dir].lo();
                    const bool ib = ix < dieW * 0.08 || ix > dieW * 0.92;
                    const UCostType c = (static_cast<UCostType>(abs(ix - ox)) << 32) + abs(in[1 - dir] - on[1 - dir]);
                    const bool d = ob || ib || Net::isDirection(in, on);
                    if (d && c < cost) {
                        cost = c;
                        select = true;
                        onodeIdx = i;
                        inodeIdx = j;
                    }
                }
            }
            if (select) {
                if (arcs.size() < DBModule::NumCands) {
                    arcs.emplace_back(onet, inet, onodeIdx, inodeIdx, cost);
                    if (arcs.size() == DBModule::NumCands) make_heap(arcs.begin(), arcs.end());
                } else if (cost < arcs.front().cost()) {
                    pop_heap(arcs.begin(), arcs.end());
                    if (onet->parent() == arcs.back().inet()->parent()) {
                        isCovered = false;
                        //  return;
                    }
                    arcs.pop_back();
                    arcs.emplace_back(onet, inet, onodeIdx, inodeIdx, cost);
                    push_heap(arcs.begin(), arcs.end());
                } else if (onet->parent() == inet->parent()) {
                    isCovered = false;
                    //  return;
                }
                icap = 1;
            }
        }
        if (onet->parent() == inet->parent()) {
            if (icap) {
                isMissed = false;
            } else {
                if (isLoop) printlog(LOG_WARN, "onet %s inet %s is loop", onet->name().c_str(), inet->name().c_str());
                isCovered = false;
                //  lock_guard<mutex> lock(selMtx);
                //  ++totalNumONetsMissed;
                //  return;
            }
        }
    }

    lock_guard<mutex> lock(selMtx);
    if (isMissed) {
        ++totalNumONetsMissed;
        //  return;
    }
    if (isCovered) {
        ++nCoveredNets;
        nCoveredPins += onet->numOPins();
    }
    nFlows += arcs.size();
    for (const GraphArc<T>& arc : arcs) arc.write(selOfs, design, die, pitch, dir);
}

template <typename T>
void Graph<T>::writeImg(
    const T* splitNet, const vector<T*>& splitNets, const Image& image, const string& path, const unsigned dir) {
    for (unsigned i = 0; i != splitNet->upVias.size(); ++i) {
        const NetRouteUpNode& node = splitNet->upVias[i];
        int x = node.x();
        int y = node.y();
        for (unsigned j = 1; j <= 4; j *= 2) {
            const double xOrigin = x - image.xStep() * j * 49.5;
            const double yOrigin = y - image.yStep() * j * 49.5;
            const double xStep = image.xStep() * j;
            const double yStep = image.yStep() * j;
            const int xCeil =
                min<int>(image.xNum(), ceil((xOrigin + xStep * 99 - image.xOrigin()) / image.xStep()) + 0.5);
            const int yCeil =
                min<int>(image.yNum(), ceil((yOrigin + yStep * 99 - image.yOrigin()) / image.yStep()) + 0.5);
            Image img(xOrigin, yOrigin, xStep, yStep, 99, 99);
            img.setRouting(splitNet, 4);
            for (unsigned yIdx = max<int>(0, (yOrigin - image.yOrigin()) / image.yStep());
                 static_cast<int>(yIdx) < yCeil;
                 ++yIdx) {
                for (unsigned xIdx = max<int>(0, (xOrigin - image.xOrigin()) / image.xStep());
                     static_cast<int>(xIdx) < xCeil;
                     ++xIdx) {
                    img.setData(image.yOrigin() + image.yStep() * (yIdx + 0.5),
                                image.xOrigin() + image.xStep() * (xIdx + 0.5),
                                image.data(yIdx, xIdx));
                }
            }
            img.encode(path + "/" + splitNet->name() + "_" + to_string(i) + "_" + to_string(j) + ".png", dir);
        }
    }
}

template <typename T>
bool Graph<T>::run(const vector<T*>& splitNets,
                   const Image& image,
                   const Rectangle& die,
                   const Point& pitch,
                   const unsigned dir,
                   const string& file) {
    for (T* splitNet : splitNets) {
        if (splitNet->upVias.empty()) continue;

        addNode(splitNet);
    }

    size_t pos0 = file.find_last_of('/');
    string path = ".";
    string csv = file;
    if (pos0 != string::npos) {
        path = file.substr(0, pos0);
        csv = file.substr(pos0 + 1);
    }
    size_t pos1 = csv.find('.');
    const string& base = csv.substr(0, pos1);
    path = path + "/" + base;

    size_t pos2 = base.find_last_of('_');
    const string& design = base.substr(0, pos2);

    ofstream wscOfs = io::IOModule::write(path + "/" + base + ".wsc.csv", true);
    ofstream drvOfs = io::IOModule::write(path + "/" + base + ".drv.csv", true);
    ofstream snkOfs = io::IOModule::write(path + "/" + base + ".snk.csv", true);
    wscOfs << "Vpin ID,X coordinate,Y coordinate,WL down to L1,Pin Type,NumberLayer One Pins,Ave Cell Area Input,Ave "
              "Cell Area Output,Ave Pin X coordinate,Ave Pin Y "
              "coordinate,CongestionCrouting,CongestionPlacement,Matching Vpin ID\n";
    drvOfs << "DESIGN,PARENT,NAME,VIA_IDX,VIA_ABSOLUTE_X,VIA_ABSOLUTE_Y,VIA_RELATIVE_X,VIA_RELATIVE_Y,DIR_NN,DIR_NP,"
              "DIR_PN,DIR_PP,SINK_COUNT,UP_PIN,WL,VIAS,WL_M4";
    snkOfs << "DESIGN,PARENT,NAME,VIA_IDX,VIA_ABSOLUTE_X,VIA_ABSOLUTE_Y,VIA_RELATIVE_X,VIA_RELATIVE_Y,DIR_NN,DIR_NP,"
              "DIR_PN,DIR_PP,SINK_COUNT,UP_PIN,WL,VIAS,WL_M4";
    for (unsigned i = 3; i; --i) {
        drvOfs << ",VIAS_V" << i << i + 1 << ",WL_M" << i;
        snkOfs << ",VIAS_V" << i << i + 1 << ",WL_M" << i;
    }
    drvOfs << endl;
    snkOfs << endl;
    for (unsigned inetIdx = 0; inetIdx != inodes.size(); ++inetIdx) {
        const T* inet = inodes[inetIdx];
        for (const T* onet : _onodes) {
            if (onet->parent() != inet->parent()) continue;

            inet->write(drvOfs, design, die, pitch, dir);
            onet->write(snkOfs, design, die, pitch, dir);
            const NetRouteUpNode& ivia = inet->upVias[0];
            const NetRouteUpNode& ovia = onet->upVias[0];
            wscOfs << 'S' << inetIdx * 2 << ',' << ivia[dir] << ',' << ivia[1 - dir] << ',' << inet->len() << ",O,"
                   << inet->numPins() << ',' << inet->oArea() << ',' << inet->iArea() << ',';
            const Point iMeanPin = inet->meanPin();
            wscOfs << iMeanPin[dir] << ',' << iMeanPin[1 - dir] << ",0,0,S" << inetIdx * 2 + 1 << endl;
            wscOfs << 'S' << inetIdx * 2 + 1 << ',' << ovia[dir] << ',' << ovia[1 - dir] << ',' << onet->len() << ",I,"
                   << onet->numPins() << ',' << onet->oArea() << ",0,";
            const Point oMeanPin = onet->meanPin();
            wscOfs << oMeanPin[dir] << ',' << oMeanPin[1 - dir] << ",0,0,S" << inetIdx * 2 << endl;
            break;
        }
    }
    wscOfs.close();
    drvOfs.close();
    snkOfs.close();

    selOfs = io::IOModule::write(path + "/" + base + ".sel.csv", true);
    selOfs << "SNK_NAME,DRV_NAME,SNK_VIA_IDX,DRV_VIA_IDX,SIGNED_ABSOLUTE_DIST_X,SIGNED_ABSOLUTE_DIST_Y,UNSIGNED_"
              "ABSOLUTE_DIST_X,UNSIGNED_ABSOLUTE_DIST_Y,SIGNED_RELATIVE_DIST_X,SIGNED_RELATIVE_DIST_Y,UNSIGNED_"
              "RELATIVE_DIST_X,UNSIGNED_RELATIVE_DIST_Y,SNK_SINK_COUNT,DRV_SINK_COUNT,SNK_UP_PIN,DRV_UP_PIN,SNK_WL,SNK_"
              "VIAS,SNK_WL_M4,";
    for (unsigned i = 3; i; --i) selOfs << "SNK_VIAS_V" << i << i + 1 << ",SNK_WL_M" << i << ',';
    selOfs << "DRV_WL,DRV_VIAS,DRV_WL_M4,";
    for (unsigned i = 3; i; --i) selOfs << "DRV_VIAS_V" << i << i + 1 << ",DRV_WL_M" << i << ',';
    selOfs << "LABEL\n";

    totalNumONetsMissed = 0;
    nFlows = 0;
    nCoveredNets = 0;
    nCoveredPins = 0;

#pragma omp parallel for
    for (unsigned i = 0; i < _nONodes; ++i) {
        Graph<T>::writeSel(_onodes[i], design, die, pitch, dir);
    }

    selOfs.close();
    if (totalNumONetsMissed) {
        printlog(LOG_WARN,
                 "%u / %u = %f sink nets missed while adding arcs",
                 totalNumONetsMissed,
                 _nONodes,
                 totalNumONetsMissed / static_cast<double>(_nONodes));
    }
    printlog(LOG_INFO,
             "%u iters %u driver nets %u flows cover %u / %u = %f sink nets and %u / %u = %f sink pins",
             DBModule::NumCands,
             _nINodes,
             nFlows,
             nCoveredNets,
             _nONodes,
             nCoveredNets / static_cast<double>(_nONodes),
             nCoveredPins,
             _nOPins,
             nCoveredPins / static_cast<double>(_nOPins));

#pragma omp parallel for
    for (unsigned i = 0; i < _nONodes; ++i) {
        Graph<T>::writeImg(_onodes[i], splitNets, image, path, dir);
    }

#pragma omp parallel for
    for (unsigned i = 0; i < _nINodes; ++i) {
        Graph<T>::writeImg(inodes[i], splitNets, image, path, dir);
    }

    return true;
}

}  // namespace db

#endif
