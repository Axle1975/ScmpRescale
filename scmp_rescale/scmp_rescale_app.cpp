#include <iostream>
#include <fstream>
#include <sstream>

#include "scmp_rescale_window.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    ScmpRescaleWindow mw;
    mw.show();
    a.exec();
}
