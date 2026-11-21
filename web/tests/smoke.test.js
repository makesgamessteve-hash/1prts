/* 1stPRTS browser tests: the commander-is-there smoke set (walk, stairs, door, lift, war table;
 * task 10.5) run on the web build. The set is every scenario in tests/game/scenarios/ with a
 * "-- ctest-labels: smoke" line; CTest runs the same files headless on Windows, Linux and macOS
 * (`ctest -L smoke`), where each must end with the commander in the cell its `final_cell`
 * names. Here the page runs each one headless (?test=1&scenario=<name>) and must report it as
 * passed with that same final cell (spec "Same scenario result on desktop and web"). MIT License. */
'use strict';
const fs = require('fs');
const path = require('path');
const { expect, newPage, waitFor } = require('./lib/harness');

const SCENARIOS = path.resolve(__dirname, '..', '..', 'tests', 'game', 'scenarios');
const RUN_TIMEOUT = 120000;

function smokeSet() {
  return fs.readdirSync(SCENARIOS).filter((f) => f.endsWith('.lua')).sort().map((file) => {
    const text = fs.readFileSync(path.join(SCENARIOS, file), 'utf8');
    if (!/^-- ctest-labels:.*\bsmoke\b/m.test(text)) return null;
    const cell = /^\s*final_cell\s*=\s*"([^"]+)"/m.exec(text);
    return { name: path.basename(file, '.lua'), cell: cell ? cell[1] : null };
  }).filter(Boolean);
}

const SET = smokeSet();
// The set the task names: walking, stairs, a door, the lift and the war table.
for (const required of ['walk_speed', 'keep_stairs', 'keep_door', 'keep_lift', 'war_table']) {
  if (!SET.some((s) => s.name === required)) throw new Error(`smoke scenario ${required} is missing from ${SCENARIOS}`);
}

module.exports = SET.map((s) => ({
  name: `${s.name} passes with the final cell ${s.cell}`,
  async run(env, context) {
    expect(s.cell, `${s.name}.lua names no final_cell, so desktop and web results cannot be compared`);
    const page = await newPage(context);
    await page.goto(`${env.server.url}1stprts.html?test=1&scenario=${s.name}`);
    await waitFor(page, () => window.prts_test && ['passed', 'failed'].includes(window.prts_test.scenario().state),
      null, `the scenario ${s.name} to end`, RUN_TIMEOUT);
    const result = await page.evaluate(() => window.prts_test.scenario());
    expect(result.state === 'passed', `${s.name} failed on the web: ${result.message}`);
    expect(result.cell === s.cell, `${s.name} ended in cell "${result.cell}", the desktop run in "${s.cell}"`);
    expect(/^[0-9a-f]{16}$/.test(result.hash), `no state hash reported (${result.hash})`);
    console.log(`      ${s.name}: ${result.ticks} ticks, final cell ${result.cell}, state hash ${result.hash}`);
    expect(page.prtsErrors.length === 0, `errors on the page: ${page.prtsErrors.join(' | ')}`);
  },
}));
