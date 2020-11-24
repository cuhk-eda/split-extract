#include "io.h"
#include <sys/stat.h>
#include "../db/db.h"
#include "../global.h"

using namespace io;

std::string IOModule::_name = "io";

std::string IOModule::Format = "lefdef";
IOModule::Flow IOModule::IOFlow = IOModule::Flow::extract;
std::string IOModule::BookshelfAux = "";
std::string IOModule::BookshelfPl = "";
std::string IOModule::BookshelfVariety = "";
std::string IOModule::BookshelfPlacement = "";

std::string IOModule::LefTech = "";
std::string IOModule::LefCell = "";
std::string IOModule::DefFloorplan = "";
std::string IOModule::DefCell = "";
std::string IOModule::DefPlacement = "";

std::string io::IOModule::Liberty = "";

std::string io::IOModule::NetDetail = "";
std::string io::IOModule::TimePath = "";
std::string io::IOModule::TimeUnconstrain = "";

void IOModule::showOptions() const {
    printlog(LOG_INFO, "format              : %s", IOModule::Format.c_str());
    printlog(LOG_INFO, "flow                : %u", static_cast<unsigned>(IOModule::IOFlow));
    printlog(LOG_INFO, "bookshelfAux        : %s", IOModule::BookshelfAux.c_str());
    printlog(LOG_INFO, "bookshelfPl         : %s", IOModule::BookshelfPl.c_str());
    printlog(LOG_INFO, "bookshelfVariety    : %s", IOModule::BookshelfVariety.c_str());
    printlog(LOG_INFO, "lefTech             : %s", IOModule::LefTech.c_str());
    printlog(LOG_INFO, "lefCell             : %s", IOModule::LefCell.c_str());
    printlog(LOG_INFO, "defFloorplan        : %s", IOModule::DefFloorplan.c_str());
    printlog(LOG_INFO, "defCell             : %s", IOModule::DefCell.c_str());
    printlog(LOG_INFO, "defPlacement        : %s", IOModule::DefPlacement.c_str());
    printlog(LOG_INFO, "liberty             : %s", IOModule::Liberty.c_str());
    printlog(LOG_INFO, "netDetail           : %s", IOModule::NetDetail.c_str());
}

bool io::IOModule::load() {
    if (BookshelfAux.length() > 0 && BookshelfPl.length()) {
        Format = "bookshelf";
        database.readBSAux(BookshelfAux, BookshelfPl);
    }
    if (LefTech.length()) {
        Format = "lefdef";
        database.readLEF(LefTech);
    }
    if (LefCell.length()) {
        Format = "lefdef";
        database.readLEF(LefCell);
    }
    if (DefCell.length()) {
        Format = "lefdef";
        database.readDEF(DefCell);
    } else if (DefFloorplan.length()) database.readDEF(DefFloorplan);

    if (Liberty.length()) database.readLiberty(Liberty);
    return true;
}

int io::IOModule::writeDir(const string& file) {
    size_t pos0 = file.find_last_of('/');
    if (pos0 != string::npos) {
        writeDir(file.substr(0, pos0));
    }

    int status = mkdir(file.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status) {
        switch (errno) {
            case ENOENT:
                printlog(LOG_ERROR,
                         "A component of the path prefix specified by %s does not name an existing directory or %s is "
                         "an empty string.",
                         file.c_str(),
                         file.c_str());
                return ENOENT;
            case EEXIST:
                return 0;
            default:
                printlog(LOG_ERROR, "mkdir failed on %s with error number: %d", file.c_str(), errno);
                return errno;
        }
    }
    return 0;
}

ofstream io::IOModule::write(const string& file, const bool verbose) {
    size_t pos0 = file.find_last_of('/');
    if (pos0 != string::npos) {
        writeDir(file.substr(0, pos0));
    }

    ofstream ofs(file.c_str());
    if (!ofs.good()) {
        printlog(LOG_ERROR, "Unable to create/open DEF: %s", file.c_str());
    }
    if (verbose) {
        printlog(LOG_INFO, "writing %s", file.c_str());
    }
    return ofs;
}

