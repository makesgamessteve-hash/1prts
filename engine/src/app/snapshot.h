/* 1stPRTS engine: saves (versioned binary snapshots of the simulation) and replays. MIT License. */
#ifndef PRTS_SNAPSHOT_H
#define PRTS_SNAPSHOT_H

#include "prts/prts_base.h"
#include "prts/prts_rng.h"
#include "prts/prts_serial.h"

typedef struct prts_engine prts_engine;

#define PRTS_SNAPSHOT_FORMAT 1u
#define PRTS_REPLAY_FORMAT 1u

/* A snapshot holds the simulation state on top of the booted map: every entity slot (parents,
 * local transforms, velocities, movers, usable flags), which collision boxes are enabled, the
 * commander, the script random stream, the tick and every living entity's script data
 * (prts.entity.data), plus the mounted mods. Geometry built by scripts is not included: a
 * snapshot is loaded into an engine that booted the same map. Script data must be plain data;
 * writing fails and err names the offending field otherwise. */
bool prts_engine_snapshot_write(prts_engine *e, prts_writer *w, char *err, size_t err_size);
/* Checks a snapshot and, unless dry_run, applies it. Nothing changes when it fails. */
bool prts_engine_snapshot_read(prts_engine *e, const void *data, size_t size, bool dry_run, char *err,
                               size_t err_size);
bool prts_engine_save(prts_engine *e, const char *real_path, char *err, size_t err_size);
bool prts_engine_load(prts_engine *e, const char *real_path, char *err, size_t err_size);
/* Adds every living entity's script data to a state hash (functions etc. by type only). */
void prts_engine_hash_script_data(const prts_engine *e, prts_hasher *h);
/* prts.save.write / prts.save.load (slots in the user area's saves/ folder). */
void prts_engine_open_save_api(prts_engine *e);

/* Replays: a start snapshot plus each tick's simulation inputs (action values and presses,
 * the commander's look yaw and pitch) and the state hash after the tick. --record writes one
 * when the engine is destroyed; --replay plays one back, checking every tick's hash, and stops
 * with exit code 1 at the first tick that differs. Pending save loads are applied here too. */
bool prts_engine_replay_start(prts_engine *e, char *err, size_t err_size);
void prts_engine_replay_before_tick(prts_engine *e);
void prts_engine_replay_after_tick(prts_engine *e);
bool prts_engine_replaying(const prts_engine *e);
/* Writes the recording (if any) and frees replay state. */
void prts_engine_replay_finish(prts_engine *e);

#endif
