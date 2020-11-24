#ifndef _DB_NET_H_
#define _DB_NET_H_

#include <algorithm>
#include <set>

namespace db {

class NetRouteNode : public Point {
protected:
    const Layer* _layer = nullptr;
    int _comp = -1;
    Pin* _pin = nullptr;

public:
    int z = 0;
    vector<unsigned> adjs;

    NetRouteNode() {}
    NetRouteNode(const Layer* layer, const int x, const int y, const int z = 0) : Point(x, y), _layer(layer), z(z) {}
    NetRouteNode(const Layer* layer, const Rectangle& rect) : NetRouteNode(layer, rect.cx(), rect.cy()) {}
    NetRouteNode(const Geometry& geo) : NetRouteNode(geo.layer, geo) {}

    const Layer* layer() const { return _layer; }
    int comp() const { return _comp; }
    Pin* pin() { return _pin; }

    void comp(const int c) { _comp = c; }
    void pin(Pin* p) { _pin = p; }

    void shift(const int dx, const int dy);
    bool operator==(const NetRouteNode& r) const {
        return _layer == r._layer && _x == r._x && _y == r._y /*&& z == r.z*/;
    }
    static unsigned manhattanDistance(const NetRouteNode& m, const NetRouteNode& n) {
        return abs(m._x - n._x) + abs(m._y - n._y);
    }

    friend ostream& operator<<(ostream& os, const NetRouteNode& node) {
        return cout << node.layer()->name() << " ( " << node.x() << ' ' << node.y() << " )";
    }
};

class NetRouteUpNode : public NetRouteNode {
private:
    bool _dirnn = true;
    bool _dirnp = true;
    bool _dirpn = true;
    bool _dirpp = true;

public:
    NetRouteUpNode(const NetRouteNode& n) : NetRouteNode(n) {}

    bool dirnn() const { return _dirnn; }
    bool dirnp(const unsigned dir) const;
    bool dirpp() const { return _dirpp; }

    void setDir(const Net* net);
};

class NetRouteSegment {
private:
    const char _dir;
    const unsigned _len;
    const int _width;

public:
    //  The default extension value
    int z;
    //  Index of the `NetRouteNode` this segment begins with
    unsigned fromNode;
    //  Index of the `NetRouteNode` this segment ends at
    unsigned toNode;
    //  The routing path between the two `NetRouteNode`s
    //  Currently, all routing points are `NetRouteNode`s
    //  so that there is only one element in the vertor
    //  path = [<direction,len>]
    //  direction : N,S,E,W,U,D

    NetRouteSegment(const unsigned fromi, const unsigned toi, const char dir, const unsigned len, int width)
        : _dir(dir), _len(len), _width(width), fromNode(fromi), toNode(toi) {}

    char dir() const { return _dir; }
    int width() const { return _width; }
};

class NetRouting : public Rectangle {
protected:
    string _name = "";
    unsigned _len = 0;
    double _pitchLen = 0;
    unsigned _numVias = 0;

public:
    vector<NetRouteNode> nodes;
    vector<NetRouteSegment> segments;
    vector<unsigned> lens;
    vector<double> pitchLens;
    vector<unsigned> nVias;
    vector<Pin*> pins;
    vector<bg::index::rtree<std::pair<boostBox, Pin*>, bg::index::rstar<32>>> rtrees;

    NetRouting(const string& name, const unsigned nLayers)
        : _name(name), lens(32, 0), pitchLens(32, 0), nVias(32, 0), rtrees(nLayers) {}
    const string& name() const { return _name; }

    void eraseBackslashInName() { _name.erase(remove(_name.begin(), _name.end(), '\\'), _name.end()); }
    void addNode(const NetRouteNode& n, const bool updatePin = true);
    void addWire(const NetRouteNode& fromNode, const NetRouteNode& toNode, const int width, const char dir = '\0');

    unsigned len(const int rIdx = -1) const;
    double pitchLen(const int rIdx = -1) const;
    unsigned numVias(const int cIdx = -1) const;
    bool hasNodeOnLayer(const Layer* layer) const;

    static unsigned manhattanDistance(const NetRouting* m, const NetRouting* n, const Layer* layer = nullptr);
};

class Net : public NetRouting {
protected:
    const Use::UseEnum _use = Use::UseEnum::Signal;
    bool gRouted = false;
    bool dRouted = false;

    Pin* _iPin = nullptr;
    unsigned _numOPins = 0;
    unsigned _numIOPins = 0;
    const Pin* _upPin = nullptr;

    double _capacitance = 0;
    bool noWire(const NetRouteNode& upviaN, const NetRouteNode& upviaM) const;

public:
    const NDR* ndr = nullptr;
    vector<Geometry> shapes;

    Net(const string& name, const NDR* ndr, const Use::UseEnum use, const unsigned nLayers)
        : NetRouting(name, nLayers), _use(use), ndr(ndr) {}
    bool globalRouted() const { return gRouted; }
    bool detailedRouted() const { return dRouted; }
    Pin* iPin() { return _iPin; }
    unsigned numOPins() const { return _numOPins; }
    unsigned numIOPins() const { return _numIOPins; }
    const Pin* upPin() const { return _upPin; }
    double capacitance() const { return _capacitance; }

    void addPin(Pin* pin);
    vector<NetRouteNode> getNodes(const Layer* layer) const;

    unsigned numPins() const { return pins.size(); }
    int getPinId(const Pin* pin) const;
    double setCapacitance(const double default_capacitance = 0, const double default_max_capacitance = 0);
    vector<tuple<int, int, int, char>> halfPlane(const NetRouteNode& upvia) const;

    int iArea() const;
    int oArea() const;
    const Point meanPin() const;

    static bool isDirectNet(const NetRouteNode& upvia, const vector<tuple<int, int, int, char>>& planeSet);
    static bool isDirection(const NetRouteUpNode& upviaM, const NetRouteUpNode& upviaN);
    static bool isLoop(Net* inet, const Net* onet);
};

class SplitNet : public Net {
private:
    bool _isSelected = false;
    double _totalCap = 0;
    const Net* _parent = nullptr;

public:
    vector<NetRouteUpNode> upVias;

    SplitNet(const string& name, const NDR* ndr, const Net* parent)
        : Net(name, ndr, Use::UseEnum::Signal, 0), _parent(parent) {}

    bool isSelected() const { return _isSelected; }
    double totalCap() const { return _totalCap; }
    const Net* parent() const { return _parent; }

    void select() { _isSelected = true; }
    void totalCap(const double cap) { _totalCap = cap; }

    bool isSink() { return !iPin() && numOPins() && _parent; }
    bool isSource() { return iPin() && _parent; }

    void addUpVia(const NetRouteNode& n);

    void write(ostream& os, const string& design, const Rectangle& die, const Point& pitch, const unsigned dir) const;

    static bool isSeparate(const SplitNet* m, const SplitNet* n);
};

}  // namespace db

#endif
