#include <iostream>
#include <fstream>
#include <sstream>


#include "scmp/scmp.h"
#include "scmp_rescale_window.h"

using namespace nfa::scmp;

int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    ScmpRescaleWindow mw;
    mw.show();
    a.exec();
}
