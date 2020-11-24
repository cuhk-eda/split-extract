#ifndef _DB_CELL_H_
#define _DB_CELL_H_

namespace db {
class CellType {
    friend class Database;

private:
    int _originX = 0;
    int _originY = 0;
    //  X=1  Y=2  R90=4  (can be combined)
    char _symmetry = 0;
    string _siteName = "";
    vector<Geometry> _obs;

    int _libcell;
    unsigned _drive_strength;

public:
    std::string name = "";
    //  get gate composition
    int gType = 0;
    bool stdcell = false;
    int width = 0;
    int height = 0;
    vector<PinType*> pins;
    int edgetypeL = 0;
    int edgetypeR = 0;
    int usedCount = 0;
    //  CellType(const string& name = "", int libcell = -1) : _libcell(libcell), name(name) {}
    CellType(const string& name = "", int libcell = -1);
    ~CellType();

    PinType* addPin(const string& name = "",
                    const NDR* ndr = nullptr,
                    const char direction = 'x',
                    const Use::UseEnum use = Use::UseEnum::Signal);
    void addPin(PinType& pintype);

    template <typename... Args>
    void addObs(Args&&... params) { _obs.emplace_back(params...); }

    const PinType* getPin(const string& name) const;

    int originX() const { return _originX; }
    int originY() const { return _originY; }
    char symmetry() const { return _symmetry; }
    const std::vector<Geometry>& obs() const { return _obs; }
    int libcell() const { return _libcell; }
    unsigned drive_strength() const { return _drive_strength; }

    void setOrigin(const int x, const int y);
    void setXSymmetry() { _symmetry &= 1; }
    void setYSymmetry() { _symmetry &= 2; }
    void set90Symmetry() { _symmetry &= 4; }
    void siteName(const std::string& name) { _siteName = name; }
    void drive_strength(const unsigned u) { _drive_strength = u; }

    bool operator==(const CellType& r) const;
    bool operator!=(const CellType& r) const { return !(*this == r); }
};

class Cell {
private:
    string _name = "";
    int _spaceL = 0;
    int _spaceR = 0;
    int _spaceB = 0;
    int _spaceT = 0;
    bool _fixed = false;
    CellType* _type = nullptr;
    vector<Pin*> _pins;

public:
    bool highlighted = false;
    Region* region = nullptr;

    Cell(const string& name = "", CellType* type = nullptr) : _name(name) { ctype(type); }
    ~Cell();

    const string& name() const { return _name; }
    const vector<Pin*>& pins() const { return _pins; }
    Pin* pin(const std::string& name) const;
    Pin* pin(unsigned i) const { return _pins[i]; }
    CellType* ctype() const { return _type; }
    void ctype(CellType* t);
    int lx() const;
    int ly() const;
    int hx() const { return lx() + width(); }
    int hy() const { return ly() + height(); }
    int cx() const { return lx() + width() / 2; }
    int cy() const { return ly() + height() / 2; }
    bool r90() const;
    bool flipX() const;
    bool flipY() const;
    int width() const { return _type->width + _spaceL + _spaceR; }
    int height() const { return _type->height + _spaceB + _spaceT; }
    int siteWidth() const;
    int siteHeight() const;
    bool fixed() const { return _fixed; }
    void fixed(bool fix) { _fixed = fix; }
    bool placed() const;
    void place(int x, int y);
    void place(int x, int y, bool r90, bool flipX, bool flipY);
    void unplace();

    unsigned numPins() const { return _pins.size(); }
    void transform(Geometry& geo) const;
};
}  // namespace db

#endif
