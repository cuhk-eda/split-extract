#ifndef _DB_PIN_H_
#define _DB_PIN_H_

namespace db {
class PinType : public Rectangle {
private:
    const string _name = "";
    const NDR* _ndr = nullptr;
    //  i: input, o:output
    char _direction = 'x';
    Use::UseEnum _use = Use::UseEnum::Signal;
    double _capacitance = 0;

public:
    vector<Geometry> shapes;

    PinType(const string& name, const NDR* ndr, const char direction = 'x', const Use::UseEnum use = Use::UseEnum::Signal)
        : _name(name), _ndr(ndr), _direction(direction), _use(use) {}

    const string& name() const { return _name; }
    char direction() const { return _direction; }
    Use::UseEnum use() const { return _use; }
    double capacitance() const { return _capacitance; }

    void direction(const char c) { _direction = c; }
    void capacitance(const double d) { _capacitance = d; }

    template <typename... Args> void addShape(Args&&... params) {
        shapes.emplace_back(params...);
        unionWith(shapes.back());
    }

    const tuple<int, int, int, int, int> makeTuple() const { return make_tuple(lx(), ly(), hx(), hy(), shapes[0].layer->rIdx); }

    bool operator==(const PinType& rhs) const { return makeTuple() == rhs.makeTuple(); }
    bool operator!=(const PinType& rhs) const { return !(*this == rhs); }
    bool operator<(const PinType& rhs) const { return makeTuple() < rhs.makeTuple(); }
    bool operator>(const PinType& rhs) const { return rhs < *this; }
    bool operator>=(const PinType& rhs) const { return !(*this < rhs); }
    bool operator<=(const PinType& rhs) const { return !(*this > rhs); }
    static bool comparePin(vector<PinType*> pins1, vector<PinType*> pins2);
};

class IOPin {
protected:
    string _netName = "";

public:
    string name = "";
    int x = INT_MIN;
    int y = INT_MIN;
    PinType* type;
    Pin* pin;

    IOPin(const string& name = "", const string& netName = "", const char direction = 'x');
    ~IOPin();

    const string& netName() const { return _netName; }
    void transform(Geometry& geo) const { geo.shift(x, y); }
};

class PinSTA {
public:
    //'b': timing begin, 'e': timing end, 'i': intermediate
    char type = 'x';
    double capacitance;
    double aat[4];
    double rat[4];
    double slack[4];
    int nCriticalPaths[4];
    PinSTA();
};

class Pin {
private:
    double _delay = 0;
    double _arrive = 0;
    double _require = 0;
    Net* _net = nullptr;
    NetRouteNode _nrn;
    const SplitNet* _splitNet = nullptr;

public:
    Cell* cell = nullptr;
    IOPin* iopin = nullptr;
    const PinType* type = nullptr;

    PinSTA* staInfo = nullptr;

    Pin(const PinType* type = nullptr) : type(type) {}
    Pin(Cell* cell, int i) : cell(cell), type(cell->ctype()->pins[i]) {}
    Pin(IOPin* iopin) : iopin(iopin), type(iopin->type) {}

    void delay(double d) { _delay = d; }
    void arrive(double a) { _arrive = a; }
    void require(double r) { _require = r; }
    void net(Net* n) { _net = n; }
    void nrn(const NetRouteNode& nrn) { _nrn = nrn; }
    void splitNet(const SplitNet* s) { _splitNet = s; }

    double delay() const { return _delay; }
    double arrive() const { return _arrive; }
    double require() const { return _require; }
    const Net* net() const { return _net; }
    const NetRouteNode& nrn() const { return _nrn; }
    const SplitNet* splitNet() const { return _splitNet; }

    void getPinCenter(int& x, int& y) const;
    const vector<NetRouteUpNode>& getUpVias() const { return _splitNet->upVias; }
    bool isSink() const { return type->direction() == 'i'; }
    unsigned numOPins() const { return isSink() ? 1 : 0; }
    bool isSource() const { return type->direction() == 'o'; }
    const Pin* iPin() const { return isSource() ? this : nullptr; }
    double capacitance() const { return type->capacitance(); }
    const string& name() const { return type->name(); }
    const Pin* upPin() const { return this; }
    const Net* parent() const { return net(); }
    static unsigned manhattanDistance(const Pin* m, const Pin* n);
    static unsigned manhattanDistance(const vector<Pin*>& m, const vector<Pin*>& n);
    static unsigned manhattanDistance(Net* m, Net* n);
    static bool isLoop(const Pin* m, const Pin* n) { return m->cell && n->cell && m->cell == n->cell; }
};
}   //  namespace db

#endif

