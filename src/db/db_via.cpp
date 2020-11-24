#include "db.h"
using namespace db;

/***** Via *****/
Via::Via(){
}

Via::Via(ViaType *type, int x, int y){
    this->x = x;
    this->y = y;
    this->type = type;
}

/***** ViaType *****/
ViaType::ViaType(const string& name, const bool isDef)
    : isDef_(isDef)
    , name(name)
{
}

