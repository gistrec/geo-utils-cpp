# Releasing geo-utils-cpp

The full checklist for cutting version `X.Y.Z` and rolling it out to every
distribution channel. Time budget: ~30 minutes of work plus CI waits;
registry PRs then merge on their own schedule.

## 1. Bump the version

Grep first — this list has grown before: `grep -rn "<previous version>"
--exclude-dir=.git .`

- [ ] `CMakeLists.txt` — `project(GeoUtilsCpp VERSION X.Y.Z ...)`
- [ ] `manifest` (build2) — `version: X.Y.Z`
- [ ] `include/geo/version.hpp` — the three `GEO_UTILS_CPP_VERSION_*`
      components (the test suite fails if this disagrees with CMake)
- [ ] `README.md` — FetchContent `GIT_TAG`, `conan install --requires`,
      build2 `depends:`, `find_package` example; re-check the
      "N KB across M headers" line if headers changed
- [ ] `docs/getting-started.md` — same four spots
- [ ] `CHANGELOG.md` — new `## vX.Y.Z` section at the top (Added / Fixed /
      Docs / Unchanged), following the existing tone

## 2. Verify, push, tag, release

```sh
cmake -S . -B build-rel -DGEO_UTILS_CPP_BUILD_TESTS=ON && \
    cmake --build build-rel -j && ctest --test-dir build-rel
git push origin master          # or merge the release PR
```

- [ ] Wait for **all** workflows on the release commit to go green
      (`gh run list --commit <sha>`) — tag only a green commit.
- [ ] Annotated tag + GitHub release. Pushing the tag triggers the
      `Release assets` workflow, which creates a **draft** release with the
      notes extracted from `CHANGELOG.md` and the single-header `geo.hpp`
      attached (the repository uses **immutable releases**, so assets must
      be attached before publishing — and only a pre-publish draft allows
      that). Review the draft, then publish:

```sh
git tag -a vX.Y.Z -m "geo-utils-cpp vX.Y.Z — <one-line summary>"
git push origin vX.Y.Z
# wait ~1 min for the draft to appear with notes + geo.hpp:
gh release view vX.Y.Z --json isDraft,assets --jq '{draft: .isDraft, assets: [.assets[].name]}'
gh release edit vX.Y.Z --draft=false
```

Fallback if the workflow failed: `python3 tools/amalgamate.py -o geo.hpp`,
then `gh release create vX.Y.Z --draft --title "vX.Y.Z" --notes-file
<notes.md> geo.hpp` and publish with `--draft=false`.

## 3. Hashes of the source tarball

Both registries hash the same auto-generated archive:

```sh
curl -sL https://github.com/gistrec/geo-utils-cpp/archive/refs/tags/vX.Y.Z.tar.gz -o vX.Y.Z.tar.gz
shasum -a 256 vX.Y.Z.tar.gz   # conan, xmake
shasum -a 512 vX.Y.Z.tar.gz   # vcpkg
```

## 4. Registries

### vcpkg (microsoft/vcpkg)

Sparse-clone upstream; the `gistrec/vcpkg` fork is only a push target — its
stale master never matters:

```sh
git clone --filter=blob:none --sparse --depth 1 https://github.com/microsoft/vcpkg.git
cd vcpkg && git sparse-checkout set ports/geo-utils-cpp versions/g-   # baseline.json comes along (cone mode)
git checkout -b geo-utils-cpp-X.Y.Z
```

- [ ] `ports/geo-utils-cpp/vcpkg.json` — `"version": "X.Y.Z"`
- [ ] `ports/geo-utils-cpp/portfile.cmake` — new `SHA512`
- [ ] `versions/baseline.json` — bump the `geo-utils-cpp` baseline
- [ ] `versions/g-/geo-utils-cpp.json` — prepend an entry; `git-tree` is
      `git rev-parse HEAD:ports/geo-utils-cpp` **after committing** the port
      change (amend the versions files into the same commit)
- [ ] Verify: `git sparse-checkout disable && ./bootstrap-vcpkg.sh
      -disableMetrics && ./vcpkg x-add-version geo-utils-cpp` must print
      "already in … No files were updated"; then `./vcpkg install
      geo-utils-cpp` (downloads, checks the SHA512, installs)
- [ ] `git remote add fork git@github.com:gistrec/vcpkg.git && git push fork
      HEAD` and `gh pr create --repo microsoft/vcpkg --head gistrec:<branch>`
      — title `[geo-utils-cpp] Update to X.Y.Z`

### Conan (conan-io/conan-center-index)

While the initial recipe PR (#30152) is still open: update **that branch**
(`gistrec:geo-utils-cpp/1.0.1`, sparse clone of the fork) so it carries only
the latest version in `recipes/geo-utils-cpp/{config.yml,all/conandata.yml}`
(url `archive/vX.Y.Z.tar.gz` + sha256), push, and leave a PR comment. Each
push resets the first-time-contributor CI approval — bump deliberately, not
for every trifle. After the recipe merges, version bumps become ordinary
small PRs.

### xmake / xrepo (xmake-io/xmake-repo)

**PRs target the `dev` branch** (default), not master.

```sh
git clone --filter=blob:none --sparse git@github.com:gistrec/xmake-repo.git
cd xmake-repo && git sparse-checkout set packages/g/geo-utils-cpp scripts
git remote add upstream https://github.com/xmake-io/xmake-repo.git
git fetch --depth 2 upstream dev && git checkout -b geo-utils-cpp-X.Y.Z upstream/dev
```

- [ ] Add `add_versions("X.Y.Z", "<sha256>")` (no `v` prefix) on top of the
      list in `packages/g/geo-utils-cpp/xmake.lua`
- [ ] Verify locally (needs ≥2 commits of history — the script diffs
      `HEAD^`): commit first, then `xmake l scripts/test.lua -D -k
      geo-utils-cpp`
- [ ] Push to the fork, `gh pr create --repo xmake-io/xmake-repo --base dev`

### build2 / cppget

From the repo root (bdep config `@cfg` lives in
`../geo-utils-cpp-cfg/`); the working tree must be clean and at the tag:

```sh
bdep publish --yes        # default section; do NOT pass --section=testing (rejected)
```

The submission lands in the queue (`queue.cppget.org/libgeo-utils-cpp/X.Y.Z`)
and auto-promotes into the `testing` repository after a successful queue
build; the queue URL 404s once promoted. `stable` promotion is a separate,
maintainer-gated process.

## 5. Afterwards

- [ ] Double-check the published release shows the single-header `geo.hpp`
      asset (it must be attached while the release is still a draft — see
      step 2; immutable releases cannot gain assets after publishing).
- [ ] Verify the release page, the four registry submissions, and that the
      README badges still resolve.
- [ ] When registry PRs merge, spot-check one install path
      (`vcpkg install geo-utils-cpp`, `xrepo install geo-utils-cpp`, …).
