// Keep one ccache entry per job: delete the older <prefix><commit> entries,
// but only when the new key is in the cache, so a restore always finds one.
const { execFileSync } = require("node:child_process");

const key = process.env.INPUT_KEY || "";
const prefix = process.env.INPUT_PREFIX || "";
const repo = process.env.GITHUB_REPOSITORY || "";
const ref = process.env.GITHUB_REF || "";
const env = { ...process.env, GH_TOKEN: process.env.INPUT_TOKEN || "" };

function gh(args) {
	return execFileSync("gh", args, { env, encoding: "utf8" });
}

// The commit suffix keeps another job's longer prefix out of this list:
// ccache-linux- must not match ccache-linux-<other>-<commit>.
function own_entries() {
	const list = JSON.parse(gh(["cache", "list", "--repo", repo, "--ref", ref,
		"--key", prefix, "--limit", "100", "--json", "id,key"]));
	return list.filter((e) => e.key.startsWith(prefix)
		&& /^[0-9a-f]{40}$/.test(e.key.slice(prefix.length)));
}

try {
	const entries = own_entries();
	if (!entries.some((e) => e.key === key)) {
		console.log(`::warning::ccache-prune: ${key} is not in the cache, so the older entries stay`);
	} else {
		for (const e of entries) {
			if (e.key === key) continue;
			gh(["cache", "delete", String(e.id), "--repo", repo]);
			console.log(`ccache-prune: deleted ${e.key}`);
		}
	}
} catch (err) {
	// A token without actions write (a fork pull request) only skips the prune.
	console.log(`::warning::ccache-prune: ${err.message.split("\n")[0]}`);
}
