#include "UnrealProspero/Oodle/Ctype.h"
#include <ctype.h>
#include <clocale>
#include <stdexcept>

int main(int argc, char** argv)
{
    if (argc < 2) throw std::invalid_argument("Specify locales to verify");
    for (int argument = 1; argument < argc; ++argument)
    {
        if (std::setlocale(LC_ALL, argv[argument]) == nullptr) throw std::runtime_error("Cannot select test locale");
        const auto* classification = *UnrealProsperoOodleCtype();
        const auto* lowercase = *UnrealProsperoOodleLowercase();
        const auto* uppercase = *UnrealProsperoOodleUppercase();
        for (int value = -128; value < 256; ++value)
        {
            if (classification[value] != (*__ctype_b_loc())[value]) throw std::runtime_error("Character classification differs from glibc");
            if (lowercase[value] != (*__ctype_tolower_loc())[value]) throw std::runtime_error("Lowercase conversion differs from glibc");
            if (uppercase[value] != (*__ctype_toupper_loc())[value]) throw std::runtime_error("Uppercase conversion differs from glibc");
        }
    }
    return 0;
}
