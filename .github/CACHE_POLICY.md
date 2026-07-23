# CI cache policy

GitHub Actions gives this repository 10 GiB of cache storage. Cache entries
must remain useful without allowing one main-branch generation plus its
replacements to cross that limit.

## Cache classes

### Compiler output

Each ccache-producing job declares one `CCACHE_KEY`. Pull requests restore the
latest matching snapshot from `main` but never save one. On successful pushes,
`finalize-ccache` trims stale objects, saves a run-ID snapshot, verifies it is
visible, and deletes its predecessor.

The five job families whose working sets reached 500 MB use a 1 GB local limit
and a three-day trim. Smaller families use the action's 500 MB default and a
seven-day trim. The limit provides build-time headroom; trimming controls the
persisted size.

### Incremental analysis

Clang-tidy uses ctcache snapshots keyed by toolkit and configuration hash. The
repository janitor keeps the newest snapshot per ref and key family, so a pull
request can reuse its own previous analysis without accumulating every run.

### Content-addressed dependencies

Windows dependencies, Android vcpkg binaries, Gradle data, and BSD VM images
use keys derived from their inputs. Branch-scoped Windows and BSD entries are
removed once `main` has the same key and cache version.

## Repository maintenance

Before jobs fan out, cache schemas that current jobs cannot restore are removed.
After all cache producers finish, the janitor removes superseded snapshots and
branch copies shadowed by `main`, then warns if usage remains above 9 GiB.

When adding a cache:

1. Include every compiler, platform, configuration, or dependency input that
   changes whether an entry can be reused.
2. Use a content-addressed key for immutable dependencies.
3. Use `CCACHE_KEY` and `finalize-ccache` for evolving compiler output.
4. Add the producing job to the janitor's `needs` list.
5. Account for its steady-state size within the 9 GiB warning threshold.
