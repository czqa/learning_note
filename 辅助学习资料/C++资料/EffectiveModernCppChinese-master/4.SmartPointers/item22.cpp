#include "Widget.h"

int main()
{
    {
        Widget w;
        Widget w2(std::move(w));
    }
    return 0;
}