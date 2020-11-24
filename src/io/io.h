#ifndef _IO_IO_H_
#define _IO_IO_H_

#include <string>

namespace io {
class IOModule {
public:
    enum class Flow : unsigned char { extract, form };

private:
    static std::string _name;

    void showOptions() const;

public:
    static std::string Format;
    static IOModule::Flow IOFlow;

    static std::string BookshelfAux;
    static std::string BookshelfPl;
    static std::string BookshelfVariety;
    static std::string BookshelfPlacement;

    static std::string LefTech;
    static std::string LefCell;
    static std::string DefFloorplan;
    static std::string DefCell;
    static std::string DefPlacement;

    static std::string Liberty;

    static std::string NetDetail;
    static std::string TimePath;
    static std::string TimeUnconstrain;

public:
    const std::string& name() const { return _name; }
    static bool load();

    static int writeDir(const std::string& file);
    static std::ofstream write(const std::string& file, const bool verbose = false);
};
}  // namespace io

#endif
