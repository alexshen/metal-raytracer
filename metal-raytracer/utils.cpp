#include "utils.h"

#include <cstdlib>

namespace utils
{

float random()
{
    return (float)std::rand() / RAND_MAX;
}

}