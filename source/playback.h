#pragma once

#include "nlohmann/json.hpp"
#include "jaffar.h"
#include "utils.h"
#include "SDLPopInstance.h"
#include <vector>

class Playback
{

public:

 Playback();
 void play();

private:

 bool _pauseAfterFrame;
 bool _produceReplay;
 size_t _frameDuration;
};

