#ifndef _DB_DB_H_
#define _DB_DB_H_

#include <boost/geometry.hpp>
#include <mutex>

#include "../global.h"

namespace bg = boost::geometry;

#if defined __GNUC__
#if 408 <= __GNUC__ * 100 + __GNUC_MINOR__
#define _GNUC_4_8_
#endif
#endif

namespace db {

class Use {
public:
    enum class UseEnum { None, Analog, Clock, Ground, Power, Signal };

    static UseEnum getUse(const string& name) {
        if (name == "ANALOG") {
            //  Pin is used for analog connectivity.
            return UseEnum::Analog;
        } else if (name == "CLOCK") {
            //  Pin is used for clock net connectivity.
            return UseEnum::Clock;
        } else if (name == "GROUND") {
            //  Pin is used for connectivity to the chip-
            //  level ground distribution network.
            return UseEnum::Ground;
        } else if (name == "POWER") {
            //  Pin is used for connectivity to the chip-
            //  level power distribution network.
            return UseEnum::Power;
        } else if (name == "SIGNAL") {
            //  Pin is used for regular net connectivity.
            return UseEnum::Signal;
        } else {
            printlog(LOG_ERROR, "unknown use: %s", name.c_str());
            return UseEnum::None;
        }
    }
};

class DBModule {
private:
    static std::string _name;

public:
    static unsigned CPU;
    static bool EdgeSpacing;
    static bool EnableFence;
    static bool EnablePG;
    static unsigned Metal;
    static unsigned NumCands;

    static void showOptions();

public:
    static bool setup();
    const std::string& name() const { return _name; }
};

class Cell;
class CellType;
class EdgeTypes;
class GCell;
class Geometry;
class Image;
class Interval;
class IOPin;
class Layer;
class NDR;
class Net;
class NetRouteNode;
class NetRouting;
class Pin;
class PinType;
class Placement;
class Point;
class Rectangle;
class Region;
class Row;
class RowSegment;
class SplitNet;
class Track;
class Via;
class ViaType;

template <typename T>
class Graph;

using boostPoint = bg::model::point<int, 2, bg::cs::cartesian>;
using boostBox = bg::model::box<boostPoint>;

}  // namespace db

#include "db_cell.h"
#include "db_drc.h"
#include "db_geom.h"
#include "db_image.h"
#include "db_layer.h"
#include "db_map.h"
#include "db_net.h"
#include "db_pin.h"
#include "db_place.h"
#include "db_region.h"
#include "db_row.h"
#include "db_td.h"
#include "db_via.h"

// Fragment Graph
#include "db_graph.h"

namespace db {

#define CLEAR_POINTER_LIST(list) \
    {                            \
        for (auto obj : list) {  \
            delete obj;          \
        }                        \
        list.clear();            \
    }

#define CLEAR_POINTER_MAP(map) \
    {                          \
        for (auto p : map) {   \
            delete p.second;   \
        }                      \
        map.clear();           \
    }

class Database : public Rectangle {
public:
    enum IssueType {
        E_ROW_EXCEED_DIE,
        E_OVERLAP_ROWS,
        W_NON_UNIFORM_SITE_WIDTH,
        W_NON_HORIZONTAL_ROW,
        E_MULTIPLE_NET_DRIVING_PIN,
        E_NO_NET_DRIVING_PIN
    };

    unordered_map<string, CellType*> name_celltypes;
    unordered_map<string, Cell*> name_cells;
    unordered_map<string, Net*> name_nets;
    unordered_map<string, SplitNet*> name_splitNets;
    unordered_map<string, IOPin*> name_iopins;
    unordered_map<string, ViaType*> name_viatypes;

    vector<Layer*> layers;
    unsigned nLayers = 0;

    vector<ViaType*> viatypes;
    vector<CellType*> celltypes;

    vector<Cell*> cells;
    vector<IOPin*> iopins;
    vector<Net*> nets;
    vector<SplitNet*> splitNets;
    vector<Row*> rows;
    vector<Region*> regions;
    map<string, NDR*> ndrs;
    vector<Net*> snets;
    vector<Track> tracks;

    vector<Image> images;
    unsigned snIdx;
    mutex snIdxMtx;
    Graph<SplitNet> graph;
    //  Graph<Pin> graph;

private:
    const size_t _bufferCapacity = 128 * 1024;
    size_t _bufferSize = 0;
    char* _buffer = nullptr;

    vector<Placement> _placements;
    unsigned _activePlacement;

public:
    unsigned siteW = 0;
    int siteH = 0;
    unsigned nSitesX = 0;
    unsigned nSitesY = 0;

    SiteMap siteMap;
    TDBins tdBins;  // local target density

    EdgeTypes edgetypes;

    double maxDensity = 0;
    double maxDisp = 0;

    int LefConvertFactor;
    double DBU_Micron;
    std::string designName;

    vector<IssueType> databaseIssues;

public:
    Database();
    ~Database();
    void clear();
    void clearTechnology();
    void clearLibrary() { CLEAR_POINTER_LIST(celltypes); }
    void clearDesign();

    Layer* addLayer(const string& name, const char type = 'x');
    ViaType* addViaType(const string& name, bool isDef = false);
    CellType* addCellType(const string& name, unsigned libcell);
    Cell* addCell(const string& name, CellType* type = nullptr);
    IOPin* addIOPin(const string& name = "", const string& netName = "", const char direction = 'x');
    Net* addNet(const string& name, const NDR* ndr, const unsigned nLayers);
    SplitNet* addSplitNet(const string& name, const NDR* ndr, const Net* net);
    SplitNet* addSplitNet(Pin* pin, const Layer* splitLayer);
    Row* addRow(const string& name,
                const string& macro = "",
                const int x = 0,
                const int y = 0,
                const unsigned xNum = 0,
                const unsigned yNum = 0,
                const bool flip = false,
                const unsigned xStep = 0,
                const unsigned yStep = 0);
    template <class... Args>
    Track& addTrack(Args&&... args) {
        tracks.emplace_back(args...);
        return tracks.back();
    }
    Region* addRegion(const string& name = "", const char type = 'x');
    NDR* addNDR(const string& name = "", const bool hardSpacing = false);
    Net* addSNet(const string& name, const Use::UseEnum);

    Layer* getRLayer(const int index);
    const Layer* getCLayer(const unsigned index) const;
    Layer* getLayer(const string& name);
    CellType* getCellType(const string& name);
    Cell* getCell(const string& name);
    Net* getNet(const string& name);
    Net* getNetHint(const string& name);
    SplitNet* getSplitNet(const string& name);
    Region* getRegion(const string& name);
    Region* getRegion(const unsigned char id);
    NDR* getNDR(const string& name) const;
    IOPin* getIOPin(const string& name) const;
    Pin* getPinFromConnection(const string& instance, const string& pin);
    ViaType* getViaType(const string& name) const;
    Net* getSNet(const string& name);

    Placement& placement(unsigned i);
    Placement& placement();
    void setActivePlacement(unsigned i);

    unsigned getNumLayers() const { return layers.size(); }
    unsigned getNumCells() const { return cells.size(); }
    unsigned getNumNets() const { return nets.size(); }
    unsigned getNumRegions() const { return regions.size(); }
    unsigned getNumIOPins() const { return iopins.size(); }
    unsigned getNumCellTypes() const { return celltypes.size(); }

    int getCellTypeSpace(const CellType* L, const CellType* R) const {
        return edgetypes.getEdgeSpace(L->edgetypeR, R->edgetypeL);
    }
    int getCellTypeSpace(const Cell* L, const Cell* R) const { return getCellTypeSpace(L->ctype(), R->ctype()); }

    long long getCellArea(Region* region = NULL) const;
    long long getFreeArea(Region* region = NULL) const;

    bool placed();
    bool globalRouted();
    bool detailedRouted();

    void setup();  // call after read

    long long getHPWL();

    /* defined in io/file_lefdef_db.cpp */
public:
    bool readLEF(const std::string& file);
    bool readDEF(const string& file);

    bool readBSAux(const std::string& auxFile, const std::string& plFile);
    bool readBSNodes(const std::string& file);
    bool readBSNets(const std::string& file);
    bool readBSScl(const std::string& file);
    bool readBSRoute(const std::string& file);
    bool readBSShapes(const std::string& file);
    bool readBSWts(const std::string& file);
    bool readBSPl(const std::string& file);
    bool writeBSPl(const std::string& file);

    bool readLiberty(const std::string& file);

    bool readNetDetail(const std::string& file);
    bool readTimePath(const std::string& file, bool isConstrained);

private:
    void setupSplitNets();
    unsigned setupImage(const unsigned imgIdx);
    void setupImages();
    void setupGraph();
};

}  // namespace db

extern db::Database database;

#endif
