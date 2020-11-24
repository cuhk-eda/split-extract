#include <sys/stat.h>

#include <atomic>
#include <unordered_set>

#include "../db/db.h"
#include "../def58/inc/defiUtil.hpp"
#include "../def58/inc/defrReader.hpp"
#include "../global.h"
#include "../io/io.h"
#include "../lef58/inc/lefrReader.hpp"
#include "../sta/sta.h"
#include "../ut/utils.h"
using namespace db;

bool isR90(int orient) {
    switch (orient) {
        case 0:
            return false;  // N
        case 1:
            return true;  // W
        case 2:
            return false;  // S
        case 3:
            return true;  // E
        case 4:
            return false;  // FN
        case 5:
            return true;  // FW
        case 6:
            return false;  // FS
        case 7:
            return true;  // FE
    }
    return false;
}

bool isFlipX(int orient) {
    switch (orient) {
        case 0:
            return false;  // N
        case 1:
            return false;  // W
        case 2:
            return true;  // S
        case 3:
            return true;  // E
        case 4:
            return true;  // FN
        case 5:
            return true;  // FW
        case 6:
            return false;  // FS
        case 7:
            return false;  // FE
    }
    return false;
}

bool isFlipY(int orient) {
    switch (orient) {
        case 0:
            return false;  // N
        case 1:
            return false;  // W
        case 2:
            return true;  // S
        case 3:
            return true;  // E
        case 4:
            return false;  // FN
        case 5:
            return false;  // FW
        case 6:
            return true;  // FS
        case 7:
            return true;  // FE
    }
    return false;
}

const string getOrient(const bool r90, const bool flipX, const bool flipY) {
    if (r90) {
        if (flipX) {
            if (flipY) {
                return "E";
            } else {
                return "FW";
            }
        } else {
            if (flipY) {
                return "FE";
            } else {
                return "W";
            }
        }
    } else {
        if (flipX) {
            if (flipY) {
                return "S";
            } else {
                return "FN";
            }
        } else {
            if (flipY) {
                return "FS";
            } else {
                return "N";
            }
        }
    }
}

#define DIR_UP 1
#define DIR_DOWN 2
#define DIR_LEFT 4
#define DIR_RIGHT 8

unsigned char pointDir(const int lx, const int ly, const int hx, const int hy) {
    if ((lx == hx) == (ly == hy)) {
        return 0;
    }
    if (lx < hx) {
        return DIR_RIGHT;
    }
    if (lx > hx) {
        return DIR_LEFT;
    }
    if (ly < hy) {
        return DIR_UP;
    }
    return DIR_DOWN;
}

class Vertex {
public:
    int x, y;
    unsigned char outdir = 0;

    Vertex(const int x, const int y, const unsigned char dir) : x(x), y(y), outdir(dir) {}

    inline bool operator<(const Vertex& T) const {
        if (y < T.y) {
            return true;
        }
        if (y > T.y) {
            return false;
        }
        if (x < T.x) {
            return true;
        }
        if (x > T.x) {
            return false;
        }
        if ((outdir | DIR_DOWN) && T.outdir | DIR_UP) {
            return true;
        }
        if ((outdir | DIR_UP) && T.outdir | DIR_DOWN) {
            return false;
        }
        if ((outdir | DIR_LEFT) && T.outdir | DIR_RIGHT) {
            return true;
        }
        return false;
    }

    inline bool operator==(const Vertex& T) const { return x == T.x && y == T.y; }
    inline bool operator!=(const Vertex& T) const { return !(*this == T); }
};

int readLefUnits(lefrCallbackType_e c, lefiUnits* unit, lefiUserData ud);
int readLefProp(lefrCallbackType_e c, lefiProp* prop, lefiUserData ud);
int readLefLayer(lefrCallbackType_e c, lefiLayer* leflayer, lefiUserData ud);
int readLefVia(lefrCallbackType_e c, lefiVia* lvia, lefiUserData ud);
int readLefMacroBegin(lefrCallbackType_e c, const char* macroName, lefiUserData ud);
int readLefObs(lefrCallbackType_e c, lefiObstruction* obs, lefiUserData ud);
int readLefPin(lefrCallbackType_e c, lefiPin* pin, lefiUserData ud);
int readLefMacro(lefrCallbackType_e c, lefiMacro* macro, lefiUserData ud);

int readDefUnits(defrCallbackType_e c, double d, defiUserData ud);
int readDefDesign(defrCallbackType_e c, const char* name, defiUserData ud);
int readDefDieArea(defrCallbackType_e c, defiBox* dbox, defiUserData ud);
int readDefRow(defrCallbackType_e c, defiRow* drow, defiUserData ud);
int readDefTrack(defrCallbackType_e c, defiTrack* dtrack, defiUserData ud);
int readDefVia(defrCallbackType_e c, defiVia* dvia, defiUserData ud);
int readDefNdr(defrCallbackType_e c, defiNonDefault* nd, defiUserData ud);
int readDefComponent(defrCallbackType_e c, defiComponent* co, defiUserData ud);
int readDefPin(defrCallbackType_e c, defiPin* dpin, defiUserData ud);
int readDefNet(defrCallbackType_e c, defiNet* dnet, defiUserData ud);
int readDefRegion(defrCallbackType_e c, defiRegion* dreg, defiUserData ud);
int readDefGroupMember(defrCallbackType_e c, const char* cl, defiUserData ud);
int readDefGroup(defrCallbackType_e c, defiGroup* dgp, defiUserData ud);

int readDefSNetStart(defrCallbackType_e c, int num, defiUserData ud);
int readDefSNet(defrCallbackType_e c, defiNet* dnet, defiUserData ud);

Database& getDBFromUD(defiUserData ud) { return *static_cast<Database*>(ud); }

bool Database::readLEF(const string& file) {
    FILE* fp;
    if (!(fp = fopen(file.c_str(), "r"))) {
        printlog(LOG_ERROR, "Unable to open LEF file: %s", file.c_str());
        return false;
    }

#ifndef NDEBUG
    printlog(LOG_INFO, "reading %s", file.c_str());
#endif

    lefrSetUnitsCbk(readLefUnits);
    lefrSetPropCbk(readLefProp);
    lefrSetLayerCbk(readLefLayer);
    lefrSetViaCbk(readLefVia);
    lefrSetMacroCbk(readLefMacro);
    lefrSetMacroBeginCbk(readLefMacroBegin);
    lefrSetObstructionCbk(readLefObs);
    lefrSetPinCbk(readLefPin);
    lefrInit();
    lefrReset();
    int res = lefrRead(fp, file.c_str(), (void*)&database);
    if (res) {
        printlog(LOG_ERROR, "Error in reading LEF");
        return false;
    }
    lefrReleaseNResetMemory();
    //  lefrUnsetCallbacks();
    lefrUnsetLayerCbk();
    lefrUnsetNonDefaultCbk();
    lefrUnsetViaCbk();
    fclose(fp);
    return true;
}

bool Database::readDEF(const string& file) {
    FILE* fp;
    if (!(fp = fopen(file.c_str(), "r"))) {
        printlog(LOG_ERROR, "Unable to open DEF file: %s", file.c_str());
        return false;
    }

#ifndef NDEBUG
    printlog(LOG_INFO, "reading %s", file.c_str());
#endif

    defrSetDesignCbk(readDefDesign);
    defrSetUnitsCbk(readDefUnits);
    defrSetDieAreaCbk(readDefDieArea);
    defrSetRowCbk(readDefRow);
    defrSetTrackCbk(readDefTrack);
    defrSetViaCbk(readDefVia);
    defrSetNonDefaultCbk(readDefNdr);
    defrSetComponentCbk(readDefComponent);
    defrSetPinCbk(readDefPin);

    defrSetNetCbk(readDefNet);
    // augment nets with path data
    defrSetAddPathToNet();

    defrSetRegionCbk(readDefRegion);
    defrSetGroupMemberCbk(readDefGroupMember);
    defrSetGroupCbk(readDefGroup);

    defrSetSNetStartCbk(readDefSNetStart);
    defrSetSNetCbk(readDefSNet);

    defrInit();
    defrReset();
    int res = defrRead(fp, file.c_str(), (void*)&database, 1);
    if (res) {
        printlog(LOG_ERROR, "Error in reading DEF");
        return false;
    }
    defrReleaseNResetMemory();
    defrUnsetCallbacks();
    fclose(fp);
    return true;
}

/****************/
/* LEF Callback */
/****************/

int readLefUnits(lefrCallbackType_e c, lefiUnits* unit, lefiUserData ud) {
    if (unit->lefiUnits::hasDatabase() && strcmp(unit->lefiUnits::databaseName(), "MICRONS") == 0) {
        getDBFromUD(ud).LefConvertFactor = unit->lefiUnits::databaseNumber();
    }
    return 0;
}

//-----Property-----
int readLefProp(lefrCallbackType_e c, lefiProp* prop, lefiUserData ud) {
    Database& db = getDBFromUD(ud);
    // default edge type
    if (db.edgetypes.types.empty()) {
        db.edgetypes.types.push_back("default");
        db.edgetypes.distTable.resize(1, vector<int>(1, 0));
    }
    if (prop->lefiProp::hasString() && !strcmp(prop->lefiProp::propName(), "LEF58_CELLEDGESPACINGTABLE")) {
        istringstream iss(prop->lefiProp::string());
        string buffer;

        while (!iss.eof()) {
            iss >> buffer;
            if (buffer == "EDGETYPE") {
                string type1, type2;
                double microndist;
                iss >> type1 >> type2 >> microndist;
                int dbdist = 0;
                if (DBModule::EdgeSpacing) dbdist = microndist * db.LefConvertFactor + 0.5;
                int type1_idx = db.edgetypes.getEdgeType(type1);
                int type2_idx = db.edgetypes.getEdgeType(type2);

                if (type1_idx < 0) {
                    type1_idx = db.edgetypes.types.size();
                    db.edgetypes.types.push_back(type1);
                }
                if (type2_idx < 0) {
                    type2_idx = db.edgetypes.types.size();
                    db.edgetypes.types.push_back(type2);
                }

                size_t numEdgeTypes = db.edgetypes.types.size();
                if (numEdgeTypes > db.edgetypes.distTable.size()) db.edgetypes.distTable.resize(numEdgeTypes);
                for (vector<int>& table : db.edgetypes.distTable) table.resize(numEdgeTypes, 0);
                db.edgetypes.distTable[type1_idx][type2_idx] = dbdist;
                db.edgetypes.distTable[type2_idx][type1_idx] = dbdist;
            }
            if (buffer == ";") break;
        }
        // default edge type dist
        for (unsigned i = 0; i < db.edgetypes.types.size(); i++) {
            db.edgetypes.distTable[0][i] = 0;
            db.edgetypes.distTable[i][0] = 0;
        }
    }
    return 0;
}

//-----Layer-----
int readLefLayer(lefrCallbackType_e c, lefiLayer* leflayer, lefiUserData ud) {
    Database& db = getDBFromUD(ud);
    const int convertFactor = db.LefConvertFactor;

    char type = 'x';
    if (!strcmp(leflayer->type(), "ROUTING")) {
        type = 'r';
    } else if (!strcmp(leflayer->type(), "CUT")) {
        type = 'c';
    } else {
        return 0;
    }

    Layer* layer = db.addLayer(leflayer->name(), type);

    switch (type) {
        case 'r':
            // routing layer
            if (leflayer->hasDirection()) {
                if (strcmp(leflayer->direction(), "HORIZONTAL") == 0) {
                    layer->direction = 'h';
                } else if (strcmp(leflayer->direction(), "VERTICAL") == 0) {
                    layer->direction = 'v';
                }
            } else {
                layer->direction = 'x';
            }

            if (leflayer->hasXYPitch()) {
                if (layer->direction == 'v') {
                    layer->pitch = leflayer->pitchX() * convertFactor + 0.5;
                } else {
                    layer->pitch = leflayer->pitchY() * convertFactor + 0.5;
                }
            }
            if (leflayer->hasPitch()) {
                layer->pitch = leflayer->pitch() * convertFactor + 0.5;
            }

            if (leflayer->hasXYOffset()) {
                if (layer->direction == 'v') {
                    layer->offset = leflayer->offsetX() * convertFactor + 0.5;
                } else {
                    layer->offset = leflayer->offsetY() * convertFactor + 0.5;
                }
            }
            if (leflayer->hasOffset()) {
                layer->offset = leflayer->offset() * convertFactor + 0.5;
            }

            if (leflayer->hasWidth()) {
                layer->width = leflayer->width() * convertFactor + 0.5;
            }

            if (layer->width > 0 && layer->pitch > 0) {
                layer->spacing = layer->pitch - layer->width;
            } else if (layer->width < 0 && layer->pitch > 0) {
                layer->width = layer->pitch / 2;
                layer->spacing = layer->pitch - layer->width;
            } else if (layer->pitch < 0 && layer->width > 0) {
                layer->pitch = layer->width * 2;
                layer->spacing = layer->width;
            } else {
                // no pitch nor width information
            }
            return 0;
        case 'c':
            // cut (via) layer
            int nspace;
            if (leflayer->hasSpacingNumber()) {
                nspace = leflayer->numSpacing();
                for (int i = 0; i < nspace; i++) {
                    layer->spacing = leflayer->spacing(i);
                }
            }
            return 0;
        default:
            return 0;
    }
}

//-----Via-----
int readLefVia(lefrCallbackType_e c, lefiVia* lvia, lefiUserData ud) {
    Database& db = getDBFromUD(ud);
    const int convertFactor = db.LefConvertFactor;

    ViaType* via = db.addViaType(lvia->name(), false);
    for (int i = 0; i < lvia->numLayers(); i++) {
        const string layername(lvia->layerName(i));
        Layer* layer = db.getLayer(layername);
        if (!layer) printlog(LOG_ERROR, "layer not found: %s", layername.c_str());
        for (int j = 0; j < lvia->numRects(i); ++j) {
            via->addRect(layer,
                         lvia->xl(i, j) * convertFactor + 0.5,
                         lvia->yl(i, j) * convertFactor + 0.5,
                         lvia->xh(i, j) * convertFactor + 0.5,
                         lvia->yh(i, j) * convertFactor + 0.5);
        }
    }
    return 0;
}

//-----CellType-----
int readLefMacroBegin(lefrCallbackType_e c, const char* macroName, lefiUserData ud) {
    getDBFromUD(ud).addCellType(macroName, rlib.addCell(macroName));
    return 0;
}

int readLefObs(lefrCallbackType_e c, lefiObstruction* obs, lefiUserData ud) {
    Database& db = getDBFromUD(ud);
    const int convertFactor = db.LefConvertFactor;
    CellType* celltype = db.celltypes.back();  // get the last inserted celltype

    Layer* layer = NULL;
    lefiGeometries* geom = obs->lefiObstruction::geometries();
    int nitem = geom->lefiGeometries::numItems();
    for (int i = 0; i < nitem; i++) {
        if (geom->lefiGeometries::itemType(i) == lefiGeomLayerE)
            layer = db.getLayer(geom->lefiGeometries::getLayer(i));
        else if (geom->lefiGeometries::itemType(i) == lefiGeomRectE && layer != NULL) {
            lefiGeomRect* rect = geom->lefiGeometries::getRect(i);
            celltype->addObs(layer,
                             rect->xl * convertFactor + 0.5,
                             rect->yl * convertFactor + 0.5,
                             rect->xh * convertFactor + 0.5,
                             rect->yh * convertFactor + 0.5);
        }
    }
    return 0;
}

int readLefPin(lefrCallbackType_e c, lefiPin* pin, lefiUserData ud) {
    Database& db = getDBFromUD(ud);
    const int convertFactor = db.LefConvertFactor;
    CellType* celltype = db.celltypes.back();  // get the last inserted celltype

    char direction = 'x';
    Use::UseEnum use = Use::UseEnum::Signal;
    if (pin->hasUse()) use = Use::getUse(pin->use());
    if (pin->hasDirection()) {
        if (!strcmp("OUTPUT", pin->direction()) || !strcmp("OUTPUT TRISTATE", pin->direction())) {
            direction = 'o';
        } else if (!strcmp("INPUT", pin->direction())) {
            direction = 'i';
        } else if (!strcmp("INOUT", pin->direction())) {
            direction = 'p';
        } else {
            printlog(LOG_ERROR, "unknown pin direction: %s", pin->direction());
        }
    }

    string name(pin->name());

    switch (direction) {
        case 'i':
            rlib.cells[celltype->libcell()].addIPin(name);
            break;
        case 'o':
            rlib.cells[celltype->libcell()].addOPin(name);
            break;
        default:
            break;
    }

    NDR* ndr = nullptr;
    if (pin->hasTaperRule()) {
        ndr = db.getNDR(string(pin->taperRule()));
    }

    PinType* pintype = celltype->addPin(name, ndr, direction, use);

    set<Vertex> nodes;
    Layer* layer = nullptr;
    lefiGeomRect* rect = nullptr;
    lefiGeomPolygon* polygon = nullptr;
    lefiGeometries* geom = pin->port(0);
    for (unsigned i = 0; i != (unsigned)geom->numItems(); ++i) {
        switch (geom->itemType(i)) {
            case lefiGeomLayerE:
                layer = db.getLayer(string(geom->getLayer(i)));
                assert(layer);
                break;
            case lefiGeomRectE:
                rect = geom->getRect(i);
                pintype->addShape(layer,
                                  rect->xl * convertFactor + 0.5,
                                  rect->yl * convertFactor + 0.5,
                                  rect->xh * convertFactor + 0.5,
                                  rect->yh * convertFactor + 0.5);
                break;
            case lefiGeomPolygonE:
                polygon = geom->getPolygon(i);
                for (unsigned j = 0, numPoints = polygon->numPoints; j != numPoints; ++j) {
                    unsigned jPre = (j + numPoints - 1) % numPoints;
                    unsigned jPost = (j + 1) % numPoints;
                    const unsigned char dir = pointDir(polygon->x[j] * convertFactor + 0.5,
                                                       polygon->y[j] * convertFactor + 0.5,
                                                       polygon->x[jPre] * convertFactor + 0.5,
                                                       polygon->y[jPre] * convertFactor + 0.5) |
                                              pointDir(polygon->x[j] * convertFactor + 0.5,
                                                       polygon->y[j] * convertFactor + 0.5,
                                                       polygon->x[jPost] * convertFactor + 0.5,
                                                       polygon->y[jPost] * convertFactor + 0.5);
                    if (dir == (DIR_DOWN | DIR_UP) || dir == (DIR_LEFT | DIR_RIGHT)) continue;
                    nodes.emplace(polygon->x[j] * convertFactor + 0.5, polygon->y[j] * convertFactor + 0.5, dir);
                }
                while (nodes.size()) {
                    set<Vertex>::iterator pk;
                    set<Vertex>::iterator pl;
                    set<Vertex>::iterator pm;
                    set<Vertex>::iterator pn;

                    while (nodes.size()) {
                        pk = nodes.begin();
                        pl = nodes.begin();
                        ++pl;

                        if (pk->x != pl->x) {
                            break;
                        }

                        //  cout << "erase  " << nodes.begin()->x << '\t' << nodes.begin()->y << endl;
                        nodes.erase(nodes.begin());
                        //  cout << "erase  " << nodes.begin()->x << '\t' << nodes.begin()->y << endl;
                        nodes.erase(nodes.begin());
                    }

                    if (nodes.empty()) {
                        break;
                    }

                    if (nodes.size() < 4) {
                        cout << "Error when partitioning rectangles." << endl;
                        cout << "Remaining points: ";
                        for (const Vertex& p : nodes) {
                            printf("(%d, %d), ", p.x, p.y);
                        }
                        cout << endl;
                        break;
                    }

                    for (pm = nodes.begin(); pm->y <= pk->y || pm->x < pk->x || pm->x >= pl->x; ++pm) {
                    }

                    for (pn = pm; pn->x < pl->x && pn->y <= pm->y; ++pn) {
                    }
                    pintype->addShape(layer, pk->x, pk->y, pl->x, pm->y);

                    Vertex ul(pk->x, pm->y, DIR_DOWN | DIR_RIGHT);
                    Vertex ur(pl->x, pm->y, DIR_DOWN | DIR_LEFT);

                    // Insert or erase the upper-right point of the rectangle.
                    if (*pn == ur && pn->outdir != (DIR_UP | DIR_RIGHT)) {
                        //  cout << "erase  " << pn->x << '\t' << pn->y << endl;
                        nodes.erase(pn);
                    } else {
                        //  cout << "insert " << ur.x << '\t' << ur.y << endl;
                        nodes.insert(ur);
                    }

                    // Insert or erase the upper-left point of the rectangle.
                    pm = nodes.find(ul);
                    if (pm == nodes.end()) {
                        //  cout << "insert " << ul.x << '\t' << ul.y << endl;
                        nodes.insert(ul);
                    } else {
                        //  cout << "erase  " << pm->x << '\t' << pm->y << endl;
                        nodes.erase(pm);
                    }

                    //  cout << "erase  " << nodes.begin()->x << '\t' << nodes.begin()->y << endl;
                    nodes.erase(nodes.begin());
                    //  cout << "erase  " << nodes.begin()->x << '\t' << nodes.begin()->y << endl;
                    nodes.erase(nodes.begin());
                    //  cout << nodes.size() << endl;
                }
                break;
            default:
                break;
        }
    }
    return 0;
}

int readLefMacro(lefrCallbackType_e c, lefiMacro* macro, lefiUserData ud) {
    Database& db = getDBFromUD(ud);
    const int convertFactor = db.LefConvertFactor;
    CellType* celltype = db.celltypes.back();

    celltype->width = macro->sizeX() * convertFactor + 0.5;
    celltype->height = macro->sizeY() * convertFactor + 0.5;

    if (macro->lefiMacro::hasOrigin()) {
        celltype->setOrigin(macro->originX() * convertFactor + 0.5, macro->originY() * convertFactor + 0.5);
    }

    if (macro->hasXSymmetry()) {
        celltype->setXSymmetry();
    }
    if (macro->hasYSymmetry()) {
        celltype->setYSymmetry();
    }
    if (macro->has90Symmetry()) {
        celltype->set90Symmetry();
    }

    if (macro->hasSiteName()) {
        celltype->siteName(string(macro->siteName()));
    }

    for (int i = 0; i < macro->numProperties(); ++i) {
        if (!strcmp(macro->propName(i), "LEF58_EDGETYPE")) {
            stringstream ssedgetype(macro->propValue(i));
            string buffer;
            while (!ssedgetype.eof()) {
                ssedgetype >> buffer;
                if (buffer == "EDGETYPE") {
                    string edgeside;
                    string edgetype;
                    ssedgetype >> edgeside >> edgetype;
                    if (edgeside == "LEFT") {
                        celltype->edgetypeL = db.edgetypes.getEdgeType(edgetype);
                    } else if (edgeside == "RIGHT") {
                        celltype->edgetypeR = db.edgetypes.getEdgeType(edgetype);
                    } else {
                        printlog(LOG_WARN, "unknown edge side: %s", edgeside.c_str());
                    }
                }
            }
        }
    }
    return 0;
}

/******************/
/*  DEF Callback  */
/******************/

//-----Unit-----
int readDefUnits(defrCallbackType_e c, double d, defiUserData ud) {
    getDBFromUD(ud).DBU_Micron = d;
    return 0;
}

//-----Design-----
int readDefDesign(defrCallbackType_e c, const char* name, defiUserData ud) {
    getDBFromUD(ud).designName = string(name);
    return 0;
}

//-----Die Area-----
int readDefDieArea(defrCallbackType_e c, defiBox* dbox, defiUserData ud) {
    getDBFromUD(ud).set(dbox->xl(), dbox->yl(), dbox->xh(), dbox->yh());
    return 0;
}

//-----Row-----
int readDefRow(defrCallbackType_e c, defiRow* drow, defiUserData ud) {
    getDBFromUD(ud).addRow(string(drow->name()),
                           string(drow->macro()),
                           drow->x(),
                           drow->y(),
                           drow->xNum(),
                           drow->yNum(),
                           isFlipY(drow->orient()),
                           drow->xStep(),
                           drow->yStep());
    return 0;
}

//-----Track-----
int readDefTrack(defrCallbackType_e c, defiTrack* dtrack, defiUserData ud) {
    Database& db = getDBFromUD(ud);

    char direction = 'x';
    if (strcmp(dtrack->macro(), "X") == 0) {
        direction = 'v';
    } else if (strcmp(dtrack->macro(), "Y") == 0) {
        direction = 'h';
    }
    Track& track = db.addTrack(direction, dtrack->x(), dtrack->xNum(), dtrack->xStep());
    for (unsigned i = 0; (int)i < dtrack->numLayers(); ++i) {
        string layername(dtrack->layer(i));
        track.addLayer(layername);
        Layer* layer = db.getLayer(layername);
        if (!layer) {
            printlog(LOG_ERROR, "layer name not found: %s", layername.c_str());
        } else if (layer->direction == direction) {
            layer->track = track;
        }
    }
    return 0;
}

//-----Via-----
int readDefVia(defrCallbackType_e c, defiVia* dvia, defiUserData ud) {
    Database& db = getDBFromUD(ud);

    const string name(dvia->name());
    ViaType* via = db.getViaType(name);
    if (via)
        via->isDef(true);
    else
        via = db.addViaType(name, true);

    char* dvialayer = nullptr;
    int lx = 0;
    int ly = 0;
    int hx = 0;
    int hy = 0;
    for (int i = 0; i != dvia->numLayers(); ++i) {
        dvia->layer(i, &dvialayer, &lx, &ly, &hx, &hy);
        Layer* layer = db.getLayer(dvialayer);
        if (layer) {
            via->addRect(layer, lx, ly, hx, hy);
        } else {
            printlog(LOG_INFO, "layer name not found: %s", dvialayer);
        }
    }
    return 0;
}

//-----NonDefaultRule-----

int readDefNdr(defrCallbackType_e c, defiNonDefault* nd, defiUserData ud) {
    Database& db = getDBFromUD(ud);
    NDR* ndr = db.addNDR(string(nd->name()), nd->hasHardspacing());

    for (int i = 0; i < nd->numLayers(); i++) {
        int space = 0;
        if (nd->hasLayerSpacing(i)) {
            space = nd->layerSpacingVal(i);
        }
        ndr->rules.emplace_back(db.getLayer(string(nd->layerName(i))), nd->defiNonDefault::layerWidthVal(i), space);
    }
    for (int i = 0; i < nd->numVias(); i++) {
        string vianame(nd->viaName(i));
        ViaType* viatype = db.getViaType(vianame);
        if (!viatype) {
            printlog(LOG_WARN, "NDR via type not found: %s", vianame.c_str());
        }
        ndr->vias.push_back(viatype);
    }
    return 0;
}

//-----Cell-----
int readDefComponent(defrCallbackType_e c, defiComponent* co, defiUserData ud) {
    Database& db = getDBFromUD(ud);

    Cell* cell = db.addCell(string(co->id()), db.getCellType(string(co->name())));

    if (co->isUnplaced()) {
        cell->fixed(false);
        cell->unplace();
    } else if (co->isPlaced()) {
        cell->place(co->placementX(),
                    co->placementY(),
                    isR90(co->placementOrient()),
                    isFlipX(co->placementOrient()),
                    isFlipY(co->placementOrient()));
        cell->fixed(false);
    } else if (co->isFixed()) {
        cell->place(co->placementX(),
                    co->placementY(),
                    isR90(co->placementOrient()),
                    isFlipX(co->placementOrient()),
                    isFlipY(co->placementOrient()));
        cell->fixed(true);
    }
    return 0;
}

//-----Pin-----
int readDefPin(defrCallbackType_e c, defiPin* dpin, defiUserData ud) {
    Database& db = getDBFromUD(ud);

    char direction = 'x';
    if (!strcmp(dpin->direction(), "INPUT")) {
        // INPUT to the chip, output from external
        direction = 'o';
    } else if (!strcmp(dpin->direction(), "OUTPUT")) {
        // OUTPUT to the chip, input to external
        direction = 'i';
    } else {
        printlog(LOG_WARN, "unknown pin signal direction: %s", dpin->direction());
    }

    const string& name = dpin->pinName();
    IOPin* iopin = db.addIOPin(name, string(dpin->netName()), direction);

    if (dpin->hasPlacement()) {
        iopin->x = dpin->placementX();
        iopin->y = dpin->placementY();
    }
    const int orient = dpin->orient();
    const bool r90 = isR90(orient);
    const bool flipX = isFlipX(orient);
    const bool flipY = isFlipY(orient);

    if (dpin->hasLayer()) {
        for (int i = 0; i < dpin->numLayer(); i++) {
            int lx, ly, hx, hy;
            dpin->bounds(i, &lx, &ly, &hx, &hy);
            int nlx = 0;
            int nly = 0;
            int nhx = 0;
            int nhy = 0;
            if (r90) {
                nlx = -hy;
                nly = lx;
                nhx = -ly;
                nhy = hx;
                lx = nlx;
                ly = nly;
                hx = nhx;
                hy = nhy;
            }
            if (flipX) {
                nlx = -hx;
                nhx = -lx;
                lx = nlx;
                hx = nhx;
            }
            if (flipY) {
                nly = -hy;
                nhy = -ly;
                ly = nly;
                hy = nhy;
            }
            iopin->type->addShape(db.getLayer(dpin->layer(i)), lx, ly, hx, hy);
        }
    } else {
        printlog(LOG_WARN, "IOPin %s has no layer", name.c_str());
    }

    return 0;
}

int readDefNetWire(defiNet* dnet, defiUserData ud, Net* net, const bool updatePin) {
    Database& db = getDBFromUD(ud);
    int path = DEFIPATH_DONE;
    // const Layer* layer = nullptr;
    Layer* layer = nullptr;
    const Layer* layerAbove = nullptr;
    const Layer* layerBelow = nullptr;
    const ViaType* viaType = nullptr;
    int width = -1;
    int fromx = 0;
    int fromy = 0;
    int fromz = -1;
    int tox = 0;
    int toy = 0;
    int toz = -1;
    unsigned nNodes = 0;
    for (unsigned i = 0; static_cast<int>(i) < dnet->numWires(); ++i) {
        const defiWire* dwire = dnet->wire(i);
        for (unsigned j = 0; static_cast<int>(j) < dwire->numPaths(); ++j) {
            const defiPath* dpath = dwire->path(j);
            dpath->initTraverse();
            while ((path = dpath->next()) != DEFIPATH_DONE) {
                switch (path) {
                    case DEFIPATH_LAYER:
                        fromx = 0;
                        fromy = 0;
                        fromz = -1;
                        tox = 0;
                        toy = 0;
                        toz = -1;
                        nNodes = 0;
                        layer = db.getLayer(dpath->getLayer());
                        width = layer->width;
                        for (const Layer* cLayer : db.layers) {
                            if (cLayer->rIdx == layer->rIdx - 1) {
                                layerBelow = cLayer;
                            } else if (cLayer->rIdx == layer->rIdx + 1) {
                                layerAbove = cLayer;
                            }
                        }
                        break;
                    case DEFIPATH_VIA:
                        viaType = db.getViaType(dpath->getVia());
                        for (const Geometry& geo : viaType->rects) {
                            if (geo.layer->rIdx >= 0) {
                                // via has two routing layers; one at dpath->layer(), second is above or below
                                if (geo.layer->rIdx > layer->rIdx) {  // second via layer is above
                                    net->addNode({layer, fromx, fromy, -1}, updatePin);
                                    net->addNode({layerAbove, fromx, fromy, -1}, updatePin);
                                    net->addWire({layer, fromx, fromy, -1}, {layerAbove, fromx, fromy, -1}, -1);
                                } else if (geo.layer->rIdx < layer->rIdx) {  // second via layer is below
                                    net->addNode({layer, fromx, fromy, -1}, updatePin);
                                    net->addNode({layerBelow, fromx, fromy, -1}, updatePin);
                                    net->addWire({layer, fromx, fromy, -1}, {layerBelow, fromx, fromy, -1}, -1);
                                }
                            }
                        }
                        break;
                    case DEFIPATH_VIAROTATION:
                        break;
                    case DEFIPATH_WIDTH:
                        width = dpath->getWidth();
                        break;
                    case DEFIPATH_POINT:
                        if (nNodes) {
                            dpath->getPoint(&tox, &toy);
                            net->addNode({layer, fromx, fromy, fromz}, updatePin);
                            net->addNode({layer, tox, toy, -1}, updatePin);
                            net->addWire({layer, fromx, fromy, fromz}, {layer, tox, toy, -1}, width);
                            fromx = tox;
                            fromy = toy;
                            fromz = -1;
                        } else {
                            dpath->getPoint(&fromx, &fromy);
                            fromz = -1;
                        }
                        ++nNodes;
                        break;
                    case DEFIPATH_FLUSHPOINT:
                        if (nNodes) {
                            dpath->getFlushPoint(&tox, &toy, &toz);
                            net->addNode({layer, fromx, fromy, fromz}, updatePin);
                            net->addNode({layer, tox, toy, toz}, updatePin);
                            net->addWire({layer, fromx, fromy, fromz}, {layer, tox, toy, toz}, width);
                            fromx = tox;
                            fromy = toy;
                            fromz = toz;
                        } else {
                            dpath->getFlushPoint(&fromx, &fromy, &fromz);
                        }
                        ++nNodes;
                        break;
                    case DEFIPATH_TAPER:
                        break;
                    case DEFIPATH_RECT: {
                        int lx = 0;
                        int ly = 0;
                        int hx = 0;
                        int hy = 0;
                        dpath->getViaRect(&lx, &ly, &hx, &hy);
                        net->shapes.emplace_back(layer, fromx + lx, fromy + ly, fromx + hx, fromy + hy);
                        break;
                    }
                    case DEFIPATH_TAPERRULE:
                        break;
                    case DEFIPATH_STYLE:
                        //  printf("STYLE %d ", dpath->getStyle());
                        break;
                }
            }
        }
    }

    return 0;
}

//-----Net-----
int readDefNet(defrCallbackType_e c, defiNet* dnet, defiUserData ud) {
    Database& db = getDBFromUD(ud);
    NDR* ndr = nullptr;

    if (dnet->hasNonDefaultRule()) {
        string designrulename(dnet->nonDefaultRule());
        ndr = db.getNDR(designrulename);
        if (!ndr) {
            printlog(LOG_WARN, "NDR rule is not found: %s", designrulename.c_str());
        }
    }

    Net* net = db.addNet(dnet->name(), ndr, db.nLayers);

    for (unsigned i = 0; i != (unsigned)dnet->numConnections(); ++i) {
        Pin* pin = db.getPinFromConnection(dnet->instance(i), dnet->pin(i));
        pin->net(net);
        net->addPin(pin);
    }
    return readDefNetWire(dnet, ud, net, true);
}

//-----Region-----
int readDefRegion(defrCallbackType_e c, defiRegion* dreg, defiUserData ud) {
    char type = 'f';
    if (dreg->hasType()) {
        if (!strcmp(dreg->type(), "FENCE")) {
            type = 'f';
        } else if (!strcmp(dreg->type(), "GUIDE")) {
            type = 'g';
        } else {
            printlog(LOG_WARN, "Unknown region type: %s", dreg->type());
        }
    } else {
        printlog(LOG_WARN, "Region is defined without type, use default region type = FENCE");
    }

    Region* region = getDBFromUD(ud).addRegion(string(dreg->name()), type);

    for (int i = 0; i < dreg->numRectangles(); i++) {
        region->addRect(dreg->xl(i), dreg->yl(i), dreg->xh(i), dreg->yh(i));
    }
    return 0;
}

//-----Group-----

int readDefGroupMember(defrCallbackType_e c, const char* cl, defiUserData ud) {
    getDBFromUD(ud).regions[0]->members.emplace_back(cl);
    return 0;
}

int readDefGroup(defrCallbackType_e c, defiGroup* dgp, defiUserData ud) {
    Database& db = getDBFromUD(ud);
    if (dgp->hasRegionName()) {
        string regionname(dgp->regionName());
        Region* region = db.getRegion(regionname);
        if (!region) {
            printlog(LOG_WARN, "Region is not found: %s", regionname.c_str());
            return 1;
        }
        region->members = db.regions[0]->members;
        db.regions[0]->members.clear();
    }
    return 0;
}

//-----Special Net-----
int readDefSNetStart(defrCallbackType_e c, int num, defiUserData ud) {
    getDBFromUD(ud).snets.reserve(num);
    return 0;
}

int readDefSNet(defrCallbackType_e c, defiNet* dnet, defiUserData ud) {
    Database& db = getDBFromUD(ud);
    Use::UseEnum use = Use::UseEnum::None;
    if (dnet->hasUse()) use = Use::getUse(dnet->use());
    Net* net = db.addSNet(dnet->name(), use);
    for (unsigned i = 0; static_cast<int>(i) < dnet->numConnections(); ++i) {
        const string& instance(dnet->instance(i));
        const string& pinname(dnet->pin(i));
        if (instance == "*") {
            for (Cell* cell : db.cells) {
                Pin* pin = cell->pin(pinname);
                if (!pin) continue;

                pin->net(net);
                net->addPin(pin);
            }
            continue;
        }

        Pin* pin = db.getPinFromConnection(instance, pinname);
        if (!pin) {
            printlog(LOG_ERROR, "Pin %s is not found in instance %s", pinname.c_str(), instance.c_str());
            continue;
        }

        pin->net(net);
        net->addPin(pin);
    }
    return readDefNetWire(dnet, ud, net, false);
}
