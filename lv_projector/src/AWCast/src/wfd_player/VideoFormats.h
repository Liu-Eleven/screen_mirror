#include <stdbool.h>
struct config_t {
    size_t width, height, framesPerSecond;
    bool interlaced;
    unsigned char profile, level;
};

enum ProfileType {
    PROFILE_CBP = 0,
    PROFILE_CHP,
    kNumProfileTypes,
};

enum LevelType {
    LEVEL_31 = 0,
    LEVEL_32,
    LEVEL_40,
    LEVEL_41,
    LEVEL_42,
    kNumLevelTypes,
};

enum ResolutionType {
    RESOLUTION_CEA,
    RESOLUTION_VESA,
    RESOLUTION_HH,
    kNumResolutionTypes,
};
