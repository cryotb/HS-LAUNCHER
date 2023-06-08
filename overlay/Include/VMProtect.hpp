#pragma once

namespace vmp
{

}

#define VMP_BEGIN_ULTRA(NAME) VMProtectBeginUltra(_XS(NAME))
#define VMP_END() VMProtectEnd()
