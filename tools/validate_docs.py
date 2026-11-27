"""Offline documentation checks and optional isolated public C17 examples.

This checks inventories/links/signatures, not prose correctness. No dependencies
beyond Python's standard library; CMake/compiler are needed only for --examples.
"""
from __future__ import annotations

import argparse
import collections
from pathlib import Path
import re
import subprocess
import tempfile
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
BASELINE = "3f99029da73e3b4f0a75a538dd241cf94cda23d7"
TAG_OBJECT = "165b1a72f2c689c78d271d4ab61651e6eefc217e"
GUIDES = ["USAGE.zh-CN.md", "IMPLEMENTATION_WALKTHROUGH.zh-CN.md",
          "IMPLEMENTATION_API_REFERENCE.zh-CN.md", "IMPLEMENTATION_INTERNALS.zh-CN.md",
          "IMPLEMENTATION_VALIDATION.zh-CN.md", "DEVELOPMENT.zh-CN.md",
          "HANDOFF.md", "TECHNICAL_DEBT.md"]
PRODUCTION = ["ordweave.c", "lks_adapter.c", "persistence.c", "wal_format.c",
              "wal_windows.c", "sha256.c"]
INCLUDES = ["checkpoint_core.inc", "checkpoint_windows.inc", "history_windows.inc", "occurrence_index.inc", "field_index.inc"]
HEADERS = ["core_names.h", "core_internal.h", "lks_adapter.h", "wal_internal.h",
           "checkpoint_internal.h"]
DECL = re.compile(r"(?:const\s+char\s*\*|OwStatus\s+|void\s+)"
                  r"(ordweave_version|ow_\w+)\s*\([^;{}]*\)\s*;")


def command(*args: str, capture: bool = False) -> str:
    result = subprocess.run(args, cwd=ROOT, check=True, text=True,
                            encoding="utf-8", stdout=subprocess.PIPE if capture else None)
    return result.stdout.strip() if capture else ""


def normalize(s: str) -> str:
    return re.sub(r"\s+", "", s)


def declarations(s: str) -> dict[str, str]:
    s = re.sub(r"/\*.*?\*/", "", s, flags=re.S)
    return {m.group(1): normalize(m.group()) for m in DECL.finditer(s)}


def no_fences(s: str) -> str:
    return re.sub(r"^```[^\n]*\n.*?^```[^\n]*$", "", s, flags=re.M | re.S)


def anchors(s: str) -> set[str]:
    result = set(re.findall(r'<a\s+id="([^"]+)"', s))
    counts: collections.Counter[str] = collections.Counter()
    for title in re.findall(r"^#{1,6}\s+(.+?)\s*#*\s*$", no_fences(s), re.M):
        title = re.sub(r"<[^>]*>", "", title).lower()
        title = re.sub(r"[^\w\- ]", "", title).replace(" ", "-")
        suffix = f"-{counts[title]}" if counts[title] else ""
        result.add(title + suffix)
        counts[title] += 1
    return result


def check() -> None:
    for name in GUIDES:
        assert (ROOT / "docs" / name).is_file(), name
    for name in ("README.md", "README.zh-CN.md"):
        assert (ROOT / name).is_file(), name
    header = (ROOT / "include/ordweave.h").read_text(encoding="utf-8")
    api = (ROOT / "docs/IMPLEMENTATION_API_REFERENCE.zh-CN.md").read_text(encoding="utf-8")
    research = "cf342dcf881bc9e3518ede042baeaed25f807da7"
    historical_header = subprocess.check_output(["git", "show", f"{research}:include/ordweave.h"], cwd=ROOT).decode()
    historical_api = declarations(historical_header)
    approved_additions = declarations('OwStatus ow_field_index_enable(OwDatabase *db, const char *field,\n                              size_t max_records, size_t max_key_bytes);\nOwStatus ow_field_index_disable(OwDatabase *db);\nOwStatus ow_query_range(OwDatabase *db, const char *field, OwType type,\n                       const OwValue *lower, int lower_inclusive,\n                       const OwValue *upper, int upper_inclusive,\n                       OwRecordId **out, size_t *count);\nOwStatus ow_history_query_range(const OwHistory *view, const char *field,\n                       OwType type, const OwValue *lower, int lower_inclusive,\n                       const OwValue *upper, int upper_inclusive,\n                       OwRecordId **out, size_t *count);\n')
    assert len(historical_api) == 44 and len(approved_additions) == 4
    assert declarations(header) == historical_api | approved_additions, "48 APIs must be exact historical44 plus approved4"
    # Removing only the approved declarations must leave every historical
    # type, numeric status, field and preprocessor token unchanged.
    stripped = re.sub(r"/\*.*?\*/", "", header, flags=re.S)
    stripped = DECL.sub(lambda m: "" if m.group(1) in approved_additions else m.group(), stripped)
    old_tokens = re.sub(r"/\*.*?\*/", "", historical_header, flags=re.S)
    assert normalize(stripped) == normalize(old_tokens), "historical public types/statuses changed"
    expected = declarations(header)
    blocks = "\n".join(re.findall(r"```c\n(.*?)```", api, re.S))
    actual = declarations(blocks)
    assert expected == actual, f"public declarations differ: {set(expected) ^ set(actual)}"
    for name in expected:
        assert api.count(f'<a id="api-{name.replace("_", "-")}"></a>') == 1, name
    print(f"Public API: {len(expected)}/{len(expected)} exact signatures and unique entries")
    internal = (ROOT / "docs/IMPLEMENTATION_INTERNALS.zh-CN.md").read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    sources = re.search(r"add_library\(ordweave STATIC ([^)]+)\)", cmake).group(1).split()
    assert set(sources) == {f"src/{p}" for p in PRODUCTION}, sources
    for name in PRODUCTION + INCLUDES + HEADERS:
        assert f"../src/{name}" in internal, name
        assert (ROOT / "src" / name).is_file(), name
    for owner, includes in {"ordweave.c": [INCLUDES[0], INCLUDES[3], INCLUDES[4]], "wal_windows.c": INCLUDES[1:3]}.items():
        s = (ROOT / "src" / owner).read_text(encoding="utf-8")
        for inc in includes:
            assert f'#include "{inc}"' in s, (owner, inc)
    print("Production coverage: 6 C translation units, 5 owned implementation includes, 5 private headers")
    assert command("git", "rev-parse", "v1.0.0-rc.1^{}", capture=True) == BASELINE
    assert command("git", "rev-parse", "v1.0.0-rc.1", capture=True) == TAG_OBJECT
    stable = "32f853defc0a2e9f0ca809fb3535a90c977c7665"
    assert command("git", "rev-parse", "v1.0.0^{}", capture=True) == stable
    protected = ["src", "include", "tests", "CMakeLists.txt", "third_party", ".github", ".gitmodules"]
    version_paths = {"src/ordweave.c", "tests/smoke.c", "tests/consumer/main.c"}
    historical = set(command("git", "diff", "--name-only", BASELINE, stable,
                             "--", *protected, capture=True).splitlines())
    assert historical == version_paths, f"unexpected historical Stable delta: {historical}"
    accepted = "3c5f371ca53c62912dc3d4d63aa11dc5277af020"
    for path in sorted(version_paths):
        original = subprocess.check_output(["git", "show", f"{BASELINE}:{path}"], cwd=ROOT)
        released = subprocess.check_output(["git", "show", f"{stable}:{path}"], cwd=ROOT)
        before = b'"1.0.0-rc.1"'
        assert original.count(before) == 1, path
        assert released == original.replace(before, b'"1.0.0"'), path
        current = subprocess.check_output(["git", "show", f"{accepted}:{path}"], cwd=ROOT).replace(b"\r\n", b"\n")
        assert current == released.replace(b"\r\n", b"\n").replace(b'"1.0.0"', b'"1.1.0-preview.1"'), path
    allowed = version_paths | {"src/history_windows.inc", "tests/history.c",
                              "third_party/LayerKeySort", ".github/workflows/windows.yml", "CMakeLists.txt"}
    changed = set(command("git", "diff", "--name-only", stable, accepted, "--", *protected, capture=True).splitlines())
    assert changed <= allowed, f"unexpected Preview.1 implementation delta: {changed - allowed}"
    # The only non-version production correction is the reviewed early guard.
    original = subprocess.check_output(["git", "show", f"{stable}:src/history_windows.inc"], cwd=ROOT).decode()
    marker = "OwStatus ow_history_cleanup("
    prefix, body = original.split(marker)
    needle = "  if (s->recovery_root[0]) {"
    guard = ("  /* Quarantined or busy sessions must not reconstruct or repair disk roots. */\n"
             "  status = ow_store_guard(s);\n  if (status != OW_OK)\n    return status;\n")
    expected_body = body.replace(needle, guard + needle, 1)
    assert subprocess.check_output(["git", "show", f"{accepted}:src/history_windows.inc"], cwd=ROOT).decode() == prefix + marker + expected_body
    history_text = (ROOT / "src/history_windows.inc").read_text(encoding="utf-8")
    assert history_text.startswith(prefix + marker + expected_body)
    suffix = history_text[len(prefix + marker + expected_body):]
    assert re.findall(r"OwStatus\s+(ow_\w+)\s*\(", suffix) == ["ow_history_query_range"]
    cmake_before = subprocess.check_output(["git", "show", f"{stable}:CMakeLists.txt"], cwd=ROOT).decode()
    assert cmake == cmake_before.replace("project(OrdWeave VERSION 1.0.0", "project(OrdWeave VERSION 1.1.0")
    pin = "f751e1ee2afd682874281d6dc495969f9eca49eb"
    assert command("git", "-C", "third_party/LayerKeySort", "rev-parse", "HEAD", capture=True) == pin
    workflow_before = subprocess.check_output(["git", "show", f"{stable}:.github/workflows/windows.yml"], cwd=ROOT).decode()
    assert (ROOT / ".github/workflows/windows.yml").read_text(encoding="utf-8") == workflow_before.replace("70d7f0ef5857b54381b863d292a213909d87459f", pin)
    preview2_paths = {"src/ordweave.c", "src/checkpoint_core.inc", "src/occurrence_index.inc",
                      "tests/failures.c", "tests/smoke.c", "tests/consumer/main.c"}
    delta = set(command("git", "diff", "--name-only", accepted, "2bbf7640711e6561a5d5979f0de53a7f6ee0a1e8", "--", *protected, capture=True).splitlines())
    # Include the newly introduced owned implementation before it is staged.
    delta |= {"src/occurrence_index.inc"} if (ROOT / "src/occurrence_index.inc").exists() else set()
    assert delta <= preview2_paths, f"unexpected Preview.2 protected delta: {delta - preview2_paths}"
    for path in ("tests/smoke.c", "tests/consumer/main.c"):
        old = subprocess.check_output(["git", "show", f"{accepted}:{path}"], cwd=ROOT).decode()
        assert subprocess.check_output(["git", "show", f"2bbf7640711e6561a5d5979f0de53a7f6ee0a1e8:{path}"], cwd=ROOT).decode() == old.replace("1.1.0-preview.1", "1.1.0-preview.2")
    preview2 = "2bbf7640711e6561a5d5979f0de53a7f6ee0a1e8"
    preview3_paths = {"src/ordweave.c", "src/checkpoint_core.inc", "src/checkpoint_windows.inc",
                      "tests/failures.c", "tests/checkpoint_oracle.py", "tests/smoke.c", "tests/consumer/main.c"}
    current_delta = set(command("git", "diff", "--name-only", preview2, "edb9675b10125cecdf936e030bd9a54a90a84146", "--", *protected, capture=True).splitlines())
    assert current_delta <= preview3_paths, f"unexpected Preview.3 delta: {current_delta - preview3_paths}"
    for path in ("src/ordweave.c", "tests/smoke.c", "tests/consumer/main.c"):
        old = subprocess.check_output(["git", "show", f"{preview2}:{path}"], cwd=ROOT).decode()
        assert subprocess.check_output(["git", "show", f"edb9675b10125cecdf936e030bd9a54a90a84146:{path}"], cwd=ROOT).decode() == old.replace("1.1.0-preview.2", "1.1.0-preview.3")
    preview3 = "edb9675b10125cecdf936e030bd9a54a90a84146"
    preview4_paths = {"src/ordweave.c", "tests/smoke.c", "tests/consumer/main.c"}
    current_delta = set(command("git", "diff", "--name-only", preview3, research, "--", *protected, capture=True).splitlines())
    assert current_delta <= preview4_paths, f"unexpected Preview.4 delta: {current_delta - preview4_paths}"
    for path in preview4_paths:
        old = subprocess.check_output(["git", "show", f"{preview3}:{path}"], cwd=ROOT).decode()
        assert subprocess.check_output(["git", "show", f"{research}:{path}"], cwd=ROOT).decode() == old.replace("1.1.0-preview.3", "1.1.0-preview.4")
    step15_paths = {"include/ordweave.h", "src/ordweave.c", "src/field_index.inc",
                    "src/core_names.h", "src/core_internal.h", "src/persistence.c", "src/history_windows.inc",
                    "tests/core.c", "tests/range_cases.inc", "tests/failures.c", "tests/index_failures.inc",
                    "tests/history.c", "tests/consumer/main.c"}
    preview4 = "5ab01df34bcb62a9feb1c61a443a1f08c0a56a06"
    step15_delta = set(command("git", "diff", "--name-only", research, preview4, "--", *protected, capture=True).splitlines())
    assert step15_delta <= step15_paths, f"unexpected approved Step15 delta: {step15_delta-step15_paths}"
    # RC-to-Stable permits only this narrow version transition. Accepted
    # Preview production/test/build content is otherwise immutable here.
    version = re.search(r'ordweave_version\(void\) \{ return "([^"]+)";',
                        (ROOT / "src/ordweave.c").read_text(encoding="utf-8")).group(1)
    assert version in {"1.1.0-rc.1", "1.1.0"}, version
    rc_delta = set(command("git", "diff", "--name-only", preview4, "--", *protected, capture=True).splitlines())
    assert rc_delta <= version_paths | {"tests/history.c"}, f"unexpected RC implementation delta: {rc_delta-version_paths}"
    for path in sorted(version_paths):
        old = subprocess.check_output(["git", "show", f"{preview4}:{path}"], cwd=ROOT).decode()
        assert (ROOT / path).read_text(encoding="utf-8") == old.replace('"1.1.0-preview.4"', f'"{version}"'), path
    original_history = subprocess.check_output(["git", "show", f"{preview4}:tests/history.c"], cwd=ROOT).decode()
    facade_block = """  /* RC_FACADE_COVERAGE_BEGIN: explicit historical wrapper assertions. */
  {
    OwHistory *current=NULL;size_t n=0;OwCollectionId c={0};OwRecordId r={0},*ids=NULL;
    int order=0;OwValue last={.type=OW_INT,.as.integer=29};
    OK(ow_history_open(db,latest,&current));
    OK(ow_history_collection_count(current,collection,&n));T(n==6);
    OK(ow_history_occurrence_get(current,(OwOccurrenceId){1},&c,&r));
    T(c.value==collection.value&&r.value==record.value);
    OK(ow_history_occurrence_compare(current,(OwOccurrenceId){1},(OwOccurrenceId){6},&order));T(order<0);
    OK(ow_history_query_equal(current,"counter",&last,&ids,&n));
    T(n==1&&ids[0].value==record.value);ow_history_close(current);
    T(ids[0].value==record.value);ow_array_release(ids);
  }
  /* RC_FACADE_COVERAGE_END */
"""
    expected_history = original_history.replace("  latest = revision(db);\n", "  latest = revision(db);\n" + facade_block, 1)
    assert (ROOT / "tests/history.c").read_text(encoding="utf-8") == expected_history, "historical tests changed beyond approved facade coverage"
    for name in GUIDES:
        assert "v1.1.0" in (ROOT / "docs" / name).read_text(encoding="utf-8"), name
    for name in ("PERSISTENCE.md", "CHECKPOINTS.md", "HISTORY.md"):
        original = subprocess.check_output(["git", "show", f"{BASELINE}:docs/{name}"], cwd=ROOT)
        try:
            original_text = original.decode("utf-8")
        except UnicodeDecodeError:
            original_text = original.decode("cp1252")  # Legacy separator encoding, not format bytes.
        rows = lambda text: [line for line in text.splitlines() if line.startswith("|")]
        assert rows(original_text) == rows((ROOT / "docs" / name).read_text(encoding="utf-8")), name
    print("Immutable RC/Stable provenance and wire tables preserved; explicit Preview.1 pin/version/guard/path gates")
    documents = [ROOT / "AGENTS.md", ROOT / "README.md", ROOT / "README.zh-CN.md",
                 ROOT / "CHANGELOG.md",
                 ROOT / "THIRD_PARTY_NOTICES.md", *sorted((ROOT / "docs").rglob("*.md"))]
    links = 0
    for doc in documents:
        raw = doc.read_bytes()
        assert b"\r\r\n" not in raw, f"doubled CR: {doc}"
        s = raw.decode("utf-8-sig")
        assert "\ufffd" not in s and "¡ª" not in s, f"visible encoding corruption: {doc}"
        assert not any(ord(c) < 32 and c not in "\n\r\t" for c in s), f"control character: {doc}"
        text = no_fences(s)
        for match in re.finditer(r"!?\[[^\]\n]*\]\(([^)\n]+)\)", text):
            href = match.group(1).strip().strip("<>")
            uri = urlsplit(href)
            if uri.scheme or uri.netloc:
                continue  # Offline check; fixed GitHub evidence is inspected separately.
            path = (doc.parent / unquote(uri.path)).resolve() if uri.path else doc
            assert path.exists(), f"broken link {doc.relative_to(ROOT)} -> {href}"
            if uri.fragment:
                assert path.is_file() and path.suffix == ".md", f"fragment target {href}"
                assert unquote(uri.fragment) in anchors(path.read_text(encoding="utf-8-sig")), f"broken anchor {doc.name} -> {href}"
            links += 1
        for src in re.findall(r"`((?:src|include|tests)/[A-Za-z0-9_./-]+\.(?:c|h|inc|py|json))`", text):
            if doc.name != "THIRD_PARTY_NOTICES.md":  # Its src paths explicitly refer to Obscura64.
                assert (ROOT / src).is_file(), f"source reference {doc.name}: {src}"
    print(f"UTF-8 and local Markdown: {len(documents)} documents, {links} links including fragments")


def examples(generator: str, config: str) -> None:
    (ROOT / "build").mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="docs-", dir=ROOT / "build") as temp:
        build = Path(temp).resolve()
        snippets = []
        for language in ("README.md", "README.zh-CN.md"):
            snippets.append(re.findall(r"```c\n(.*?)```", (ROOT / language).read_text(encoding="utf-8"), re.S)[0])
        assert snippets[0] == snippets[1], "README examples drifted between languages"
        snippet = build / "readme-example.c"
        snippet.write_text(snippets[0], encoding="utf-8")
        args = ["cmake", "-S", str(ROOT / "docs/examples"), "-B", str(build),
                "-G", generator, f"-DORDWEAVE_SOURCE_DIR={ROOT}", f"-DDOCS_README_SOURCE={snippet}"]
        if generator.startswith("Visual Studio"):
            args += ["-A", "x64"]
        else:
            args += [f"-DCMAKE_BUILD_TYPE={config}"]
        command(*args)
        command("cmake", "--build", str(build), "--config", config)
        command("ctest", "--test-dir", str(build), "-C", config, "--output-on-failure")
    print(f"Public C17 documentation examples: quickstart + lifecycle + bilingual README ({generator}, {config})")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--examples", action="store_true")
    parser.add_argument("--generator", default="Ninja")
    parser.add_argument("--config", choices=["Debug", "Release"], default="Release")
    args = parser.parse_args()
    check()
    if args.examples:
        examples(args.generator, args.config)
