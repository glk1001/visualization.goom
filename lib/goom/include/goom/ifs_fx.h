#ifndef IFS_FX_H
#define IFS_FX_H

/*
 * File created 11 april 2002 by JeKo <jeko@free.fr>
 */

#include "goom_visual_fx.h"
#include "sound_info.h"

namespace goom
{

VisualFX ifs_visualfx_create(void);
void ifsRenew(VisualFX* _this, const SoundInfo&);

} // namespace goom
#endif
