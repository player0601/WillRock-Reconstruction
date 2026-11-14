// wr_sdk.h -- Will Rock (2003) UNOFFICIAL modding SDK umbrella header. Include this one file in a mod.
// Build the mod 32-bit (MSVC x86 / mingw -m32). Addresses are for 1.2 (v12) unless a comment says feb.
#pragma once
#include "core/wr_core.h"
// ---- math_app
#include "math_app/app.h"
#include "math_app/m3d.h"
// ---- engine_core
#include "engine_core/anim.h"
#include "engine_core/collision.h"
#include "engine_core/core_types.h"
#include "engine_core/entity.h"
#include "engine_core/fileio.h"
#include "engine_core/object.h"
#include "engine_core/physics.h"
#include "engine_core/pssheet.h"
// ---- engine_render
#include "engine_render/d3d_driver.h"
#include "engine_render/particle.h"
#include "engine_render/rend_driver.h"
#include "engine_render/scene.h"
#include "engine_render/shader.h"
#include "engine_render/texture.h"
// ---- platform_drv
#include "platform_drv/app_win.h"
#include "platform_drv/input.h"
#include "platform_drv/sound_drv.h"
// ---- game_systems
#include "game_systems/cinema.h"
#include "game_systems/domains.h"
#include "game_systems/gs_common.h"
#include "game_systems/input.h"
#include "game_systems/level.h"
#include "game_systems/render.h"
#include "game_systems/saveload.h"
#include "game_systems/sound.h"
#include "game_systems/strings.h"
#include "game_systems/vis.h"
// ---- game_shared
#include "game_shared/damage.h"
#include "game_shared/weapon_base.h"
#include "game_shared/world_objects.h"
// ---- player
#include "player/player.h"
// ---- weapons
#include "weapons/projectiles.h"
#include "weapons/weapons.h"
// ---- ai
#include "ai/ai_npc.h"
#include "ai/ai_world.h"
#include "ai/enemies.h"
// ---- sfx_sob
#include "sfx_sob/particles.h"
#include "sfx_sob/sfx_api.h"
#include "sfx_sob/sfx_entities.h"
#include "sfx_sob/sob_entities.h"
// ---- frontend
#include "frontend/ui_core.h"
#include "frontend/wr_game.h"
#include "frontend/wr_ui.h"
