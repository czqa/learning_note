#include <iostream>

int mian()
{
    const int theAnswer = 42;

    auto x = theAnswer;
    auto y = &theAnswer;
    return 0;
}